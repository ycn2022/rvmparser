#pragma once

#include "platform_config.h"
#include <string>
#include <vector>
#include <memory>

// 7-Zip SDK前向声明
#ifdef _WIN32
    // Windows下使用7-Zip SDK
    struct IInArchive;
    struct IOutArchive;
    struct IArchiveUpdateCallback;
    struct IArchiveExtractCallback;
#endif

namespace E5D {
    namespace Compression {
        
        /**
         * @brief 压缩结果结构体
         */
        struct CompressionResult {
            bool success;
            std::string errorMessage;
            size_t originalSize;
            size_t compressedSize;
            double compressionRatio;
            
            CompressionResult() : success(false), originalSize(0), compressedSize(0), compressionRatio(0.0) {}
            
            // 显式定义拷贝构造函数和赋值操作符
            CompressionResult(const CompressionResult& other) = default;
            CompressionResult& operator=(const CompressionResult& other) = default;
            
            // 显式定义移动构造函数和移动赋值操作符
            CompressionResult(CompressionResult&& other) noexcept = default;
            CompressionResult& operator=(CompressionResult&& other) noexcept = default;
            
            // 显式定义析构函数
            ~CompressionResult() = default;
        };

        /**
         * @brief 7z文件压缩工具类 (使用7-Zip SDK)
         */
        class SevenZipCompressor {
        public:
            SevenZipCompressor();
            ~SevenZipCompressor();

            /**
             * @brief 压缩单个文件到7z格式
             * @param inputFilePath 输入文件路径
             * @param outputArchivePath 输出7z文件路径
             * @param compressionLevel 压缩级别 (0-9, 默认5)
             * @return 压缩结果
             */
            CompressionResult CompressFile(const std::string& inputFilePath, 
                                         const std::string& outputArchivePath, 
                                         int compressionLevel = 5);

            /**
             * @brief 压缩多个文件到7z格式
             * @param inputFilePaths 输入文件路径列表
             * @param outputArchivePath 输出7z文件路径
             * @param compressionLevel 压缩级别 (0-9, 默认5)
             * @return 压缩结果
             */
            CompressionResult CompressFiles(const std::vector<std::string>& inputFilePaths,
                                          const std::string& outputArchivePath,
                                          int compressionLevel = 5);

            /**
             * @brief 压缩整个目录到7z格式
             * @param inputDirPath 输入目录路径
             * @param outputArchivePath 输出7z文件路径
             * @param compressionLevel 压缩级别 (0-9, 默认5)
             * @return 压缩结果
             */
            CompressionResult CompressDirectory(const std::string& inputDirPath,
                                              const std::string& outputArchivePath,
                                              int compressionLevel = 5);

            /**
             * @brief 解压7z文件
             * @param archivePath 7z文件路径
             * @param outputDirPath 输出目录路径
             * @return 解压结果
             */
            CompressionResult ExtractArchive(const std::string& archivePath,
                                           const std::string& outputDirPath);

        private:
            class Impl;
            std::unique_ptr<Impl> pImpl;
        };

        /**
         * @brief 二进制数据压缩工具类
         */
        class BinaryCompressor {
        public:
            /**
             * @brief 压缩算法类型
             */
            enum class Algorithm {
                ZLIB,       // zlib压缩
                GZIP,       // gzip压缩
                DEFLATE,    // deflate压缩
                LZ4,        // LZ4快速压缩
                ZSTD        // Zstandard压缩
            };

            BinaryCompressor();
            ~BinaryCompressor();

            /**
             * @brief 压缩二进制数据
             * @param inputData 输入数据
             * @param outputData 输出压缩数据
             * @param algorithm 压缩算法
             * @param compressionLevel 压缩级别 (算法相关)
             * @return 压缩结果
             */
            CompressionResult Compress(const std::vector<uint8_t>& inputData,
                                     std::vector<uint8_t>& outputData,
                                     Algorithm algorithm = Algorithm::ZLIB,
                                     int compressionLevel = 6);

            /**
             * @brief 解压二进制数据
             * @param compressedData 压缩数据
             * @param outputData 输出解压数据
             * @param algorithm 压缩算法
             * @return 解压结果
             */
            CompressionResult Decompress(const std::vector<uint8_t>& compressedData,
                                       std::vector<uint8_t>& outputData,
                                       Algorithm algorithm = Algorithm::ZLIB);

            /**
             * @brief 压缩字符串数据
             * @param inputString 输入字符串
             * @param outputData 输出压缩数据
             * @param algorithm 压缩算法
             * @param compressionLevel 压缩级别
             * @return 压缩结果
             */
            CompressionResult CompressString(const std::string& inputString,
                                           std::vector<uint8_t>& outputData,
                                           Algorithm algorithm = Algorithm::ZLIB,
                                           int compressionLevel = 6);

            /**
             * @brief 解压到字符串
             * @param compressedData 压缩数据
             * @param outputString 输出字符串
             * @param algorithm 压缩算法
             * @return 解压结果
             */
            CompressionResult DecompressToString(const std::vector<uint8_t>& compressedData,
                                               std::string& outputString,
                                               Algorithm algorithm = Algorithm::ZLIB);

        private:
            class Impl;
            std::unique_ptr<Impl> pImpl;
        };

        /**
         * @brief 压缩工具辅助函数
         */
        namespace Utils {
            /**
             * @brief 获取算法名称
             */
            std::string GetAlgorithmName(BinaryCompressor::Algorithm algorithm);

            /**
             * @brief 计算压缩比
             */
            double CalculateCompressionRatio(size_t originalSize, size_t compressedSize);

            /**
             * @brief 格式化文件大小
             */
            std::string FormatFileSize(size_t sizeInBytes);
        }
    }
}