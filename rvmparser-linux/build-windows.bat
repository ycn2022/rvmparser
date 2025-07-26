@echo off
REM Native Windows build script for rvmparser

echo === Building rvmparser for Windows ===

REM Create build directory
if exist build-windows rmdir /s /q build-windows
mkdir build-windows
cd build-windows

echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

if %errorlevel% neq 0 (
    echo Configuration failed!
    cd ..
    pause
    exit /b 1
)

echo Building...
cmake --build . --config Release -j %NUMBER_OF_PROCESSORS%

if %errorlevel% neq 0 (
    echo Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

echo === Build completed successfully ===
echo Executable location: build-windows\Release\rvmparser.exe

REM Copy required DLL files
echo Copying required DLL files...
if exist ThirdParty\E5DZipUtils\bin\Release\E5DZipUtils.dll (
    copy ThirdParty\E5DZipUtils\bin\Release\E5DZipUtils.dll build-windows\Release\ >nul
    echo Copied E5DZipUtils.dll
)
if exist ThirdParty\E5DZipUtils\bin\Release\zstd.dll (
    copy ThirdParty\E5DZipUtils\bin\Release\zstd.dll build-windows\Release\ >nul
    echo Copied zstd.dll
)

robocopy ThirdParty\copies\ build-windows\Release\ /E

REM Test if executable was created and works
if exist build-windows\Release\rvmparser.exe (
    echo Testing executable...
    build-windows\Release\rvmparser.exe --help >nul 2>&1
    if %errorlevel% equ 0 (
        echo Build verification: OK - Executable runs successfully
    ) else (
        echo Build verification: WARNING - Executable created but may have runtime issues
    )
) else if exist build-windows\rvmparser.exe (
    echo Testing executable...
    build-windows\rvmparser.exe --help >nul 2>&1
    if %errorlevel% equ 0 (
        echo Build verification: OK - Executable runs successfully
    ) else (
        echo Build verification: WARNING - Executable created but may have runtime issues
    )
) else (
    echo Error: Executable not found!
    pause
    exit /b 1
)

