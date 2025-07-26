#pragma once

#include <cstddef>
#include <thread>

namespace E5DZipUtils {

class SystemInfo {
public:
    // 获取CPU物理核心数
    static unsigned int getPhysicalCoreCount();
    
    // 获取可用内存大小（字节）
    static size_t getAvailableMemory();
    
    // 获取总内存大小（字节）
    static size_t getTotalMemory();
    
    // 根据文件大小和系统资源计算最优的块大小
    static size_t calculateOptimalChunkSize(size_t fileSize);
    
    // 根据文件大小和系统资源计算最优的线程数
    static unsigned int calculateOptimalThreadCount(size_t fileSize);
    
    // 根据文件大小推荐压缩级别
    static int recommendCompressionLevel(size_t fileSize, bool prioritizeSpeed = false);
};

} // namespace E5DZipUtils