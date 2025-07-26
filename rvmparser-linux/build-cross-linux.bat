@echo off
REM Cross-compile script for building Linux binaries on Windows
REM Requires MinGW-w64 cross-compilation toolchain

echo === Cross-compiling rvmparser for Linux on Windows ===

REM Check if cross-compilation toolchain is available
where x86_64-linux-gnu-gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: x86_64-linux-gnu-gcc not found in PATH
    echo Please install MinGW-w64 cross-compilation toolchain
    echo You can install it via:
    echo   - MSYS2: pacman -S mingw-w64-x86_64-toolchain
    echo   - Or download from: https://www.mingw-w64.org/downloads/
    pause
    exit /b 1
)

REM Create build directory for Linux x64
if exist build-linux-x64 rmdir /s /q build-linux-x64
mkdir build-linux-x64
cd build-linux-x64

echo Configuring for Linux x64...
cmake .. -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-linux-x64.cmake ^
    -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo Configuration failed!
    cd ..
    pause
    exit /b 1
)

echo Building for Linux x64...
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo === Cross-compilation completed successfully ===
echo Linux x64 binary: build-linux-x64\rvmparser
echo.
echo To test on Linux, copy the binary to a Linux system and run:
echo   ./rvmparser --help

pause