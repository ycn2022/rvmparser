# Cross-compile script for building Linux binaries on Windows using PowerShell
# Requires MinGW-w64 cross-compilation toolchain

Write-Host "=== Cross-compiling rvmparser for Linux on Windows ===" -ForegroundColor Green

# Check if cross-compilation toolchain is available
try {
    $null = Get-Command "x86_64-linux-gnu-gcc" -ErrorAction Stop
    Write-Host "Found x86_64-linux-gnu-gcc toolchain" -ForegroundColor Green
} catch {
    Write-Host "Error: x86_64-linux-gnu-gcc not found in PATH" -ForegroundColor Red
    Write-Host "Please install MinGW-w64 cross-compilation toolchain:" -ForegroundColor Yellow
    Write-Host "  - MSYS2: pacman -S mingw-w64-x86_64-toolchain" -ForegroundColor Yellow
    Write-Host "  - Or download from: https://www.mingw-w64.org/downloads/" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Function to build for a specific target
function Build-Target {
    param(
        [string]$TargetName,
        [string]$ToolchainFile,
        [string]$BuildDir
    )
    
    Write-Host "Building for $TargetName..." -ForegroundColor Cyan
    
    # Create build directory
    if (Test-Path $BuildDir) {
        Remove-Item -Recurse -Force $BuildDir
    }
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
    Set-Location $BuildDir
    
    # Configure
    Write-Host "Configuring for $TargetName..." -ForegroundColor Yellow
    & cmake .. -G "MinGW Makefiles" `
        -DCMAKE_TOOLCHAIN_FILE="../$ToolchainFile" `
        -DCMAKE_BUILD_TYPE=Release
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Configuration failed for $TargetName!" -ForegroundColor Red
        Set-Location ..
        return $false
    }
    
    # Build
    Write-Host "Building $TargetName..." -ForegroundColor Yellow
    & cmake --build . --config Release -j $env:NUMBER_OF_PROCESSORS
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Build failed for $TargetName!" -ForegroundColor Red
        Set-Location ..
        return $false
    }
    
    Set-Location ..
    Write-Host "$TargetName build completed successfully!" -ForegroundColor Green
    return $true
}

# Build Linux x64
$success = Build-Target -TargetName "Linux x64" -ToolchainFile "cmake/toolchain-linux-x64.cmake" -BuildDir "build-linux-x64"

if ($success) {
    Write-Host "=== Cross-compilation completed successfully ===" -ForegroundColor Green
    Write-Host "Linux x64 binary: build-linux-x64/rvmparser" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "To test on Linux, copy the binary to a Linux system and run:" -ForegroundColor Yellow
    Write-Host "  ./rvmparser --help" -ForegroundColor Yellow
} else {
    Write-Host "=== Cross-compilation failed ===" -ForegroundColor Red
}

Read-Host "Press Enter to exit"