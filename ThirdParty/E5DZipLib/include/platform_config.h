#pragma once

// 平台检测
#ifdef _WIN32
    #define PLATFORM_WINDOWS 1
    #define PLATFORM_LINUX 0
#elif defined(__linux__)
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 1
#else
    #define PLATFORM_WINDOWS 0
    #define PLATFORM_LINUX 0
    #error "Unsupported platform"
#endif

// 编译器检测
#ifdef _MSC_VER
    #define COMPILER_MSVC 1
    #define COMPILER_GCC 0
    #define COMPILER_CLANG 0
#elif defined(__GNUC__) && !defined(__clang__)
    #define COMPILER_MSVC 0
    #define COMPILER_GCC 1
    #define COMPILER_CLANG 0
#elif defined(__clang__)
    #define COMPILER_MSVC 0
    #define COMPILER_GCC 0
    #define COMPILER_CLANG 1
#else
    #define COMPILER_MSVC 0
    #define COMPILER_GCC 0
    #define COMPILER_CLANG 0
#endif

// 平台特定包含
#if PLATFORM_WINDOWS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <objbase.h>
    #include <initguid.h>
#endif

// 导出宏定义
#if PLATFORM_WINDOWS
    #ifdef E5DZIPLIB_EXPORTS
        #define E5DZIPLIB_API __declspec(dllexport)
    #else
        #define E5DZIPLIB_API __declspec(dllimport)
    #endif
#else
    #define E5DZIPLIB_API __attribute__((visibility("default")))
#endif

// 调用约定
#if PLATFORM_WINDOWS
    #define E5DZIPLIB_CALL __stdcall
#else
    #define E5DZIPLIB_CALL
#endif

// 内联宏
#if COMPILER_MSVC
    #define E5DZIPLIB_INLINE __forceinline
#elif COMPILER_GCC || COMPILER_CLANG
    #define E5DZIPLIB_INLINE __attribute__((always_inline)) inline
#else
    #define E5DZIPLIB_INLINE inline
#endif

// 字节序检测
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        #define E5DZIPLIB_LITTLE_ENDIAN 1
        #define E5DZIPLIB_BIG_ENDIAN 0
    #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        #define E5DZIPLIB_LITTLE_ENDIAN 0
        #define E5DZIPLIB_BIG_ENDIAN 1
    #endif
#elif PLATFORM_WINDOWS
    // Windows通常是小端序
    #define E5DZIPLIB_LITTLE_ENDIAN 1
    #define E5DZIPLIB_BIG_ENDIAN 0
#else
    // 默认假设小端序
    #define E5DZIPLIB_LITTLE_ENDIAN 1
    #define E5DZIPLIB_BIG_ENDIAN 0
#endif

// 调试宏
#ifdef _DEBUG
    #define E5DZIPLIB_DEBUG 1
#else
    #define E5DZIPLIB_DEBUG 0
#endif

// 版本信息
#define E5DZIPLIB_VERSION_MAJOR 1
#define E5DZIPLIB_VERSION_MINOR 0
#define E5DZIPLIB_VERSION_PATCH 0
#define E5DZIPLIB_VERSION_STRING "1.0.0"