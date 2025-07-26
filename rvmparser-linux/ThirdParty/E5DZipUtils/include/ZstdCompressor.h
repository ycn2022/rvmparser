#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <memory>

namespace E5DZipUtils {

class ZstdCompressor {
public:
    ZstdCompressor();
    ~ZstdCompressor();

    bool compressFile(const std::string& inputPath, 
                     const std::string& outputPath,
                     CompressionLevel level = CompressionLevel::NORMAL);

    bool compressBuffer(const void* input, size_t inputSize,
                       std::vector<uint8_t>& output,
                       CompressionLevel level = CompressionLevel::NORMAL);

    bool decompressFile(const std::string& inputPath, 
                       const std::string& outputPath);

    bool decompressBuffer(const void* input, size_t inputSize,
                         std::vector<uint8_t>& output);

    // 高性能单文件压缩
    bool compressSingleFileOptimized(const std::string& inputPath,
                                   const std::string& outputPath,
                                   CompressionLevel level,
                                   ProgressCallback callback = nullptr);

    // 高性能单文件解压
    bool decompressSingleFileOptimized(const std::string& inputPath,
                                     const std::string& outputPath,
                                     ProgressCallback callback = nullptr);

    // 创建TAR+ZSTD格式压缩包
    bool createTarZstd(const std::vector<FileEntry>& files,
                      const std::string& outputPath,
                      const CompressionOptions& options,
                      ProgressCallback callback = nullptr);

    // 解压TAR+ZSTD格式
    bool extractTarZstd(const std::string& archivePath,
                       const std::string& outputDir,
                       ProgressCallback callback = nullptr);

    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace E5DZipUtils