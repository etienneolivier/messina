@echo off
REM Build script for Messina Windows VST3
REM Run this on a Windows machine with Visual Studio installed
REM Requires: Visual Studio 2022 (with "Desktop development with C++" workload)
REM          Node.js 18+, Python 3, CMake 3.22+

echo ============================================
echo Messina Windows VST3 Build Script
echo ============================================

echo [1/5] Setting up environment...
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found. Install CMake 3.22+ from https://cmake.org/download/
    pause
    exit /b 1
)

where node >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Node.js not found. Install Node.js 18+ from https://nodejs.org/
    pause
    exit /b 1
)

where python3 >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Python 3 not found. Install Python 3 from https://python.org/
    pause
    exit /b 1
)

echo [2/5] Building React UI...
cd ui
if not exist node_modules (
    npm install
)
npm run build
python3 inline_assets.py
cd ..

echo [3/5] Downloading JUCE...
if not exist Libs\JUCE (
    mkdir Libs
    cd Libs
    git clone --depth 1 --branch 8.0.12 https://github.com/juce-framework/JUCE.git JUCE
    cd ..
)

echo [4/5] Configuring CMake...
if exist build (
    rmdir /s /q build
)
cmake -B build -DCMAKE_BUILD_TYPE=Release -DJUCE_COPY_PLUGIN_AFTER_BUILD=OFF
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo [5/5] Building VST3 plugin...
cmake --build build --config Release --target Messina_VST3 -j
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build completed successfully!
echo ============================================
echo VST3 plugin location:
if exist build\Messina_artefacts\VST3\Messina.vst3 (
    echo   build\Messina_artefacts\VST3\Messina.vst3
    echo.
    echo To install:
    echo   Copy Messina.vst3 to:
    echo     C:\Program Files\VST3\ (system-wide)
    echo     C:\Users\%%USERNAME%%\AppData\Roaming\VST3\ (user)
    echo     Your DAW's VST3 folder
) else (
    echo   WARNING: VST3 bundle not found at expected location
    echo   Check build directory for the .vst3 folder
)

pause