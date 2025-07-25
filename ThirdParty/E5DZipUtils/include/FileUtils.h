#pragma once

#include "Common.h"
#include <string>
#include <vector>

namespace E5DZipUtils {

class FileUtils {
public:
    // 递归扫描文件夹
    static std::vector<FileEntry> scanDirectory(const std::string& dirPath);

    // 获取文件大小
    static size_t getFileSize(const std::string& filePath);

    // 检查文件是否存在
    static bool fileExists(const std::string& filePath);

    // 创建目录
    static bool createDirectories(const std::string& dirPath);

    // 获取文件扩展名
    static std::string getFileExtension(const std::string& filePath);

    // 获取相对路径
    static std::string getRelativePath(const std::string& basePath, 
                                     const std::string& fullPath);

    // 规范化路径
    static std::string normalizePath(const std::string& path);

    // 获取父目录
    static std::string getParentDirectory(const std::string& filePath);

    // 获取文件名
    static std::string getFileName(const std::string& filePath);

    // 读取文件内容
    static bool readFile(const std::string& filePath, std::vector<uint8_t>& data);

    // 写入文件内容
    static bool writeFile(const std::string& filePath, const void* data, size_t size);
};

} // namespace E5DZipUtils