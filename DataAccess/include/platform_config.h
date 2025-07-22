#pragma once

#include <string>

// 跨平台配置头文件

#ifdef _WIN32
    // Windows平台配置
    #define PLATFORM_WINDOWS
    #define PATH_SEPARATOR '\\'
    #define PATH_SEPARATOR_STR "\\"
    
    // Windows特定的函数映射
    #define strcasecmp _stricmp
    #define strncasecmp _strnicmp
    
#elif defined(__linux__)
    // Linux平台配置
    #define PLATFORM_LINUX
    #define PATH_SEPARATOR '/'
    #define PATH_SEPARATOR_STR "/"
    
#else
    #error "Unsupported platform"
#endif

// 通用宏定义
#ifndef UNUSED
    #define UNUSED(x) ((void)(x))
#endif

// 日志级别定义
enum LogLevel {
    LOG_INFO = 0,
    LOG_WARNING = 1,
    LOG_ERROR = 2
};

// 跨平台的日志函数声明
void platform_logger(unsigned level, const char* msg, ...);

// 跨平台的文件路径处理函数
std::string normalize_path(const std::string& path);
bool file_exists(const std::string& path);
bool create_directory_recursive(const std::string& path);