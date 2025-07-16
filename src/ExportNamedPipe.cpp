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

#include "Tessellator.h"
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
        double Min[3];
        double Max[3];
        std::string Name;
    };

    // InstanceData 结构体 
    struct InstanceData {
        int64_t Id;
        int64_t ParentId;
        bool IsSignificant;
        double Min[3];
        double Max[3];
        std::string Name;
    };

    // ShapeData 结构体 
    struct ShapeData {
        int64_t Id;
        int64_t InstanceId;
        int64_t MaterialId;
        double LocalMin[3];
        double LocalMax[3];
        double WorldMin[3];
        double WorldMax[3];
        double Matrix[12];
        std::string Geometry;

    };

    struct MaterialData
    {
        int64_t Id;
        int Diffuse[3];
        float Dissolve; // 1= 全透明
        float Roughness;
        float Metallic;
        int Diffuse_texname_len;
        int Alpha_texname_len;
        int Normal_texname_len;
        int Metallic_texname_len;
        int Roughness_texname_len;
        std::string Diffuse_texname;
        std::string Alpha_texname;
        std::string Normal_texname;
        std::string Metallic_texname;
        std::string Roughness_texname;
    };

    // 恢复默认的字节对齐 
#pragma pack(pop) 

    int PipeDataVersion = 1;

    int64_t InstanceId = 0;
    int64_t ShapeId = 0;
    int64_t MaterialId = 0;

    Map definedMaterials;

    Store* __store = nullptr;

  struct DataItem
  {
    DataItem* next = nullptr;
    const void* ptr = nullptr;
    uint32_t size = 0;
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

  //uint32_t addDataItem(Context& /*ctx*/, Model& model, const void* ptr, size_t size, bool copy)
  //{
  //  assert((size % 4) == 0);
  //  assert(model.dataBytes + size <= std::numeric_limits<uint32_t>::max());

  //  if (copy) {
  //    void* copied_ptr = model.arena.alloc(size);
  //    std::memcpy(copied_ptr, ptr, size);
  //    ptr = copied_ptr;
  //  }

  //  DataItem* item = model.arena.alloc<DataItem>();
  //  model.dataItems.insert(item);
  //  item->ptr = ptr;
  //  item->size = static_cast<uint32_t>(size);

  //  uint32_t offset = model.dataBytes;
  //  model.dataBytes += item->size;

  //  return offset;
  //}

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

  void processNode(Context& ctx, HANDLE hPipe, TriangulationFactory* factory, const Node* node, size_t level, const int64_t& parentId);

  void processChildren(Context& ctx, HANDLE hPipe, TriangulationFactory* factory, const Node* firstChild, size_t level, const int64_t& parentId)
  {
    size_t nextLevel = level + 1;
    for (const Node* child = firstChild; child; child = child->next) {
        processNode(ctx, hPipe, factory, child, nextLevel, parentId);
    }
  }



  void SendModel(Context& ctx, HANDLE hPipe, const Node* node, const char* modelname,int64_t & nodeId)
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
      pMsg.contentLength = sizeof(int64_t) + sizeof(double) * 6 + utf8Len;


      double worldMin[3] = { node->bboxWorld.min.x,node->bboxWorld.min.y,node->bboxWorld.min.z };
      double worldMax[3] = { node->bboxWorld.max.x,node->bboxWorld.max.y,node->bboxWorld.max.z };

      nodeId = ++InstanceId;

      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &nodeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, worldMin, sizeof(worldMin), &bytesWritten, NULL);
      WriteFile(hPipe, worldMax, sizeof(worldMax), &bytesWritten, NULL);
      WriteFile(hPipe, utf8Data, utf8Len, &bytesWritten, NULL);


      if (needdelete)
      {
          delete[] utf8Data;
      }
  }


  void SendInstance(Context& ctx, HANDLE hPipe, const Node* node, const char* instName,const std::vector<double> & matrix, const int64_t& parentId, int64_t& nodeId)
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
      pMsg.contentLength = sizeof(int64_t) + sizeof(int64_t) + sizeof(bool) + sizeof(double) * 6 + utf8Len;

      double worldMin[3] = { node->bboxWorld.min.x,node->bboxWorld.min.y,node->bboxWorld.min.z };
      double worldMax[3] = { node->bboxWorld.max.x,node->bboxWorld.max.y,node->bboxWorld.max.z };

      nodeId = ++InstanceId;

      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &nodeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &parentId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &isSignificant, sizeof(bool), &bytesWritten, NULL);
      WriteFile(hPipe, worldMin, sizeof(worldMin), &bytesWritten, NULL);
      WriteFile(hPipe, worldMax, sizeof(worldMax), &bytesWritten, NULL);
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


  int64_t createOrGetColor(Context& /*ctx*/, const Geometry* geo, bool& created)
  {
      uint32_t color = geo->color;
      uint8_t transparency = static_cast<uint8_t>(geo->transparency);
      // make sure key is never zero
      uint64_t key = (uint64_t(color) << 9) | (uint64_t(transparency) << 1) | 1;
      if (uint64_t val; definedMaterials.get(val, key)) {
          return uint32_t(val);
      }

      created = true;

      int64_t matId = ++MaterialId;

      definedMaterials.insert(key, matId);

      return matId;
  }

  bool SendMaterial(Context& ctx, HANDLE hPipe, Geometry* geo, int64_t& matId)
  {
      bool created = false;
      matId = createOrGetColor(ctx, geo, created);

      if (created)
      {

          int t = ((geo->color >> 24) & 0xFF);


          int r = ((geo->color >> 16) & 0xFF);
          int g = ((geo->color >> 8) & 0xFF);
          int b = ((geo->color) & 0xFF);

          uint8_t transparency = static_cast<uint8_t>(geo->transparency);

          float dissolve = transparency / 100.f;// 1 - std::min(1.f, std::max(0.f, 1.f - (1.f / 100.f) * transparency));


          CustomMessageHeader pMsg;
          pMsg.msgType = 4;
          pMsg.contentLength = sizeof(int64_t) + sizeof(int) * 3 + sizeof(float) * 3 + sizeof(int) * 5;

          MaterialData matData;
          matData.Id = matId;
          matData.Diffuse[0] = r;
          matData.Diffuse[1] = g;
          matData.Diffuse[2] = b;
          matData.Dissolve = dissolve;
          matData.Roughness = 0.4f;
          matData.Metallic = 0.6f;
          matData.Diffuse_texname_len = 0;
          matData.Alpha_texname_len = 0;
          matData.Normal_texname_len = 0;
          matData.Metallic_texname_len = 0;
          matData.Roughness_texname_len = 0;

          // 发送完整数据块
          DWORD bytesWritten;
          WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Id, sizeof(int64_t), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Diffuse, sizeof(int) * 3, &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Dissolve, sizeof(float), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Roughness, sizeof(float), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Metallic, sizeof(float), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Diffuse_texname_len, sizeof(int), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Alpha_texname_len, sizeof(int), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Normal_texname_len, sizeof(int), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Metallic_texname_len, sizeof(int), &bytesWritten, NULL);
          WriteFile(hPipe, &matData.Roughness_texname_len, sizeof(int), &bytesWritten, NULL);


          return true;
      }
      else
      {
          return false;
      }
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



  bool SendShape(Context& ctx, HANDLE hPipe, TriangulationFactory* factory, Geometry* geo, const int64_t& instanceId)
  {

      int64_t  matId = 0;
      if (SendMaterial(ctx, hPipe, geo, matId))
      {
          ReadWaiting(ctx, hPipe);
      }

      std::shared_ptr< e5d_Shape> shape = nullptr;
      switch (geo->kind) {

      case Geometry::Kind::Pyramid:
      {
          std::shared_ptr<e5dFace_Pyramid> subshape = std::make_shared<e5dFace_Pyramid>();
          subshape->top[0] = geo->pyramid.top[0];
          subshape->top[1] = geo->pyramid.top[1];
          subshape->bottom[0] = geo->pyramid.bottom[0];
          subshape->bottom[1] = geo->pyramid.bottom[1];
          subshape->offset[0] = geo->pyramid.offset[0];
          subshape->offset[1] = geo->pyramid.offset[1];
          subshape->height = geo->pyramid.height;

          shape = subshape;

      }
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
      {
          std::shared_ptr<e5dFace_RectangularTorus> subshape = std::make_shared<e5dFace_RectangularTorus>();
          subshape->angle = geo->rectangularTorus.angle;
          subshape->height = geo->rectangularTorus.height;
          subshape->inner_radius = geo->rectangularTorus.inner_radius;
          subshape->outer_radius = geo->rectangularTorus.outer_radius;

          shape = subshape;

      }
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

      {
          auto scale = getScale(geo->M_3x4);
          geo->triangulation = factory->facetGroup(&__store->arenaTriangulation, geo, scale);
          std::shared_ptr<e5d_Mesh> subshape = std::make_shared<e5d_Mesh>();
          for (size_t i = 0; i < geo->triangulation->vertices_n; i++)
          {
              {
                  std::shared_ptr<e5d_Vector> vt = std::make_shared<e5d_Vector>();
                  vt->x = geo->triangulation->vertices[i * 3];
                  vt->y = geo->triangulation->vertices[i * 3 + 1];
                  vt->z = geo->triangulation->vertices[i * 3 + 2];
                  subshape->postions.push_back(vt);
              }
              {
                  std::shared_ptr<e5d_Vector> vt = std::make_shared<e5d_Vector>();
                  vt->x = geo->triangulation->normals[i * 3];
                  vt->y = geo->triangulation->normals[i * 3 + 1];
                  vt->z = geo->triangulation->normals[i * 3 + 2];
                  subshape->normals.push_back(vt);
              }
          }
          for (size_t i = 0; i < geo->triangulation->triangles_n; i++)
          {
              subshape->faces.push_back(geo->triangulation->indices[i * 3]);
              subshape->faces.push_back(geo->triangulation->indices[i * 3 + 1]);
              subshape->faces.push_back(geo->triangulation->indices[i * 3 + 2]);
          }

          shape = subshape;

      }
          break;

      case Geometry::Kind::Snout:
      {
          std::shared_ptr<e5dFace_Snout> subshape = std::make_shared<e5dFace_Snout>();
          subshape->radius_t = geo->snout.radius_t;
          subshape->tshear[0] = geo->snout.tshear[0];
          subshape->tshear[1] = geo->snout.tshear[1];
          subshape->radius_b = geo->snout.radius_b;
          subshape->bshear[0] = geo->snout.bshear[0];
          subshape->bshear[1] = geo->snout.bshear[1];
          subshape->height = geo->snout.height;
          subshape->offset[0] = geo->snout.offset[0];
          subshape->offset[1] = geo->snout.offset[1];

          shape = subshape;

      }
          break;
      case Geometry::Kind::EllipticalDish:

      {
          std::shared_ptr<e5dFace_EllipticalDish> subshape = std::make_shared<e5dFace_EllipticalDish>();
          subshape->baseRadius = geo->ellipticalDish.baseRadius;
          subshape->height = geo->ellipticalDish.height;

          shape = subshape;

      }
          break;
      case Geometry::Kind::SphericalDish:


      {
          std::shared_ptr<e5dFace_SphericalDish> subshape = std::make_shared<e5dFace_SphericalDish>();
          subshape->baseRadius = geo->sphericalDish.baseRadius;
          subshape->height = geo->sphericalDish.height;

          shape = subshape;

      }
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


      {
          std::shared_ptr<e5dFace_CircularTorus> subshape = std::make_shared<e5dFace_CircularTorus>();
          subshape->radius = geo->circularTorus.radius;
          subshape->angle = geo->circularTorus.angle;
          subshape->offset = geo->circularTorus.offset;

          shape = subshape;

      }
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
      pMsg.contentLength = sizeof(int64_t) + sizeof(int64_t) + sizeof(int64_t) + sizeof(double) * 24 + binstr.length();

      double localMin[3] = { geo->bboxLocal.min.x,geo->bboxLocal.min.y,geo->bboxLocal.min.z };
      double localMax[3] = { geo->bboxLocal.max.x,geo->bboxLocal.max.y,geo->bboxLocal.max.z };
      double worldMin[3] = { geo->bboxWorld.min.x,geo->bboxWorld.min.y,geo->bboxWorld.min.z };
      double worldMax[3] = { geo->bboxWorld.max.x,geo->bboxWorld.max.y,geo->bboxWorld.max.z };

      double Matrix[12] = {
        geo->M_3x4.m00,
        geo->M_3x4.m01,
         geo->M_3x4.m02,
         geo->M_3x4.m03,
         geo->M_3x4.m10,
         geo->M_3x4.m11,
         geo->M_3x4.m12,
         geo->M_3x4.m13,
         geo->M_3x4.m20,
         geo->M_3x4.m21,
         geo->M_3x4.m22,
         geo->M_3x4.m23
      };

      //std::vector<uint8_t> Geometry;

      auto shapeId = ++ShapeId;


      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      WriteFile(hPipe, &shapeId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &instanceId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, &matId, sizeof(int64_t), &bytesWritten, NULL);
      WriteFile(hPipe, localMin, sizeof(localMin), &bytesWritten, NULL);
      WriteFile(hPipe, localMax, sizeof(localMax), &bytesWritten, NULL);
      WriteFile(hPipe, worldMin, sizeof(worldMin), &bytesWritten, NULL);
      WriteFile(hPipe, worldMax, sizeof(worldMax), &bytesWritten, NULL);
      WriteFile(hPipe, Matrix, sizeof(Matrix), &bytesWritten, NULL);
      WriteFile(hPipe, binstr.data(), binstr.length(), &bytesWritten, NULL);




      return true;
  }


  void SendPipeDataVersion(Context& ctx, HANDLE hPipe)
  {

      CustomMessageHeader pMsg;
      pMsg.msgType = 100;
      pMsg.contentLength = PipeDataVersion;


      // 发送完整数据块
      DWORD bytesWritten;
      WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);


  }

  void processNode(Context& ctx, HANDLE hPipe, TriangulationFactory* factory, const Node* node, size_t level, const int64_t& parentId)
  {

      int64_t nodeId = 0;
      std::vector<double> matrix;

      switch (node->kind) {
      case Node::Kind::File:
          if (node->file.path) {

              //

          }
          SendModel(ctx, hPipe, node, node->file.path, nodeId);
          ReadWaiting(ctx, hPipe);
          //if (includeContent) {
          //  addAttributes(ctx, model, rjNode, node);
          //}
          break;

      case Node::Kind::Model:
          SendInstance(ctx, hPipe, node, node->model.name, matrix, parentId, nodeId);
          ReadWaiting(ctx, hPipe);
          //if (includeContent) {
          //  addAttributes(ctx, model, rjNode, node);
          //}
          break;

      case Node::Kind::Group:
          //if (node->group.translation[0] != 0 || node->group.translation[1] != 0 || node->group.translation[2] != 0)
          //{
          //    matrix.push_back(1.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(node->group.translation[0]);

          //    matrix.push_back(0.0);
          //    matrix.push_back(1.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(node->group.translation[1]);

          //    matrix.push_back(0.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(1.0);
          //    matrix.push_back(node->group.translation[2]);

          //    matrix.push_back(0.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(0.0);
          //    matrix.push_back(1.0);
          //}
          SendInstance(ctx, hPipe, node, node->group.name, matrix, parentId, nodeId);
          ReadWaiting(ctx, hPipe);

          //if (includeContent) 
          {
              //addAttributes(ctx, model, rjNode, node);

              if (node->group.geometries.first != nullptr) {


                  for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {

                      if (SendShape(ctx, hPipe, factory, geo, nodeId))
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

      if (nodeId > 0)
      {
          // And recurse into children
          processChildren(ctx, hPipe, factory, node->children.first, level, nodeId);

      }

  }


  void extendBounds(BBox3f& worldBounds, Node* node)
  {
      BBox3f nodeBounds = createEmptyBBox3f();
    for (Node* child = node->children.first; child; child = child->next) {
      extendBounds(nodeBounds, child);
    }
    if (node->kind == Node::Kind::Group) {
      for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
        engulf(nodeBounds, geo->bboxWorld);
      }
    }
    node->bboxWorld = nodeBounds;
    engulf(worldBounds, nodeBounds);
  }

  //void calculateOrigin(Context& ctx, Model& model, Node* firstNode)
  //{
  //  BBox3f worldBounds = createEmptyBBox3f();

  //  for (Node* node = firstNode; node; node = node->next) {
  //    extendBounds(worldBounds, node);
  //  }

  //  model.origin = 0.5f * (worldBounds.min + worldBounds.max);
  //  ctx.logger(0, "exportGLTF: world bounds = [%.2f, %.2f, %.2f]x[%.2f, %.2f, %.2f]",
  //             worldBounds.min.x, worldBounds.min.y, worldBounds.min.z,
  //             worldBounds.max.x, worldBounds.max.y, worldBounds.max.z);
  //  ctx.logger(0, "exportGLTF: setting origin = [%.2f, %.2f, %.2f]",
  //             model.origin.x, model.origin.y, model.origin.z);
  //}



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

    if (hPipe != INVALID_HANDLE_VALUE) {

        //// 获取当前超时配置 
        //COMMTIMEOUTS timeouts;
        //GetCommTimeouts(hPipe, &timeouts);

        //FString timeoutsettingfile = DataAccessUtil::GetExeDir() / "meshconverttimeout";
        //FString timeoutsetting;
        //FFileHelper::LoadFileToString(timeoutsetting, *timeoutsettingfile);

        //if (timeoutsetting.IsNumeric())
        //{
        //    timeouts.ReadTotalTimeoutConstant = FCString::Atoi(*timeoutsetting);
        //}
        //else
        //{
        //    timeouts.ReadTotalTimeoutConstant = 60000;   // 固定总超时60秒 
        //}

        //ReadTotalTimeoutConstant = timeouts.ReadTotalTimeoutConstant;

        //// 设置读超时参数（单位：毫秒）
        ////timeouts.ReadIntervalTimeout = 50;          // 字节间最大间隔50ms 
        ////timeouts.ReadTotalTimeoutMultiplier = 10;   // 每字节耗时10ms 

        //// 应用新配置 
        //SetCommTimeouts(hPipe, &timeouts);



        //STUDIO_LOG(LogTemp, Display, TEXT("connected %s. set timeout to %d(ms,setting by meshconverttimeout file)"),
        //    *mConvertInfo.PipeServerAppParam, timeouts.ReadTotalTimeoutConstant);

        //SendConfig();

        //bShouldCreatePipe = false;

        //return true;
    }
    else
    {
        ctx.logger(2, "初始化失败");
        return false;
    }

    SendPipeDataVersion(ctx, hPipe);

    ReadWaiting(ctx, hPipe);

    //HANDLE hPipe = CreateNamedPipe(
    //    fullpipename.c_str(),         // 管道名称 
    //    PIPE_ACCESS_DUPLEX,            // 双向通信 
    //    PIPE_TYPE_MESSAGE | PIPE_WAIT, // 消息模式+阻塞 
    //    PIPE_UNLIMITED_INSTANCES,      // 最大实例数 
    //    4096, 4096,                    // 输入输出缓冲区 
    //    0, NULL);                      // 安全属性 

    BBox3f worldBounds = createEmptyBBox3f();

    extendBounds(worldBounds, store->getFirstRoot());

    __store = store;

    float tolerance = 0.1f;
    int maxSamples = 100;
    auto factory = new TriangulationFactory(store, logger, tolerance, 3, maxSamples);

    processChildren(ctx, hPipe, factory, store->getFirstRoot(), 0, 0);


    delete factory;

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
