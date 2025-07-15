#define _USE_MATH_DEFINES
#include <cmath>

#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <algorithm>
#include <span>
#include <memory>
#include <cctype>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

#include "Store.h"
#include "LinAlgOps.h"
#include <string>

#include <cstdint> 
#include <e5d_Shape.h>

#define NOMINMAX
#include "windows.h"



#define RVMPARSER_GLTF_PRETTY_PRINT (0)

namespace rj = rapidjson;

namespace ExportNamedPipe{

    // 强制 1 字节对齐，消除填充 
#pragma pack(push, 1) 

// CustomMessageHeader 结构体 
    struct CustomMessageHeader {
        // 999 结束 1 - Model 2 - Instnce 3 - Shape 
        int msgType;       // 消息类型（4 字节） 
        int contentLength; // 内容长度（4 字节） 
    };

    // ModelData 结构体 
    struct ModelData {
        int64_t Id;
        std::string Name;
    };

    // InstanceData 结构体 
    struct InstanceData {
        int64_t Id;
        int64_t ParentId;
        bool IsSignificant;
        std::string Name;
    };

    // ShapeData 结构体 
    struct ShapeData {
        int64_t Id;
        int64_t InstanceId;
        double Min[3];
        double Max[3];
        std::vector<uint8_t> Geometry;

        // 默认构造函数 
        ShapeData() : Geometry() {}
    };

    // 恢复默认的字节对齐 
#pragma pack(pop) 

    int64_t InstanceId = 0;
    int64_t ShapeId = 0;

  struct DataItem
  {
    DataItem* next = nullptr;
    const void* ptr = nullptr;
    uint32_t size = 0;
  };

  // Temporary state gathered prior to writing a GLTF file
  struct Model
  {
    rj::MemoryPoolAllocator<rj::CrtAllocator> rjAlloc = rj::MemoryPoolAllocator<rj::CrtAllocator>();

    rj::Value rjNodes = rj::Value(rj::kArrayType);
    rj::Value rjMeshes = rj::Value(rj::kArrayType);
    rj::Value rjAccessors = rj::Value(rj::kArrayType);
    rj::Value rjBufferViews = rj::Value(rj::kArrayType);
    rj::Value rjMaterials = rj::Value(rj::kArrayType);
    rj::Value rjBuffers = rj::Value(rj::kArrayType);

    uint32_t dataBytes = 0;
    ListHeader<DataItem> dataItems{};
    Arena arena;

    Map definedMaterials;

    Vec3f origin = makeVec3f(0.f);
  };

  struct GeometryItem
  {
    size_t sortKey;   // Bit 0 is line-not-line, bits 1 and up are material index
    const Geometry* geo;
  };

  struct Context {
    Logger logger = nullptr;
    
    const char* path = nullptr; // Path without suffix
    const char* suffix = nullptr;

    std::vector<char> tmpBase64;
    std::vector<Vec3f> tmp3f_1;
    std::vector<Vec3f> tmp3f_2;
    std::vector<uint32_t> tmp32ui;
    std::vector<GeometryItem> tmpGeos;

    struct {
      size_t level = 0;   // Level to do splitting, 0 for no splitting
      size_t choose = 0;  // Keeping track of which split we are processing
      size_t index = 0;   // Which subtree we are to descend
      bool done = false;  // Set to true when there are no more splits
    } split;

    bool centerModel = true;
    bool rotateZToY = true;
    bool includeAttributes = false;
    bool glbContainer = false;
    bool mergeGeometries = true;
  };

  bool IsValidUTF8(const char* str) {
      while (*str) {
          if ((*str & 0x80) == 0) str++; // ASCII字符跳过 
          else if ((*str & 0xE0) == 0xC0) { // 2字节字符 
              if ((str[1] & 0xC0) != 0x80) return false;
              str += 2;
          }
          else if ((*str & 0xF0) == 0xE0) { // 3字节字符
              for (int i = 1; i <= 2; ++i)
                  if ((str[i] & 0xC0) != 0x80) return false;
              str += 3;
          }
          else if ((*str & 0xF8) == 0xF0) { // 4字节字符 
              for (int i = 1; i <= 3; ++i)
                  if ((str[i] & 0xC0) != 0x80) return false;
              str += 4;
          }
          else return false;
      }
      return true;
  }

  // ANSI 转 UTF-8（需释放返回的 char* 内存）
  char* ConvertToUTF8(const char* ansiStr) {
      // Step 1: ANSI → WideChar
      int wlen = MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, NULL, 0);
      wchar_t* wbuf = new wchar_t[wlen];
      MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, wbuf, wlen);
      // Step 2: WideChar → UTF-8
      int ulen = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
      char* utf8Buf = new char[ulen];
      WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8Buf, ulen, NULL, NULL);

      delete[] wbuf;
      return utf8Buf; // 调用者需 delete[] 释放
  }

  std::wstring string_to_wstring(const std::string& str) {
      int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
      std::wstring result(size_needed, 0);
      MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &result[0], size_needed);
      return result;
  }

  uint32_t addDataItem(Context& /*ctx*/, Model& model, const void* ptr, size_t size, bool copy)
  {
    assert((size % 4) == 0);
    assert(model.dataBytes + size <= std::numeric_limits<uint32_t>::max());

    if (copy) {
      void* copied_ptr = model.arena.alloc(size);
      std::memcpy(copied_ptr, ptr, size);
      ptr = copied_ptr;
    }

    DataItem* item = model.arena.alloc<DataItem>();
    model.dataItems.insert(item);
    item->ptr = ptr;
    item->size = static_cast<uint32_t>(size);

    uint32_t offset = model.dataBytes;
    model.dataBytes += item->size;

    return offset;
  }

  void encodeBase64(Context& ctx, const uint8_t* data, size_t byteLength)
  {
    // Base64 table from RFC4648
    static const char rfc4648[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static_assert(sizeof(rfc4648) == 65);

    static const char prefix[] = "data:application/octet-stream;base64,";
    static const size_t prefixLength = sizeof(prefix) - 1;

    // Add data prefix
    const size_t totalLength = prefixLength + 4 * ((byteLength + 2) / 3);
    ctx.tmpBase64.resize(totalLength);
    std::memcpy(ctx.tmpBase64.data(), prefix, prefixLength);
    size_t o = prefixLength;

    // Handle range that is a multiple of three
    size_t i = 0;
    while (3 * i + 2 < byteLength) {
      const size_t i3 = 3 * i;
      const size_t i4 = 4 * i;
      const uint8_t d0 = data[i3 + 0];
      const uint8_t d1 = data[i3 + 1];
      const uint8_t d2 = data[i3 + 2];
      ctx.tmpBase64[o + i4 + 0] = rfc4648[(d0 >> 2)];
      ctx.tmpBase64[o + i4 + 1] = rfc4648[((d0 << 4) & 0x30) | (d1 >> 4)];
      ctx.tmpBase64[o + i4 + 2] = rfc4648[((d1 << 2) & 0x3c) | (d2 >> 6)];
      ctx.tmpBase64[o + i4 + 3] = rfc4648[d2 & 0x3f];
      i++;
    }

    // Handle end if byteLength is not a multiple of three
    if (3 * i < byteLength) { // End padding
      const size_t i3 = 3 * i;
      const size_t i4 = 4 * i;
      const bool two = (i3 + 1 < byteLength);  // one or two extra bytes (three would go into loop above)?
      const uint8_t d0 = data[i3 + 0];
      const uint8_t d1 = two ? data[i3 + 1] : 0;
      ctx.tmpBase64[o + i4 + 0] = rfc4648[(d0 >> 2)];
      ctx.tmpBase64[o + i4 + 1] = rfc4648[((d0 << 4) & 0x30) | (d1 >> 4)];
      ctx.tmpBase64[o + i4 + 2] = two ? rfc4648[((d1 << 2) & 0x3c)] : '=';
      ctx.tmpBase64[o + i4 + 3] = '=';
    }
  }

  void processNode(Context& ctx, HANDLE hPipe, const Node* node, size_t level, const int64_t& parentId);

  void processChildren(Context& ctx, HANDLE hPipe, const Node* firstChild, size_t level, const int64_t& parentId)
  {
    size_t nextLevel = level + 1;
    for (const Node* child = firstChild; child; child = child->next) {
        processNode(ctx, hPipe, child, nextLevel, parentId);
    }
  }

  void SendModel(Context& ctx, HANDLE hPipe, const char* modelname,int64_t & nodeId)
  {
      const char* utf8Data = nullptr;
      int utf8Len = 0;
      bool needdelete = false;
      if (modelname)
      {
          //
            // 转换为 UTF-8
          if (!IsValidUTF8(modelname))
          {
              utf8Data = ConvertToUTF8(modelname);
              needdelete = true;
          }
          else
          {
              utf8Data = modelname;
          }
          utf8Len = strlen(utf8Data); // 获取字节长度 

      }

      CustomMessageHeader pMsg;
      pMsg.msgType = 1;
      pMsg.contentLength = sizeof(int64_t) + utf8Len;


      nodeId = ++InstanceId;

      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &nodeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, utf8Data, utf8Len, &bytesWritten, NULL);


      if (needdelete)
      {
          delete[] utf8Data;
      }
  }


  void SendInstance(Context& ctx, HANDLE hPipe, const char* instName,const std::vector<double> & matrix, const int64_t& parentId, int64_t& nodeId)
  {
      const char* utf8Data = nullptr;
      int utf8Len = 0;
      bool needdelete = false;
      bool isSignificant = false;
      if (instName)
      {
          if (strlen(instName) > 0 && instName[0] == '/')
          {
              isSignificant = true;
          }
          //
            // 转换为 UTF-8
          if (!IsValidUTF8(instName))
          {
              utf8Data = ConvertToUTF8(instName);
              needdelete = true;
          }
          else
          {
              utf8Data = instName;
          }
          utf8Len = strlen(utf8Data); // 获取字节长度 

      }

      CustomMessageHeader pMsg;
      pMsg.msgType = 2;
      pMsg.contentLength = sizeof(int64_t) + sizeof(int64_t) + sizeof(bool) + utf8Len;


      nodeId = ++InstanceId;

      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &nodeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &parentId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &isSignificant, sizeof(bool), &bytesWritten, NULL);
      WriteFile(hPipe, utf8Data, utf8Len, &bytesWritten, NULL);


      if (needdelete)
      {
          delete[] utf8Data;
      }
  }

  bool Shape2Binary(std::shared_ptr< e5d_Shape>& shape, std::string& binary)
  {

      std::ostringstream outStream;

      cereal::PortableBinaryOutputArchive archive(outStream);

      archive(shape);

      binary = outStream.str();

      return true;
  }

  bool SendShape(Context& ctx, HANDLE hPipe, Geometry* geo, const int64_t& instanceId)
  {
      std::shared_ptr< e5d_Shape> shape = nullptr;
      switch (geo->kind) {

      case Geometry::Kind::Pyramid:
          break;
      case Geometry::Kind::Box:

      {
          std::shared_ptr<e5dFace_Box> subshape = std::make_shared<e5dFace_Box>();
          subshape->dX = geo->box.lengths[0];
          subshape->dY = geo->box.lengths[1];
          subshape->dZ = geo->box.lengths[2];

          shape = subshape;

      }
          break;
      case Geometry::Kind::RectangularTorus:
          break;
      case Geometry::Kind::Sphere:
      {
          std::shared_ptr<e5dFace_Sphere3d> subshape = std::make_shared<e5dFace_Sphere3d>();
          subshape->radius = geo->sphere.diameter / 2.0;

          shape = subshape;

      }
          break;
      case Geometry::Kind::Line:
          break;
      case Geometry::Kind::FacetGroup:

          break;

      case Geometry::Kind::Snout:
          break;
      case Geometry::Kind::EllipticalDish:
          break;
      case Geometry::Kind::SphericalDish:
          break;
      case Geometry::Kind::Cylinder:

      {
          std::shared_ptr<e5dFace_Cylinder> subshape = std::make_shared<e5dFace_Cylinder>();
          subshape->radius = geo->cylinder.radius;
          subshape->hight = geo->cylinder.height;

          shape = subshape;

      }
          break;

      case Geometry::Kind::CircularTorus:

          break;

      default:
          assert(false && "Illegal kind");
          break;
      }

      if (shape == nullptr)
          return false;

      std::string binstr;

      Shape2Binary(shape, binstr);


      CustomMessageHeader pMsg;
      pMsg.msgType = 3;
      pMsg.contentLength = sizeof(int64_t) + sizeof(int64_t) + sizeof(double) * 6 + binstr.length();

      double Min[3] = { geo->bboxLocal.min.x,geo->bboxLocal.min.y,geo->bboxLocal.min.z };
      double Max[3] = { geo->bboxLocal.max.x,geo->bboxLocal.max.y,geo->bboxLocal.max.z };
      std::vector<uint8_t> Geometry;

      auto shapeId = ++ShapeId;

      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &shapeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &instanceId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, Min, sizeof(Min), &bytesWritten, NULL);
      WriteFile(hPipe, Max, sizeof(Max), &bytesWritten, NULL);
      WriteFile(hPipe, binstr.data(), binstr.length(), &bytesWritten, NULL);

      return true;
  }

  bool ReadWaiting(Context& ctx, HANDLE hPipe)
  {
      // 3. 通信循环 
      DWORD bytesRead;
      CustomMessageHeader header;
      BOOL success = ReadFile(hPipe, &header, sizeof(header), &bytesRead, NULL);

      if (!success) {
          DWORD err = GetLastError();
          if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
              // 客户端已断开 [3]()

              ctx.logger(2, "已断开...");
          }
          return false;
      }

      if (header.msgType == 888)
          return true;
      return false;
  }

  void processNode(Context& ctx, HANDLE hPipe, const Node* node, size_t level,const int64_t& parentId)
  {

      int64_t nodeId = 0;
      std::vector<double> matrix;

    switch (node->kind) {
    case Node::Kind::File:
      if (node->file.path) {
        
          //

      }
      SendModel(ctx,hPipe, node->file.path, nodeId);
      ReadWaiting(ctx, hPipe);
      //if (includeContent) {
      //  addAttributes(ctx, model, rjNode, node);
      //}
      break;

    case Node::Kind::Model:
        SendInstance(ctx,hPipe, node->model.name, matrix, parentId, nodeId);
        ReadWaiting(ctx, hPipe);
      //if (includeContent) {
      //  addAttributes(ctx, model, rjNode, node);
      //}
      break;

    case Node::Kind::Group:
        if (node->group.translation[0] != 0 || node->group.translation[1] != 0 || node->group.translation[2] != 0)
        {
            matrix.push_back(1.0);
            matrix.push_back(0.0);
            matrix.push_back(0.0);
            matrix.push_back(node->group.translation[0]);

            matrix.push_back(0.0);
            matrix.push_back(1.0);
            matrix.push_back(0.0);
            matrix.push_back(node->group.translation[1]);

            matrix.push_back(0.0);
            matrix.push_back(0.0);
            matrix.push_back(1.0);
            matrix.push_back(node->group.translation[2]);

            matrix.push_back(0.0);
            matrix.push_back(0.0);
            matrix.push_back(0.0);
            matrix.push_back(1.0);
        }
        SendInstance(ctx,hPipe, node->group.name, matrix, parentId, nodeId);
        ReadWaiting(ctx, hPipe);

      //if (includeContent) 
      {
        //addAttributes(ctx, model, rjNode, node);

        if(node->group.geometries.first != nullptr) {


          for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {

              if (SendShape(ctx, hPipe, geo, nodeId))
              {
                  ReadWaiting(ctx, hPipe);
              }

          }



        }
      }
      break;

    default:
      assert(false && "Illegal enum");
      break;
    }

    if(nodeId>0)
    {
        // And recurse into children
        processChildren(ctx, hPipe, node->children.first, level, nodeId);

    }

  }


  void extendBounds(BBox3f& worldBounds, const Node* node)
  {
    for (Node* child = node->children.first; child; child = child->next) {
      extendBounds(worldBounds, child);
    }
    if (node->kind == Node::Kind::Group) {
      for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
        engulf(worldBounds, geo->bboxWorld);
      }
    }
  }

  void calculateOrigin(Context& ctx, Model& model, const Node* firstNode)
  {
    BBox3f worldBounds = createEmptyBBox3f();

    for (const Node* node = firstNode; node; node = node->next) {
      extendBounds(worldBounds, node);
    }

    model.origin = 0.5f * (worldBounds.min + worldBounds.max);
    ctx.logger(0, "exportGLTF: world bounds = [%.2f, %.2f, %.2f]x[%.2f, %.2f, %.2f]",
               worldBounds.min.x, worldBounds.min.y, worldBounds.min.z,
               worldBounds.max.x, worldBounds.max.y, worldBounds.max.z);
    ctx.logger(0, "exportGLTF: setting origin = [%.2f, %.2f, %.2f]",
               model.origin.x, model.origin.y, model.origin.z);
  }



}

using namespace ExportNamedPipe;

bool exportNamedPipe(Store* store, Logger logger, const std::string& pipename)
{
  Context ctx{
    .logger = logger
  };

  ctx.logger(0, "exportNamedPipe: rotate-z-to-y=%u center=%u attributes=%u",
             ctx.rotateZToY ? 1 : 0,
             ctx.centerModel ? 1 : 0,
             ctx.includeAttributes ? 1 : 0);

  std::wstring fullpipename = L"\\\\.\\pipe\\";

  fullpipename = fullpipename + string_to_wstring(pipename);

  HANDLE hPipe = CreateFile(
      fullpipename.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      0, NULL, OPEN_EXISTING, 0, NULL);

  //HANDLE hPipe = CreateNamedPipe(
  //    fullpipename.c_str(),         // 管道名称 
  //    PIPE_ACCESS_DUPLEX,            // 双向通信 
  //    PIPE_TYPE_MESSAGE | PIPE_WAIT, // 消息模式+阻塞 
  //    PIPE_UNLIMITED_INSTANCES,      // 最大实例数 
  //    4096, 4096,                    // 输入输出缓冲区 
  //    0, NULL);                      // 安全属性 

  processChildren(ctx, hPipe, store->getFirstRoot(), 0, 0);

  CustomMessageHeader pMsg;
  pMsg.msgType = 999;
  pMsg.contentLength = 0;

  // 发送完整数据块
  DWORD bytesWritten;
  WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);

  // 4. 清理资源 
  DisconnectNamedPipe(hPipe);
  CloseHandle(hPipe);

  return true;
}
