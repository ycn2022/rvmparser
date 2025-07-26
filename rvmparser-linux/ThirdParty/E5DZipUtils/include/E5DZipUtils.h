#pragma once

#include "Common.h"
#include <string>
#include <vector>

namespace E5DZipUtils {

class E5DZIPUTILS_API ZipUtils {
public:
    ZipUtils();
    ~ZipUtils();

    // 压缩单个文件夹
    bool compressFolder(const std::string& folderPath, 
                       const std::string& outputPath,
                       const CompressionOptions& options = CompressionOptions(),
                       ProgressCallback callback = nullptr);

    // 压缩多个文件
    bool compressFiles(const std::vector<std::string>& filePaths,
                      const std::string& outputPath,
                      const CompressionOptions& options = CompressionOptions(),
                      ProgressCallback callback = nullptr);

    // 压缩单个文件（高性能，自动优化参数）
    bool compressSingleFile(const std::string& inputPath,
                           const std::string& outputPath,
                           CompressionLevel level = CompressionLevel::NORMAL,
                           ProgressCallback callback = nullptr);

    // 解压单个文件（高性能，自动优化参数）
    bool decompressSingleFile(const std::string& inputPath,
                             const std::string& outputPath,
                             ProgressCallback callback = nullptr);

    // 解压文件
    bool extractArchive(const std::string& archivePath,
                       const std::string& outputDir,
                       ProgressCallback callback = nullptr);

    // 列出压缩包内容
    std::vector<FileEntry> listArchiveContents(const std::string& archivePath);

    // 检测压缩格式
    ArchiveFormat detectFormat(const std::string& filePath);

    // 获取错误信息
    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace E5DZipUtils