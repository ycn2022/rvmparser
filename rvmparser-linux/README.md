# rvmparser-crossplatform

跨平台的AVEVA PDMS RVM文件解析器，支持Windows、Linux和macOS，使用CMake构建系统。

## 项目描述

这是一个跨平台的RVM文件解析器，基于原始的rvmparser项目，使用CMake作为构建系统，支持现代C++20标准。支持本地编译和交叉编译，可以在Windows上编译Linux版本。

## 功能特性

- 读取AVEVA PDMS二进制RVM和属性文件
- 匹配相邻几何体以避免内部盖子
- 使用指定误差容限对原始形状进行三角剖分
- 支持多种输出格式：
  - Wavefront OBJ文件
  - GLTF/GLB文件
  - JSON属性文件
  - .rev文本文件

## 系统要求

### 基本要求
- CMake 3.16或更高版本
- 支持C++20的编译器：
  - Windows: Visual Studio 2019+ 或 MinGW-w64
  - Linux: GCC 9.0+ 或 Clang 10.0+
  - macOS: Xcode 12+ 或 Clang 10.0+

### 交叉编译要求（Windows → Linux）
- MinGW-w64交叉编译工具链
- 可通过MSYS2安装：`pacman -S mingw-w64-x86_64-toolchain`

## 构建说明

### 本地构建

#### Linux/macOS
```bash
chmod +x build.sh
./build.sh
```

#### Windows
```batch
# 使用批处理脚本
build-windows.bat

# 或使用PowerShell
powershell -ExecutionPolicy Bypass -File build-cross-linux.ps1
```

### 交叉编译（Windows → Linux）

#### 使用批处理脚本
```batch
build-cross-linux.bat
```

#### 使用PowerShell脚本
```powershell
powershell -ExecutionPolicy Bypass -File build-cross-linux.ps1
```

#### 手动交叉编译
```bash
mkdir build-linux-x64
cd build-linux-x64
cmake .. -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-x64.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### Docker构建
```bash
chmod +x build-docker.sh
./build-docker.sh
```

### 手动构建

#### 本地构建
```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)  # Linux/macOS
# 或
cmake --build . --config Release  # 跨平台
```

#### 调试构建
```bash
mkdir build-debug
cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## 使用方法

```bash
./build/rvmparser [options] files

# 示例
./build/rvmparser --output-obj=output --tolerance=0.1 input.rvm input.att
```

## 安装

```bash
cd build
sudo make install
```

默认安装到 `/usr/local/bin/rvmparser`

## 项目结构

```
rvmparser-crossplatform/
├── CMakeLists.txt              # CMake配置文件
├── build.sh                   # Unix构建脚本
├── build-windows.bat          # Windows本地构建脚本
├── build-cross-linux.bat      # Windows交叉编译脚本（批处理）
├── build-cross-linux.ps1      # Windows交叉编译脚本（PowerShell）
├── build-docker.sh            # Docker构建脚本
├── Dockerfile                 # Docker构建配置
├── README.md                  # 项目说明
├── cmake/                     # CMake工具链文件
│   ├── toolchain-linux-x64.cmake    # Linux x64交叉编译工具链
│   └── toolchain-linux-arm64.cmake  # Linux ARM64交叉编译工具链
├── src/                       # 源代码目录
│   ├── main.cpp              # 主程序入口
│   ├── Parser.h              # 解析器头文件
│   └── ...                   # 其他源文件
└── libs/                      # 第三方库
    ├── rapidjson/            # JSON库
    └── libtess2/             # 三角剖分库
```

## 支持的平台和架构

### 本地编译
- **Windows**: x86, x64 (Visual Studio, MinGW)
- **Linux**: x86_64, ARM64 (GCC, Clang)
- **macOS**: x86_64, ARM64 (Xcode, Clang)

### 交叉编译
- **Windows → Linux**: x86_64, ARM64
- **Docker**: 支持多架构构建

## 构建输出

不同构建方式的输出位置：
- 本地构建: `build/rvmparser` (Unix) 或 `build/Release/rvmparser.exe` (Windows)
- 交叉编译: `build-linux-x64/rvmparser`
- Docker构建: `dist/rvmparser-linux-x64`, `dist/rvmparser-linux-arm64`

## 依赖库

- **rapidjson**: 用于JSON输出
- **libtess2**: 用于复杂多边形的三角剖分

## 许可证

本项目基于MIT许可证发布。