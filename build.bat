@echo off
setlocal

REM =====================================
REM Build type
REM =====================================

set BUILD_TYPE=Release
if "%~1"=="" (
    echo No build type specified, defaulting to Release
) else (
    set BUILD_TYPE=%~1
)

REM Validate build type
if /I not "%BUILD_TYPE%"=="Release" if /I not "%BUILD_TYPE%"=="Debug" (
    echo Error: Invalid build type "%BUILD_TYPE%"
    echo Usage: %~nx0 [Debug^|Release]
    exit /b 1
)

REM =====================================
REM Build directory
REM =====================================

set BUILD_DIR=build

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cd "%BUILD_DIR%"

REM =====================================
REM Configure
REM =====================================

echo === Configuring project (%BUILD_TYPE%) ===
cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE% || goto :error

REM =====================================
REM Build
REM =====================================

echo === Building (%BUILD_TYPE%) ===
cmake --build . --config %BUILD_TYPE% || goto :error

echo.
echo === %BUILD_TYPE% build successful! ===
goto :end

:error
echo.
echo *** %BUILD_TYPE% build failed! ***
exit /b 1

:end
cd ..
endlocal
