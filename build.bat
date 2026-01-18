@echo off

setlocal enabledelayedexpansion

set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release

set DCMTK_VERSION=DCMTK-3.6.8
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

echo ========================================
echo DICOM Viewer Build Script
echo ========================================
echo.

REM Check for required tools
echo [*] Checking dependencies...

where git >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] git is required but not installed. Aborting.
    echo Install from: https://git-scm.com/download/win
    exit /b 1
)

where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] cmake is required but not installed. Aborting.
    echo Install from: https://cmake.org/download/
    exit /b 1
)

REM Check for Visual Studio
where cl.exe >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [WARNING] Visual Studio compiler not found in PATH.
    echo Attempting to find Visual Studio...
    
    REM Try to find and run vcvarsall.bat
    set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist %VSWHERE% (
        for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set VS_PATH=%%i
        )
        
        if exist "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" (
            echo [*] Found Visual Studio at: !VS_PATH!
            echo [*] Setting up Visual Studio environment...
            call "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" x64
        ) else (
            echo [ERROR] Visual Studio found but vcvarsall.bat not found.
            exit /b 1
        )
    ) else (
        echo [ERROR] Visual Studio not found. Please install Visual Studio 2019 or later with C++ tools.
        exit /b 1
    )
)

echo [*] All required dependencies found!
echo.

REM Setup DCMTK
echo [*] Setting up DCMTK...

if not exist "third_party" (
    echo [*] Creating third_party directory...
    mkdir third_party
)

if not exist "third_party\dcmtk" (
    echo [*] Cloning DCMTK repository...
    cd third_party
    git clone --depth 1 --branch %DCMTK_VERSION% https://github.com/DCMTK/dcmtk.git
    cd ..
    echo [*] DCMTK cloned successfully!
) else (
    echo [*] DCMTK already exists in third_party\
)

REM Check if DCMTK needs to be built
if not exist "third_party\dcmtk\install" (
    echo [*] DCMTK will be built during CMake configuration...
) else (
    echo [*] DCMTK already built
)

echo.

REM Create build directory
echo [*] Creating build directory...
set BUILD_DIR=build-%BUILD_TYPE%

if exist "%BUILD_DIR%" (
    echo [WARNING] Build directory exists. Cleaning...
    rmdir /s /q "%BUILD_DIR%"
)

mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo [*] Build directory: %BUILD_DIR%
echo [*] Build type: %BUILD_TYPE%
echo.

REM Detect number of processors for parallel build
if "%NUMBER_OF_PROCESSORS%"=="" (
    set NUM_JOBS=4
) else (
    set NUM_JOBS=%NUMBER_OF_PROCESSORS%
)

echo [*] Using %NUM_JOBS% parallel jobs
echo.

REM Configure
echo [*] Configuring CMake...
cmake .. ^
    -G "Visual Studio 17 2022" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE%

if %ERRORLEVEL% neq 0 (
    REM Try with Visual Studio 2019 if 2022 fails
    echo [*] Trying Visual Studio 2019...
    cmake .. ^
        -G "Visual Studio 16 2019" ^
        -A x64 ^
        -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
    
    if %ERRORLEVEL% neq 0 (
        echo [ERROR] CMake configuration failed!
        cd ..
        exit /b 1
    )
)

echo.

REM Build
echo [*] Building DICOM Viewer...
cmake --build . --config %BUILD_TYPE% --parallel %NUM_JOBS%

if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    cd ..
    exit /b 1
)

echo.
echo [*] Build completed successfully!
echo.

REM Print binary location
set BINARY_PATH=%cd%\%BUILD_TYPE%\dicom_viewer.exe

if exist "%BINARY_PATH%" (
    echo ========================================
    echo Build Summary
    echo ========================================
    echo Build Type:   %BUILD_TYPE%
    echo Binary:       %BINARY_PATH%
    echo.
    echo To run the application:
    echo   cd %BUILD_DIR%
    echo   %BUILD_TYPE%\dicom_viewer.exe
    echo.
    echo Or directly:
    echo   "%BINARY_PATH%"
    echo ========================================
) else (
    echo [WARNING] Could not find executable. Check build output above.
)

cd ..

echo.
echo Build script completed.
pause