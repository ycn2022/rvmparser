#pragma once

#include "Common.h"
#include <string>
#include <vector>
#include <memory>

namespace E5DZipUtils {

class ArchiveReader {
public:
    ArchiveReader();
    ~ArchiveReader();

    // 检测压缩格式
    ArchiveFormat detectFormat(const std::string& filePath);

    // 解压ZIP格式 (简单实现，不依赖外部库)
    bool extractZip(const std::string& zipPath, 
                   const std::string& outputDir,
                   ProgressCallback callback = nullptr);

    // 解压GZIP格式
    bool extractGzip(const std::string& gzipPath,
                    const std::string& outputPath,
                    ProgressCallback callback = nullptr);

    // 解压TAR格式
    bool extractTar(const std::string& tarPath,
                   const std::string& outputDir,
                   ProgressCallback callback = nullptr);

    // 列出ZIP内容
    std::vector<FileEntry> listZipContents(const std::string& zipPath);

    std::string getLastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace E5DZipUtils