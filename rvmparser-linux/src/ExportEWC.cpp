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


#ifdef HAVE_DATA_ACCESS
#include "DataAccess.h"
#endif

#include <execution>

#ifdef HAVE_STUDIO_MESH_SERIALIZE
#include "StudioMeshSerialize.h"
#endif


// NOMINMAX is already defined in CMakeLists.txt

#include <objbase.h>
#include <sstream>
#include <shellapi.h>
#include <filesystem>

#include "windows.h"
#include <future>
#include "TriangulationMeshSerialize.h"

#include "E5DZipUtils.h"
using namespace E5DZipUtils;

#include <iostream>

#define NOTWRITEDB 0

#define NOTPARSESHAPE 0

#define NOTSHAPE2BIN 0

#define RVMPARSER_GLTF_PRETTY_PRINT (0)

namespace rj = rapidjson;

namespace ExportEWC{

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
        int64_t ShapeInstanceId;
        int64_t InstanceId;
        int64_t MaterialId;
        int Name_len;
        std::string Name;
        double LocalMin[3];
        double LocalMax[3];
        double WorldMin[3];
        double WorldMax[3];
        double Matrix[12];
        std::string MatrixStr;
        int GeometryType = 0;
        int TriangleCount = 0;
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

    int64_t GlobalInstanceId = 0;
    int64_t GlobalShapeId = 0;
    int64_t GlobalMaterialId = 0;

    //每次发送4M数据
    int PipeDataBufferLen = 1024 * 1024 * 4;

    ////复用内存，用来序列化shape。初始10M。不够的话就扩容
    //char* geometrystr = nullptr;
    //int geometrystrlength = 1024 * 1024 * 10;

    Map definedMaterials;

    Store* __store = nullptr;

    const float pi = float(M_PI);
    const float half_pi = float(0.5 * M_PI);
    const float one_over_pi = float(1.0 / M_PI);
    const float twopi = float(2.0 * M_PI);


  struct DataItem
  {
    DataItem* next = nullptr;
    const void* ptr = nullptr;
    uint32_t size = 0;
  };



  struct GeometryItem
  {
      GeometryItem(Geometry* ingeo, const char* const inGroupName, const int& inGeoIndex,
          const int64_t& inMatId, const int64_t& inInstId,
          const int64_t& inShapeInstId, const int64_t& inShapeId,const std::string& inGeoData) :
          geo(ingeo), groupName(inGroupName), geometryIndex(inGeoIndex),
          matId(inMatId), instId(inInstId), shapeInstId(inShapeInstId), 
          shapeId(inShapeId),geometrydata(inGeoData)
      {

      }
    size_t sortKey;   // Bit 0 is line-not-line, bits 1 and up are material index
    Geometry* geo;
    const char* groupName;
    const int geometryIndex;
    const int64_t matId;
    const int64_t instId;
    const int64_t shapeInstId;
    const int64_t shapeId;
    const std::string geometrydata;
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

    std::atomic<int> processeditemnum = 0;
    int totalitemnum = 0;

    std::atomic< long long>  shape2binns = 0;
    std::atomic< long long>  sqliteopens = 0;
    std::atomic< long long>  shapeparsens = 0;
    std::atomic< long long>  facegroupparsens = 0;


    std::atomic< long long>  addmeshns = 0;
    std::atomic< long long>  addshapens = 0;
    std::atomic< long long>  addgeons = 0;
    std::atomic< long long>  addinstns = 0;
    std::atomic< long long>  addassons = 0;
    std::atomic< long long>  addshapeinstns = 0;

    std::atomic< long long>  sqliteopetimes = 0;

    std::atomic< long long> writelog = 0;

    int modelnum = 0;
    int instancenum = 0;
    int shapenum = 0;
    int materialnum = 0;

    std::vector< ModelData> model;
    std::vector<InstanceData> instances;
    std::vector<ShapeData> shapes;
    std::vector<E5D::Studio::BaseMaterial>materials;

    std::vector<GeometryItem> geometries;

    bool geometryasmesh = true;

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
#ifdef _WIN32
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
#else
      // Linux下假设输入已经是UTF-8或者直接复制
      size_t len = strlen(ansiStr) + 1;
      char* utf8Buf = new char[len];
      strcpy(utf8Buf, ansiStr);
      return utf8Buf; // 调用者需 delete[] 释放
#endif
  }

  char* unicodeToUtf8(const WCHAR* zWideFilename) {
#ifdef _WIN32
      int nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
      char* zFilename = (char*)malloc(nByte);
      WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte, 0, 0);
      return zFilename;
#else
      // Linux implementation: convert wchar_t* to UTF-8
      size_t len = wcslen(zWideFilename);
      size_t utf8_len = len * 4 + 1; // worst case: each wchar_t becomes 4 bytes in UTF-8
      char* zFilename = (char*)malloc(utf8_len);
      
      size_t result = wcstombs(zFilename, zWideFilename, utf8_len);
      if (result == (size_t)-1) {
          // Conversion failed, return empty string
          free(zFilename);
          zFilename = (char*)malloc(1);
          zFilename[0] = '\0';
      }
      
      return zFilename;
#endif
  }

  // 辅助函数：将ANSI路径转UTF-8
  std::string AnsiToUTF8(const char* ansiPath) {
      int wlen = MultiByteToWideChar(CP_ACP, 0, ansiPath, -1, NULL, 0);
      wchar_t* wstr = new wchar_t[wlen];
      MultiByteToWideChar(CP_ACP, 0, ansiPath, -1, wstr, wlen);

      int ulen = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
      char* utf8Path = new char[ulen];
      WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8Path, ulen, NULL, NULL);

      std::string result(utf8Path);
      delete[] wstr;
      delete[] utf8Path;
      return result;
  }

  std::wstring string_to_wstring(const std::string& str) {

      std::string utf8path;
      if (!IsValidUTF8(str.data()))
      {
          utf8path = AnsiToUTF8(str.data());
      }
      else
      {
          utf8path = str;
      }

#ifdef _WIN32
      int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8path.c_str(), (int)utf8path.size(), NULL, 0);
      std::wstring result(size_needed, 0);
      MultiByteToWideChar(CP_UTF8, 0, utf8path.c_str(), (int)utf8path.size(), &result[0], size_needed);
      return result;
#else
      // Linux implementation: convert UTF-8 string to wstring
      size_t len = utf8path.length();
      std::wstring result;
      result.reserve(len);
      
      size_t i = 0;
      while (i < len) {
          wchar_t wc;
          size_t bytes_consumed = mbrtowc(&wc, &utf8path[i], len - i, nullptr);
          
          if (bytes_consumed == (size_t)-1 || bytes_consumed == (size_t)-2) {
              // Invalid UTF-8 sequence, skip this byte
              i++;
              continue;
          } else if (bytes_consumed == 0) {
              // Null character
              break;
          } else {
              result.push_back(wc);
              i += bytes_consumed;
          }
      }
      
      return result;
#endif
  }

  std::string GenerateGuidString() {
      GUID guid;
      if (CoCreateGuid(&guid) == S_OK) {
          std::ostringstream oss;
          oss << std::hex << std::uppercase
              << std::setfill('0')
              << std::setw(8) << guid.Data1 
              << std::setw(4) << guid.Data2 
              << std::setw(4) << guid.Data3 
              << std::setw(2) << static_cast<int>(guid.Data4[0])
              << std::setw(2) << static_cast<int>(guid.Data4[1]) 
              << std::setw(2) << static_cast<int>(guid.Data4[2])
              << std::setw(2) << static_cast<int>(guid.Data4[3])
              << std::setw(2) << static_cast<int>(guid.Data4[4])
              << std::setw(2) << static_cast<int>(guid.Data4[5])
              << std::setw(2) << static_cast<int>(guid.Data4[6])
              << std::setw(2) << static_cast<int>(guid.Data4[7]);
          return oss.str();
      }
      return ""; // 生成失败处理 
  }

  // 辅助函数：将double转换为字符串，去除末尾多余的0和小数点
  std::string doubleToString(const double& value) {
      // 判断是否为整数
      if (std::floor(value) == value) {
          return std::to_string(static_cast<long long>(value));
      }

      // 否则按浮点数处理
      std::ostringstream oss;
      oss.precision(std::numeric_limits<double>::digits10);
      oss << std::fixed << value;

      std::string s = oss.str();
      // 去除多余的0和小数点
      size_t pos = s.find_last_not_of('0');
      if (pos != std::string::npos && s[pos] == '.') {
          s = s.substr(0, pos);
      }
      else if (pos != std::string::npos) {
          s.erase(pos + 1);
      }

      return s;
  }

  // 将文件移动到回收站
  bool MoveFileToRecycleBin(const std::string& filePath) {

#ifdef _WIN32
      std::wstring wFilePath = string_to_wstring(filePath);
      
      // 确保路径以双空字符结尾（SHFileOperation要求）
      wFilePath.push_back(L'\0');
      
      SHFILEOPSTRUCTW fileOp = {};
      fileOp.wFunc = FO_DELETE;
      fileOp.pFrom = wFilePath.c_str();
      fileOp.fFlags = FOF_ALLOWUNDO | FOF_NO_UI; // 允许撤销（移到回收站）且不显示UI
      
      int result = SHFileOperationW(&fileOp);
      return (result == 0);
#else
      std::filesystem::remove(filePath);
#endif
  }

  bool GetClusterSize(const wchar_t* path, DWORD& clusterSize) {
#ifdef _WIN32
      // Step 1: 获取挂载点的卷路径
      WCHAR volumePath[MAX_PATH];
      if (!GetVolumePathNameW(path, volumePath, MAX_PATH)) {
          std::wcerr << L"Failed to get volume path. Error code: " << GetLastError() << std::endl;
          return false;
      }

      // Step 2: 获取卷的唯一标识（GUID）
      WCHAR volumeName[MAX_PATH];
      if (!GetVolumeNameForVolumeMountPointW(volumePath, volumeName, MAX_PATH)) {
          std::wcerr << L"Failed to get volume name. Error code: " << GetLastError() << std::endl;
          return false;
      }

      // Step 3: 查询卷的簇大小
      DWORD sectorsPerCluster, bytesPerSector, numberOfFreeClusters, totalNumberOfClusters;
      if (GetDiskFreeSpaceW(volumeName, &sectorsPerCluster, &bytesPerSector,
          &numberOfFreeClusters, &totalNumberOfClusters)) {
          clusterSize = sectorsPerCluster * bytesPerSector; // 簇大小 = 每簇扇区数 * 每扇区字节数
          return true;
      }
      else {
          std::wcerr << L"Failed to get cluster size. Error code: " << GetLastError() << std::endl;
          return false;
      }
#else
      // Linux implementation: get page size for the filesystem containing the path
      struct statvfs vfs;
      std::string utf8path;
      if (!IsValidUTF8(path))
      {
          char* utf8Data = ConvertToUTF8(path);
          utf8path = std::string(utf8Data);
          delete[] utf8Data;
      }
      else
      {
          size_t len = wcslen(path);
          char* utf8Data = new char[len * 4 + 1];
          size_t result = wcstombs(utf8Data, path, len * 4 + 1);
          if (result != (size_t)-1) {
              utf8path = std::string(utf8Data);
          }
          delete[] utf8Data;
      }
      
      if (statvfs(utf8path.c_str(), &vfs) == 0) {
          clusterSize = vfs.f_frsize; // Fragment size (preferred I/O size)
          if (clusterSize == 0) {
              clusterSize = vfs.f_bsize; // Block size
          }
          return true;
      }
      else {
          clusterSize = 4096; // Default page size on Linux
          return false;
      }

#endif
  }

  int execute(const char* cmd) {

      //// 命令字符串，替换为实际 EXE 路径和参数
      //const char* command = "\"C:\\Path\\To\\YourExe.exe\" param1 param2";

      // 打开管道，"r" 表示读取标准输出
      std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
      if (!pipe) {
          std::cerr << "Failed to open pipe" << std::endl;
          return 1;
      }

      // 读取输出
      std::string output;
      char buffer[128];
      while (fgets(buffer, sizeof(buffer), pipe.get()) != nullptr) {
          output += buffer;
      }

      // 获取命令的退出码
      int returnCode = _pclose(pipe.get());
      if (returnCode == -1) {
          std::cerr << "Failed to close pipe or command failed" << std::endl;
          return 1;
      }

      // 输出结果
      std::cout << "Command output:\n" << output << std::endl;
      std::cout << "Return code: " << returnCode << std::endl;

      return returnCode;
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

  void processNode(Context& ctx, E5D::Studio::DataAccess& da,  TriangulationFactory* factory, const Node* node, size_t level, const int64_t& parentId);

  void processChildren(Context& ctx, E5D::Studio::DataAccess& da, TriangulationFactory* factory, const Node* firstChild, size_t level, const int64_t& parentId)
  {
      size_t nextLevel = level + 1;
      for (const Node* child = firstChild; child; child = child->next) {
          processNode(ctx, da, factory, child, nextLevel, parentId);
      }
  }

  bool ReadWaiting(Context& ctx, HANDLE hPipe)
  {
      return true;
      //// 3. 通信循环 
      //DWORD bytesRead;
      //CustomMessageHeader header;
      //BOOL success = ReadFile(hPipe, &header, sizeof(header), &bytesRead, NULL);

      //if (!success) {
      //    DWORD err = GetLastError();
      //    if (err == ERROR_BROKEN_PIPE || err == ERROR_NO_DATA) {
      //        // 客户端已断开 [3]()

      //        ctx.logger(2, "已断开...");
      //    }
      //    return false;
      //}

      //if (header.msgType == 888)
      //    return true;
      //return false;
  }



  void SendPipeDataBuffer(Context& ctx, HANDLE hPipe, char* PipeDataBuffer, size_t& PipeDataBufferPos)
  {
      char* CurrentBufferAddress = PipeDataBuffer + PipeDataBufferPos;

      PipeDataBufferPos += sizeof(CustomMessageHeader);

      CustomMessageHeader& pMsg = *(new (CurrentBufferAddress)CustomMessageHeader());
      pMsg.msgType = 777;

      DWORD bytesWritten;
      WriteFile(hPipe, PipeDataBuffer, PipeDataBufferPos, &bytesWritten, NULL);

      ReadWaiting(ctx, hPipe);

      PipeDataBufferPos = 0;
  }


  bool SendModel(Context& ctx, E5D::Studio::DataAccess& da,  const Node* node, const char* modelname,int64_t & nodeId)
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

      nodeId = ++GlobalInstanceId;


      //ctx.logger(1, "SendModel %ld", nodeId);

      //直接写入数据库
      std::string utf8name;
      if (utf8Data)
          utf8name = utf8Data;

      if (needdelete)
      {
          delete[] utf8Data;
          utf8Data = nullptr;
      }

      utf8name = std::filesystem::path(utf8name).filename().string();

      //ModelData model;
      //model.Id = nodeId;
      //model.Min[0] = node->bboxWorld.min.x;
      //model.Min[1] = node->bboxWorld.min.y;
      //model.Min[2] = node->bboxWorld.min.z;
      //model.Max[0] = node->bboxWorld.max.x;
      //model.Max[1] = node->bboxWorld.max.y;
      //model.Max[2] = node->bboxWorld.max.z;
      //model.Name = utf8name;
      //ctx.model.push_back(model);

      auto time0 = std::chrono::high_resolution_clock::now();

#if !NOTWRITEDB


      //ctx.logger(1, "AddModel %ld,%s", nodeId, utf8name.c_str());

      if (!da.AddModelAndBoundBox(utf8name, nodeId,
          node->bboxWorld.min.x,
          node->bboxWorld.min.y,
          node->bboxWorld.min.z,
          node->bboxWorld.max.x,
          node->bboxWorld.max.y,
          node->bboxWorld.max.z))
      {
          ctx.logger(2, "add model failed: %s", utf8name.c_str());
          return false;
      }
      //if (!da.UpdateInstanceBoundingBox(nodeId,
      //    node->bboxWorld.min.x,
      //    node->bboxWorld.min.y,
      //    node->bboxWorld.min.z,
      //    node->bboxWorld.max.x,
      //    node->bboxWorld.max.y,
      //    node->bboxWorld.max.z))
      //{
      //    ctx.logger(2, "update boundbox failed: %s", utf8name.c_str());
      //    return false;
      //}
#endif
      
      long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

      ctx.sqliteopens += e;
      //ctx.sqliteopetimes++;

      return true;
  }


  bool SendInstance(Context& ctx, E5D::Studio::DataAccess& da, const BBox3f& bboxWorld, const char* instName,const std::vector<double> & matrix, const int64_t& parentId, int64_t& nodeId)
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


      nodeId = ++GlobalInstanceId;

      //ctx.logger(1, "SendInstance %ld", nodeId);

      //直接写入数据库

      std::string utf8name;
      if (utf8Data)
          utf8name = utf8Data;


      if (needdelete)
      {
          delete[] utf8Data;
          utf8Data = nullptr;
      }

      //InstanceData inst;
      //inst.Id = nodeId;
      //inst.IsSignificant = isSignificant;
      //inst.Min[0] = bboxWorld.min.x;
      //inst.Min[1] = bboxWorld.min.y;
      //inst.Min[2] = bboxWorld.min.z;
      //inst.Max[0] = bboxWorld.max.x;
      //inst.Max[1] = bboxWorld.max.y;
      //inst.Max[2] = bboxWorld.max.z;
      //inst.Name = utf8name;
      //inst.ParentId = parentId;
      //ctx.instances.push_back(inst);
      


      auto time0 = std::chrono::high_resolution_clock::now();

#if !NOTWRITEDB

      {
          auto time01 = std::chrono::high_resolution_clock::now();
          if (!da.AddInstanceAndBoundBox(nodeId, E5D::Studio::DataAccess::ComponentClassId, utf8name, nullptr, isSignificant,
              bboxWorld.min.x,
              bboxWorld.min.y,
              bboxWorld.min.z,
              bboxWorld.max.x,
              bboxWorld.max.y,
              bboxWorld.max.z))
          {
              ctx.logger(2, "add instance failed: %s", utf8name.c_str());
              return false;
          }
          auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


          ctx.addinstns.fetch_add(e1, std::memory_order_relaxed);
      }
      //if (!da.UpdateInstanceBoundingBox(nodeId,
      //    bboxWorld.min.x,
      //    bboxWorld.min.y,
      //    bboxWorld.min.z,
      //    bboxWorld.max.x,
      //    bboxWorld.max.y,
      //    bboxWorld.max.z))
      //{
      //    ctx.logger(2, "update boundbox failed: %s", utf8name.c_str());
      //    return false;
      //}
      {
          auto time01 = std::chrono::high_resolution_clock::now();
          if (!da.AddAsso(parentId, nodeId, E5D::Studio::DataAccess::TreeAssoId))
          {
              ctx.logger(2, "add asso failed: %s", utf8name.c_str());
              return false;
          }
          auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


          ctx.addassons.fetch_add(e1, std::memory_order_relaxed);
      }
#endif

      long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

      ctx.sqliteopens += e;
      //ctx.sqliteopetimes++;

      //CustomMessageHeader pMsg;
      //pMsg.msgType = 2;
      //pMsg.contentLength = sizeof(int64_t) + sizeof(int64_t) + sizeof(bool) + sizeof(double) * 6 + utf8Len;

      //double worldMin[3] = { bboxWorld.min.x,bboxWorld.min.y,bboxWorld.min.z };
      //double worldMax[3] = { bboxWorld.max.x,bboxWorld.max.y,bboxWorld.max.z };

      //nodeId = ++GlobalInstanceId;

      //// 发送完整数据块
      //DWORD bytesWritten;
      //WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
      //WriteFile(hPipe, &nodeId, sizeof(int64_t), &bytesWritten, NULL);
      //WriteFile(hPipe, &parentId, sizeof(int64_t), &bytesWritten, NULL);
      //WriteFile(hPipe, &isSignificant, sizeof(bool), &bytesWritten, NULL);
      //WriteFile(hPipe, worldMin, sizeof(worldMin), &bytesWritten, NULL);
      //WriteFile(hPipe, worldMax, sizeof(worldMax), &bytesWritten, NULL);
      //WriteFile(hPipe, utf8Data, utf8Len, &bytesWritten, NULL);

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

      int64_t matId = ++GlobalMaterialId;

      definedMaterials.insert(key, matId);

      return matId;
  }

  bool SendMaterial(Context& ctx, E5D::Studio::DataAccess& da,  Geometry* geo, int64_t& matId)
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

          float roughness = 0.4f;
          float metallic = 0.6f;

          int  Diffuse_texname_len = 0;
          int  Alpha_texname_len = 0;
          int  Normal_texname_len = 0;
          int  Metallic_texname_len = 0;
          int  Roughness_texname_len = 0;

          E5D::Studio::BaseMaterial mat;
          mat.id = matId;
          mat.diffuse[0] = r;
          mat.diffuse[1] = g;
          mat.diffuse[2] = b;
          mat.dissolve = dissolve;
          mat.metallic = metallic;
          mat.roughness = roughness;

          //MaterialData matdata;
          //matdata.Id = matId;
          //matdata.Diffuse[0] = r;
          //matdata.Diffuse[1] = g;
          //matdata.Diffuse[2] = b;
          //matdata.Dissolve = dissolve;
          //matdata.Metallic = metallic;
          //matdata.Roughness = roughness;

          //ctx.materials.push_back(mat);
          


          auto time0 = std::chrono::high_resolution_clock::now();

#if !NOTWRITEDB

          if (!da.AddMaterial(matId,"",mat))
          {
              ctx.logger(2, "add material failed: %d", matId);
              return false;
          }
#endif

          long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

          ctx.sqliteopens += e;
          //ctx.sqliteopetimes++;

          //CustomMessageHeader pMsg;
          //pMsg.msgType = 4;
          //pMsg.contentLength = sizeof(int64_t) + sizeof(int) * 3 + sizeof(float) * 3 + sizeof(int) * 5;

          //MaterialData matData;
          //matData.Id = matId;
          //matData.Diffuse[0] = r;
          //matData.Diffuse[1] = g;
          //matData.Diffuse[2] = b;
          //matData.Dissolve = dissolve;
          //matData.Roughness = 0.4f;
          //matData.Metallic = 0.6f;
          //matData.Diffuse_texname_len = 0;
          //matData.Alpha_texname_len = 0;
          //matData.Normal_texname_len = 0;
          //matData.Metallic_texname_len = 0;
          //matData.Roughness_texname_len = 0;

          //// 发送完整数据块
          //DWORD bytesWritten;
          //WriteFile(hPipe, &pMsg, sizeof(CustomMessageHeader), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Id, sizeof(int64_t), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Diffuse, sizeof(int) * 3, &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Dissolve, sizeof(float), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Roughness, sizeof(float), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Metallic, sizeof(float), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Diffuse_texname_len, sizeof(int), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Alpha_texname_len, sizeof(int), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Normal_texname_len, sizeof(int), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Metallic_texname_len, sizeof(int), &bytesWritten, NULL);
          //WriteFile(hPipe, &matData.Roughness_texname_len, sizeof(int), &bytesWritten, NULL);


          return true;
      }
      else
      {
          return false;
      }
  }


  bool SendShape(Context& ctx, E5D::Studio::DataAccess& da, TriangulationFactory* factory, const char* instName, const int& GeoIndex, Geometry* geo, 
      const int64_t& instanceId, const int64_t& shapeInstId, const int64_t& shapeId, const int64_t& matId)
  {
      if (geo->kind == Geometry::Kind::Line)
          return false;

      //if (geo->triangulation == nullptr)
      //    return false;

      int TriangleCount = 0;

      std::string geoname;

      auto scale = getScale(geo->M_3x4);


      auto time0 = std::chrono::high_resolution_clock::now();


      //std::ostringstream geometrystr;
      //geometrystr.sync_with_stdio(false); // 禁用与 stdio 的同步

      int geometrytype = (int)geo->kind + 100;


      //ctx.logger(1, "SendInstance %d", geometrytype);

      //TriangleCount = geo->triangulation->triangles_n;

#if !NOTPARSESHAPE

      switch (geo->kind) {

      case Geometry::Kind::Pyramid:
      {
          
          TriangleCount = 12;

          geoname = "Pyramid " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::Box:
      {

          TriangleCount = 12;

          geoname = "Box " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::RectangularTorus:
      {

          TriangleCount = 24;

          geoname = "RectangularTorus " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::Sphere:
      {

          TriangleCount = factory->sagittaBasedSegmentCount(twopi, geo->sphere.diameter / 2, scale) * 5;

          geoname = "Sphere " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::Line:
          break;
      case Geometry::Kind::FacetGroup:
      {


          TriangleCount = 0;
          for (size_t i = 0; i < geo->facetGroup.polygons_n; i++)
          {
              auto& polygon = geo->facetGroup.polygons[i];
              if (polygon.contours_n == 1)
              {
                  if (polygon.contours[0].vertices_n == 3)
                      TriangleCount += 1;
                  else if (polygon.contours[0].vertices_n == 4)
                      TriangleCount += 2;
              }
              else
              {
                  for (size_t j = 0; j < polygon.contours_n; j++)
                  {
                      TriangleCount += polygon.contours[j].vertices_n / 3;
                  }
              }
          }

          geoname = "FacetGroup " + std::to_string(GeoIndex) + " of ";

      }
      break;

      case Geometry::Kind::Snout:
      {

          auto radius_max = std::max(geo->snout.radius_b, geo->snout.radius_t);
          TriangleCount = factory->sagittaBasedSegmentCount(twopi, radius_max, scale) * 3;

          geoname = "Snout " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::EllipticalDish:

      {
          TriangleCount = factory->sagittaBasedSegmentCount(twopi, geo->ellipticalDish.baseRadius, scale) * 3;

          geoname = "EllipticalDish " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::SphericalDish:
      {

          TriangleCount = factory->sagittaBasedSegmentCount(twopi, geo->sphericalDish.baseRadius, scale) * 3;

          geoname = "SphericalDish " + std::to_string(GeoIndex) + " of ";

      }
      break;
      case Geometry::Kind::Cylinder:

      {

          TriangleCount = factory->sagittaBasedSegmentCount(twopi, geo->cylinder.radius, scale) * 3;

          geoname = "Cylinder " + std::to_string(GeoIndex) + " of ";

      }
      break;

      case Geometry::Kind::CircularTorus:
      {

          TriangleCount = factory->sagittaBasedSegmentCount(twopi, geo->circularTorus.radius, scale) *
              factory->sagittaBasedSegmentCount(twopi, geo->circularTorus.offset, scale);

          geoname = "CircularTorus " + std::to_string(GeoIndex) + " of ";

      }
      break;

      default:
          assert(false && "Illegal kind");
          break;
      }

#endif


      size_t geosize = 0;

      char* localgeometrystr = nullptr;

      if (ctx.geometryasmesh)
      {
          if (!__store->serializeGeometry(geo, localgeometrystr, geosize))
          {
              ctx.logger(1, "serialize error,%s", instName);
              return false;
          }
      }
      else
      {

          TriangulationMeshSerialize meshSerial(geo->triangulation);
          uint8* buffer;
          int32 bufsize;
          meshSerial.Serialize(buffer, bufsize);
          if (bufsize > 0)
          {
              localgeometrystr = (char*)buffer;
              geosize = bufsize;
          }
          else
          {
              ctx.logger(1, "serialize error,%s", instName);
          }
      }

      //{
      //    float scale = 1.f;
      //    //测试反序列化
      //    Geometry testgeo;
      //    Store::deserializeGeometry(localgeometrystr, geosize, &testgeo, scale);
      //    Store::DeleteFacetGroup(&testgeo);
      //}

      long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

      ctx.shapeparsens.fetch_add(e, std::memory_order_relaxed);

      std::string shapename = geoname + (instName ? std::string(instName) : "");

      //if (e > 3e6)
      //{
      //    ctx.logger(1, "shape parse %s %lldms", shapename.c_str(), e / 1000000);
      //}

      if (geometrytype == 0 || geosize == 0)
          return false;

#if !NOTPARSESHAPE
      //if (shape == nullptr)
      //    return false;
#endif


      if (TriangleCount == 0)
      {
          bool debug = true;
      }

      if (geo->triangulation == nullptr)
      {
          bool debug = true;
      }
      else if (geo->triangulation->triangles_n == 0)
      {
          bool debug = true;
      }


      //shape->matrix

      std::string binstr;


      time0 = std::chrono::high_resolution_clock::now();

#if !NOTSHAPE2BIN
      binstr = std::string(localgeometrystr, geosize);
#endif


      if (!ctx.geometryasmesh && localgeometrystr != nullptr)
      {
          delete[] localgeometrystr;
          localgeometrystr = nullptr;
      }

      //Store管理不用delete
      //if (localgeometrystr != nullptr)
      //{
      //    delete[] localgeometrystr;
      //    localgeometrystr = nullptr;
      //}

      e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

      ctx.shape2binns.fetch_add(e, std::memory_order_relaxed);

      //if (e > 10e6)
      //{
      //    ctx.logger(1, "shape bin %s %lldms", shapename.c_str(), e / 1000000);
      //}


      const char* utf8Data = nullptr;
      int utf8Len = 0;
      bool needdelete = false;

      if (!IsValidUTF8(shapename.data()))
      {
          utf8Data = ConvertToUTF8(shapename.data());
          needdelete = true;
      }
      else
      {
          utf8Data = shapename.data();
      }
      utf8Len = strlen(utf8Data); // 获取字节长度 






      //写入数据库
      std::string utf8name;
      if (utf8Data)
          utf8name = utf8Data;


      if (needdelete)
      {
          delete[] utf8Data;
          utf8Data = nullptr;
      }

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
      std::string matrixstr;

      if (Matrix[0] != 1.0 ||
          Matrix[1] != 0.0 ||
          Matrix[2] != 0.0 ||
          Matrix[3] != 0.0 ||
          Matrix[4] != 0.0 ||
          Matrix[5] != 1.0 ||
          Matrix[6] != 0.0 ||
          Matrix[7] != 0.0 ||
          Matrix[8] != 0.0 ||
          Matrix[9] != 0.0 ||
          Matrix[10] != 1.0 ||
          Matrix[11] != 0.0)
      {
          matrixstr = "[" +
              doubleToString(Matrix[0]) + "," + doubleToString(Matrix[1]) + "," +
              doubleToString(Matrix[2]) + "," + doubleToString(Matrix[3]) + "," +
              doubleToString(Matrix[4]) + "," + doubleToString(Matrix[5]) + "," +
              doubleToString(Matrix[6]) + "," + doubleToString(Matrix[7]) + "," +
              doubleToString(Matrix[8]) + "," + doubleToString(Matrix[9]) + "," +
              doubleToString(Matrix[10]) + "," + doubleToString(Matrix[11]) +
              ",0,0,0,1]";

          if (!IsValidUTF8(matrixstr.data()))
          {
              auto utf8matrix = ConvertToUTF8(matrixstr.data());

              matrixstr = std::string(utf8matrix);

              delete[] utf8matrix;
          }
      }

      time0 = std::chrono::high_resolution_clock::now();

      {
          //std::lock_guard<std::mutex> lock(mtx);

          //auto shapeId = ++GlobalShapeId;
          //int64_t shapeInstanceId = ++GlobalInstanceId;
#if !NOTWRITEDB

          {
              auto time01 = std::chrono::high_resolution_clock::now();
              if (!da.AddInstanceAndBoundBox(shapeInstId, E5D::Studio::DataAccess::ShapeClassId, utf8name, nullptr, false,
                  geo->bboxWorld.min.x,
                  geo->bboxWorld.min.y,
                  geo->bboxWorld.min.z,
                  geo->bboxWorld.max.x,
                  geo->bboxWorld.max.y,
                  geo->bboxWorld.max.z))
              {
                  ctx.logger(2, "add inst failed: %s", utf8name.c_str());
                  return false;
              }
              auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


              ctx.addshapeinstns.fetch_add(e1, std::memory_order_relaxed);
          }

          {
              auto time01 = std::chrono::high_resolution_clock::now();
              if (!da.AddAsso(instanceId, shapeInstId, E5D::Studio::DataAccess::TreeAssoId))
              {
                  ctx.logger(2, "add asso failed: %s", utf8name.c_str());
                  return false;
              }
              auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


              ctx.addassons.fetch_add(e1, std::memory_order_relaxed);
          }

#endif



#if !NOTWRITEDB


          {
              auto time01 = std::chrono::high_resolution_clock::now();
              if (!da.AddShape(shapeId, shapeInstId, shapeId, matId,
                  geo->bboxWorld.min.x,
                  geo->bboxWorld.min.y,
                  geo->bboxWorld.min.z,
                  geo->bboxWorld.max.x,
                  geo->bboxWorld.max.y,
                  geo->bboxWorld.max.z, matrixstr))
              {
                  ctx.logger(2, "add shape failed: %s", utf8name.c_str());
                  return false;
              }
              auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


              ctx.addshapens.fetch_add(e1, std::memory_order_relaxed);
          }

          std::vector<uint8_t> geobin;

          {
              auto time01 = std::chrono::high_resolution_clock::now();
              if (!da.AddGeometry(shapeId, shapeId, geometrytype, geobin))
              {
                  ctx.logger(2, "add geometry failed: %s", utf8name.c_str());
                  return false;
              }
              auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


              ctx.addgeons.fetch_add(e1, std::memory_order_relaxed);
          }

          //GeometryItem geoItem(geo, instName, GeoIndex, matId, instanceId, shapeInstId, shapeId,binstr);

          //ctx.geometries.push_back(geoItem);

          //std::vector<uint8_t> meshbin(binstr.begin(), binstr.end());

          {
              auto time01 = std::chrono::high_resolution_clock::now();
              if (!da.AddMesh(shapeId, shapeId, TriangleCount,
                  geo->bboxLocal.min.x,
                  geo->bboxLocal.min.y,
                  geo->bboxLocal.min.z,
                  geo->bboxLocal.max.x,
                  geo->bboxLocal.max.y,
                  geo->bboxLocal.max.z, binstr))
              {
                  ctx.logger(2, "add mesh failed: %s", utf8name.c_str());
                  return false;
              }
              auto e1 = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time01)).count();


              ctx.addmeshns.fetch_add(e1, std::memory_order_relaxed);
          }
#endif
      }

      e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

      ctx.sqliteopens.fetch_add(e, std::memory_order_relaxed);


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

  void processNode(Context& ctx, E5D::Studio::DataAccess& da,  TriangulationFactory* factory, const Node* node, size_t level, const int64_t& parentId)
  {


      int64_t nodeId = 0;
      std::vector<double> matrix;

      switch (node->kind) {
      case Node::Kind::File:
          if (node->file.path) {

              //

          }
          SendModel(ctx, da, node, node->file.path, nodeId);
          //ReadWaiting(ctx, hPipe);
          //if (includeContent) {
          //  addAttributes(ctx, model, rjNode, node);
          //}
          break;

      case Node::Kind::Model:
          SendInstance(ctx, da, node->bboxWorld, node->model.name, matrix, parentId, nodeId);
          //ReadWaiting(ctx, hPipe);
          //if (includeContent) {
          //  addAttributes(ctx, model, rjNode, node);
          //}
          break;

      case Node::Kind::Group:
      {
          SendInstance(ctx, da,  node->bboxWorld, node->group.name, matrix, parentId, nodeId);
          //ReadWaiting(ctx, hPipe);

          //bool debugname = false;

          //if (node->group.name)
          //{
          //    std::string groupname(node->group.name);

          //    if (groupname.ends_with("FLANGE 3 of BRANCH /P333AGW/B1"))
          //    {
          //        debugname = true;
          //    }
          //}

          //if (debugname)
              //if (includeContent) 
          {
              //addAttributes(ctx, model, rjNode, node);

              if (node->group.geometries.first != nullptr) {

                  int GeoIndex = 0;
                  for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
                      ++GeoIndex;
                      //std::string geoname = node->group.name;

                      //int64_t shapeNodeId;

                      //SendInstance(ctx, hPipe, node->group.bboxWorld, geoname.c_str(), matrix, nodeId, shapeNodeId);
                      //ReadWaiting(ctx, hPipe);

                      int64_t  matId = 0;
                      if (SendMaterial(ctx, da, geo, matId))
                      {
                          //ReadWaiting(ctx, hPipe);
                      }

                      auto shapeInstId = ++GlobalInstanceId;
                      auto shapeId = ++GlobalShapeId;

                      //GeometryItem geoItem(geo, node->group.name, GeoIndex, matId, nodeId, shapeInstId, shapeId);

                      //ctx.geometries.push_back(geoItem);



                      if (SendShape(ctx, da, factory, node->group.name, GeoIndex, geo, nodeId, shapeInstId, shapeId, matId))
                      {
                          //ReadWaiting(ctx, hPipe);
                      }

                      ctx.processeditemnum++;

                      if (ctx.processeditemnum % 10000 == 0)
                      {
                          auto time0 = std::chrono::high_resolution_clock::now();
                              
                          ctx.logger(0, "processed %d / %d", ctx.processeditemnum.load(), ctx.totalitemnum);

                          long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
                          
                          ctx.writelog += e;
                      }
                  }



              }
          }

          ctx.processeditemnum++;

          if (ctx.processeditemnum % 10000 == 0)
          {
              auto time0 = std::chrono::high_resolution_clock::now();

              ctx.logger(0, "processed %d / %d", ctx.processeditemnum.load(), ctx.totalitemnum);

              long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();

              ctx.writelog.fetch_add(e, std::memory_order_relaxed);
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
          processChildren(ctx, da,  factory, node->children.first, level, nodeId);

      }

  }


  void extendBounds(Context& ctx, BBox3f& worldBounds, Node* node)
  {
      BBox3f nodeBounds = createEmptyBBox3f();
      for (Node* child = node->children.first; child; child = child->next) {
          extendBounds(ctx, nodeBounds, child);
      }
      if (node->kind == Node::Kind::Group) {
          ctx.totalitemnum++;
          ctx.instancenum++;
          for (Geometry* geo = node->group.geometries.first; geo; geo = geo->next) {
              ctx.shapenum++;
              ctx.totalitemnum++;
              engulf(nodeBounds, geo->bboxWorld);
          }
      }
      else if (node->kind == Node::Kind::Model)
      {
          ctx.instancenum++;
      }
      else if (node->kind == Node::Kind::File)
      {
          ctx.modelnum++;
      }
      node->bboxWorld = nodeBounds;
      engulf(worldBounds, nodeBounds);
  }


}

using namespace ExportEWC;

bool exportEWC(Store* store, Logger logger, const std::string& filename,const bool& delexistfile, 
    const bool& geometryasmesh,const bool& compresszip,const std::string & outformat)
{


    Context ctx{
      .logger = logger,
      .geometryasmesh = geometryasmesh
    };

    ctx.logger(0, "export: rotate-z-to-y=%u center=%u attributes=%u",
        ctx.rotateZToY ? 1 : 0,
        ctx.centerModel ? 1 : 0,
        ctx.includeAttributes ? 1 : 0);


    std::string ewcfilename = filename +
        (outformat.starts_with(".") ? "" : ".") +
        outformat;

    // 检查文件是否存在，如果存在则移动到回收站
    if (std::filesystem::exists(ewcfilename)) {
        if (delexistfile)
        {
            std::filesystem::remove(ewcfilename);
        }
        else
        {
            ctx.logger(1, "文件已存在，正在移动到回收站: %s", ewcfilename.c_str());
            if (MoveFileToRecycleBin(ewcfilename)) {
                ctx.logger(1, "成功将文件移动到回收站: %s", ewcfilename.c_str());
            }
            else {
                ctx.logger(2, "移动文件到回收站失败: %s", ewcfilename.c_str());
                return false;
            }
        }
    }
    {
        std::string wal = ewcfilename + "-wal";
        // 检查文件是否存在，如果存在则移动到回收站
        if (std::filesystem::exists(wal)) {
            if (delexistfile)
            {
                std::filesystem::remove(wal);
            }
            else
            {
                ctx.logger(1, "文件已存在，正在移动到回收站: %s", wal.c_str());
                if (MoveFileToRecycleBin(wal)) {
                    ctx.logger(1, "成功将文件移动到回收站: %s", wal.c_str());
                }
                else {
                    ctx.logger(2, "移动文件到回收站失败: %s", wal.c_str());
                    return false;
                }
            }
        }
    }
    {
        std::string wal = ewcfilename + "-shm";
        // 检查文件是否存在，如果存在则移动到回收站
        if (std::filesystem::exists(wal)) {
            if (delexistfile)
            {
                std::filesystem::remove(wal);
            }
            else
            {
                ctx.logger(1, "文件已存在，正在移动到回收站: %s", wal.c_str());
                if (MoveFileToRecycleBin(wal)) {
                    ctx.logger(1, "成功将文件移动到回收站: %s", wal.c_str());
                }
                else {
                    ctx.logger(2, "移动文件到回收站失败: %s", wal.c_str());
                    return false;
                }
            }
        }
    }
    //获取ewcfilename的目录
    std::string outputfolder = std::filesystem::path(ewcfilename).parent_path().string();

    //获取ewcfilename的文件名（带扩展名）
    std::string outputcleanfilename = std::filesystem::path(ewcfilename).filename().string();

    std::string cleanfilename = std::filesystem::path(filename).stem().string();


    // 获取ewcfilename文件的大小，并记录在TempDbFileSize变量
    std::uintmax_t TempDbFileSize = 0;
    if (std::filesystem::exists(ewcfilename)) {
        TempDbFileSize = std::filesystem::file_size(ewcfilename);
        ctx.logger(1, "文件初始大小: %zu bytes", static_cast<size_t>(TempDbFileSize));
    }

    if(!IsValidUTF8(ewcfilename.data()))
    {
        ewcfilename = AnsiToUTF8(ewcfilename.data());

        outputfolder = AnsiToUTF8(outputfolder.data());

        cleanfilename = AnsiToUTF8(cleanfilename.data());

    }


    std::wstring wfilename = string_to_wstring(ewcfilename);
    std::wstring woutputfolder = string_to_wstring(outputfolder);

    DWORD clustersize = 0;

    if (!GetClusterSize(woutputfolder.c_str(), clustersize))
    {
        clustersize = 4 * 1024;
    }
    else
    {
        ctx.logger(1, "get page size : %d", clustersize);
    }

    E5D::Studio::DataAccess da;
    da.E5dDbPath =  ewcfilename;
    da.CreateIndex = false;
    da.AutoCommitNum = 0;
    da.WALMode = false;
    da.PageSize = clustersize;// 32 * 1024;

    //ctx.logger(1, "OpenLocalDatabase");

    if (!da.OpenLocalDatabase())
    {
        ctx.logger(2, "打开ewc文件失败: %s", ewcfilename.c_str());
        return false;
    }


    std::string signcode = GenerateGuidString();
    std::string cacheguid = GenerateGuidString();

    std::map<std::string, std::string> settings;
    //if(ctx.geometryasmesh)
    //{
    //    settings.insert(std::pair<std::string, std::string>("StoreGeometryInfo", "True"));
    //}
    settings.insert(std::pair<std::string, std::string>("SignatureCode", signcode));
    settings.insert(std::pair<std::string, std::string>("cacheguid", cacheguid));
    settings.insert(std::pair<std::string, std::string>("name", cleanfilename));
    settings.insert(std::pair<std::string, std::string>("context", cleanfilename));
    da.UpdateProjectSettings(settings);

    //ctx.logger(1, "UpdateProjectSettings success");

    BBox3f worldBounds = createEmptyBBox3f();

    extendBounds(ctx, worldBounds, store->getFirstRoot());

    //ctx.model.reserve(ctx.modelnum);
    //ctx.instances.reserve(ctx.instancenum);
    //ctx.shapes.reserve(ctx.shapenum);
    //ctx.materials.reserve(1000);

    ctx.geometries.reserve(ctx.shapenum);

    __store = store;

    float tolerance = 0.1f;
    int maxSamples = 100;
    auto factory = new TriangulationFactory(store, logger, tolerance, 6, maxSamples);

    auto time0 = std::chrono::high_resolution_clock::now();


    processChildren(ctx, da, factory, store->getFirstRoot(), 0, 0);


    delete factory;




    ctx.logger(0, "shape2bin:%lldms,shapeparse:%lldms,facetgroupparse:%lldms,sqliteope:%lldms,sqlitetimes:%lld",
    ctx.shape2binns.load() / 1000000, ctx.shapeparsens.load() / 1000000,
    ctx.facegroupparsens.load() / 1000000, ctx.sqliteopens.load() / 1000000, ctx.sqliteopetimes.load() / 1000000);


    ctx.logger(0, "addassons:%lldms,addgeons:%lldms,addinstns:%lldms,addmeshns:%lldms,addshapeinstns:%lld,addshapens:%lld",
        ctx.addassons.load() / 1000000, ctx.addgeons.load() / 1000000,
        ctx.addinstns.load() / 1000000, ctx.addmeshns.load() / 1000000,
        ctx.addshapeinstns.load() / 1000000, ctx.addshapens.load() / 1000000);

    ctx.logger(0, "write log:%lldms", ctx.writelog/ 1000000);

    long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
    logger(0, "processed  in %lldms", e / 1000000);

    logger(0, "da num %d", da.PendingCommitBatchNum.load());


    time0 = std::chrono::high_resolution_clock::now();

    if (!da.CreateInitIndex())
    {
        ctx.logger(2, "重建索引失败: %s", ewcfilename.c_str());
        return false;
    }
    e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
    logger(0, "重建索引 in %lldms", e / 1000000);

    //if(!da.ReIndexAfterBatch())
    //{
    //    ctx.logger(2, "批处理恢复失败: %s", ewcfilename.c_str());
    //    return false;
    //}
    //e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
    //logger(0, "批处理恢复 in %lldms", e / 1000000);

    if (!da.EndForBatch(true))
    {
        ctx.logger(2, "提交ewc文件失败: %s", ewcfilename.c_str());
        return false;
    }
    if (!da.CloseLocalDatabase())
    {
        ctx.logger(2, "关闭ewc文件失败: %s", ewcfilename.c_str());
        return false;
    }


    // da.CloseLocalDatabase();之后再次检查文件大小，如果相等，则删除ewcfilename
    if (std::filesystem::exists(ewcfilename)) {
        std::uintmax_t currentFileSize = std::filesystem::file_size(ewcfilename);
        ctx.logger(1, "文件关闭后大小: %zu bytes", static_cast<size_t>(currentFileSize));
        
        if (currentFileSize == TempDbFileSize) {
            ctx.logger(1, "文件大小未变化，正在删除文件: %s", ewcfilename.c_str());
            if (std::filesystem::remove(ewcfilename)) {
                ctx.logger(1, "成功删除文件: %s", ewcfilename.c_str());
            } else {
                ctx.logger(2, "删除文件失败: %s", ewcfilename.c_str());
            }

            return false;
        } else {
            ctx.logger(1, "文件大小已变化，保留文件: %s (初始: %zu bytes, 当前: %zu bytes)", 
                ewcfilename.c_str(), static_cast<size_t>(TempDbFileSize), static_cast<size_t>(currentFileSize));
        }
    }

    if (compresszip)
    {
        auto time0 = std::chrono::high_resolution_clock::now();
        std::string outputzipfile = ewcfilename + ".ewz";

        std::string cmd = "zstd  -f -v -T0 --adapt --exclude-compressed --progress \"" + ewcfilename + "\" -o \"" + outputzipfile + "\"";

        int success = execute(cmd.c_str()) == 0;

        long long e = std::chrono::duration_cast<std::chrono::nanoseconds>((std::chrono::high_resolution_clock::now() - time0)).count();
        logger(0, "ewz zip  in %lldms , success:%d", e / 1000000, success ? 1 : 0);

        if (success)
        {
            if (remove(ewcfilename.c_str()) == 0) {
                // 删除成功
            }
            else {
                // 删除失败，可调用ShowError处理错误
                logger(0, "origin result file delete failed, please delete it : %s", ewcfilename.c_str());
            }
        }

    }

    return true;
}
