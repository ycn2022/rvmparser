#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

#ifdef _WIN32
    #ifdef E5DZIPUTILS_EXPORTS
        #define E5DZIPUTILS_API __declspec(dllexport)
    #else
        #define E5DZIPUTILS_API __declspec(dllimport)
    #endif
#else
    #define E5DZIPUTILS_API
#endif

namespace E5DZipUtils {

enum class CompressionLevel {
    FASTEST = 1,
    FAST = 3,
    NORMAL = 6,
    GOOD = 9,
    BEST = 19
};

enum class ArchiveFormat {
    ZSTD,
    ZIP,
    GZIP,
    TAR,
    UNKNOWN
};

struct CompressionOptions {
    CompressionLevel level = CompressionLevel::NORMAL;
    int threadCount = 0; // 0 = auto detect
    bool preserveStructure = true;
    size_t chunkSize = 64 * 1024 * 1024; // 64MB
};

struct FileEntry {
    std::string relativePath;
    std::string fullPath;
    size_t size;
    bool isDirectory;
};

using ProgressCallback = std::function<void(const std::string&, size_t, size_t)>;

} // namespace E5DZipUtils