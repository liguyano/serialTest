@echo off
setlocal

cd /d "%~dp0"

for %%T in (cmake ctest gcc g++ mingw32-make windres) do (
    where %%T >nul 2>nul
    if errorlevel 1 (
        echo [ERROR] %%T was not found in PATH.
        echo Install CMake and MinGW-w64, add their bin folders to PATH, and reopen the terminal.
        exit /b 1
    )
)

if not exist "wxwidget\CMakeLists.txt" (
    echo [ERROR] wxWidgets is missing.
    echo Run setup_wxwidgets.bat first.
    exit /b 1
)

echo [INFO] Configuring MinGW-w64 GCC Release build...
cmake -S . -B build-gcc -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if errorlevel 1 exit /b 1

echo [INFO] Building helper tests...
cmake --build build-gcc --target SerialHelpersTests --parallel 2
if errorlevel 1 exit /b 1

echo [INFO] Running helper tests...
ctest --test-dir build-gcc --output-on-failure
if errorlevel 1 exit /b 1

echo [INFO] Building SerialTest Release...
cmake --build build-gcc --target SerialTest --parallel 2
if errorlevel 1 exit /b 1

echo.
echo [OK] Build completed.
echo Executable: "%CD%\build-gcc\SerialTest.exe"
echo Keep the MinGW-w64 bin folder in PATH when running the application.
exit /b 0
