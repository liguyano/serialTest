@echo off
setlocal

cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
    echo [ERROR] CMake was not found in PATH.
    exit /b 1
)

if not exist "wxWidgets\CMakeLists.txt" (
    echo [ERROR] wxWidgets is missing.
    echo Run setup_wxwidgets.bat first.
    exit /b 1
)

echo [INFO] Configuring Visual Studio 2022 x64 build...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1

echo [INFO] Running helper tests...
cmake --build build --config Debug --target SerialHelpersTests
if errorlevel 1 exit /b 1
ctest --test-dir build -C Debug --output-on-failure
if errorlevel 1 exit /b 1

echo [INFO] Building SerialTest Release...
cmake --build build --config Release --target SerialTest
if errorlevel 1 exit /b 1

echo.
echo [OK] Build completed.
echo Executable: "%CD%\build\Release\SerialTest.exe"
exit /b 0
