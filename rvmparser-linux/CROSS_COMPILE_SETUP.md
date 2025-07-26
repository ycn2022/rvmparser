# 交叉编译环境设置指南

本文档介绍如何在Windows上设置交叉编译环境，以便编译Linux版本的rvmparser。

## 方法1：使用MSYS2（推荐）

### 1. 安装MSYS2
1. 从 https://www.msys2.org/ 下载并安装MSYS2
2. 安装完成后，打开MSYS2终端

### 2. 安装交叉编译工具链
```bash
# 更新包数据库
pacman -Syu

# 安装MinGW-w64交叉编译工具链
pacman -S mingw-w64-x86_64-toolchain

# 安装CMake和Make
pacman -S mingw-w64-x86_64-cmake
pacman -S mingw-w64-x86_64-make

# 可选：安装ARM64交叉编译工具链
pacman -S mingw-w64-x86_64-aarch64-linux-gnu-toolchain
```

### 3. 设置环境变量
将以下路径添加到Windows系统PATH环境变量：
```
C:\msys64\mingw64\bin
C:\msys64\usr\bin
```

### 4. 验证安装
打开Windows命令提示符或PowerShell，运行：
```cmd
x86_64-linux-gnu-gcc --version
cmake --version
```

## 方法2：使用预编译工具链

### 1. 下载MinGW-w64
从以下地址下载预编译的MinGW-w64工具链：
- https://www.mingw-w64.org/downloads/
- 推荐使用 winlibs 版本：https://winlibs.com/

### 2. 解压并设置PATH
1. 将下载的工具链解压到合适位置（如 `C:\mingw64`）
2. 将 `C:\mingw64\bin` 添加到系统PATH环境变量

### 3. 安装CMake
从 https://cmake.org/download/ 下载并安装CMake

## 方法3：使用Docker（无需本地工具链）

如果不想安装交叉编译工具链，可以使用Docker：

### 1. 安装Docker Desktop
从 https://www.docker.com/products/docker-desktop 下载并安装

### 2. 使用Docker构建
```bash
# 在项目根目录运行
./build-docker.sh
```

## 验证交叉编译环境

运行以下命令验证环境设置：

### Windows命令提示符
```cmd
build-cross-linux.bat
```

### PowerShell
```powershell
powershell -ExecutionPolicy Bypass -File build-cross-linux.ps1
```

## 常见问题

### 1. 找不到交叉编译器
**错误**: `x86_64-linux-gnu-gcc not found`
**解决**: 确保工具链已正确安装并添加到PATH环境变量

### 2. CMake配置失败
**错误**: CMake无法找到编译器
**解决**: 检查工具链文件路径，确保编译器可执行文件存在

### 3. 链接错误
**错误**: 链接时找不到库文件
**解决**: 检查工具链的库路径设置，可能需要调整工具链文件

### 4. 权限问题
**错误**: PowerShell执行策略限制
**解决**: 以管理员身份运行PowerShell，或使用 `-ExecutionPolicy Bypass` 参数

## 性能优化

### 并行编译
构建脚本会自动使用所有可用CPU核心进行并行编译：
- Windows: 使用 `%NUMBER_OF_PROCESSORS%` 环境变量
- Unix: 使用 `nproc` 或 `sysctl -n hw.ncpu` 命令

### 静态链接
工具链文件配置了静态链接选项，生成的Linux二进制文件不依赖外部库，可以在大多数Linux发行版上运行。

## 支持的目标平台

当前支持的交叉编译目标：
- **Linux x86_64**: 使用 `toolchain-linux-x64.cmake`
- **Linux ARM64**: 使用 `toolchain-linux-arm64.cmake`

可以根据需要添加更多目标平台的工具链文件。