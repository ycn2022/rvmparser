#pragma once

// 平台检测和通用包含
#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
    #endif
    #include <windows.h>
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #include <unistd.h>
    #include <pthread.h>
    #include <dlfcn.h>
    #define PLATFORM_LINUX
#else
    #error "Unsupported platform"
#endif

// 通用C++标准库包含
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <variant>
#include <cstdint>
#include <atomic>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <cstdarg>

// 平台特定的类型定义和宏
#ifdef PLATFORM_WINDOWS
    // Windows特定定义
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
#elif defined(PLATFORM_LINUX)
    // Linux特定定义
    #include <cstring>
    #include <cerrno>
#endif
