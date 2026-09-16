@echo off
setlocal

cd /d "%~dp0"

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Git was not found in PATH.
    echo Install Git for Windows, reopen the terminal, and run this script again.
    exit /b 1
)

if exist "wxWidgets\CMakeLists.txt" (
    echo [OK] wxWidgets already exists at "%CD%\wxWidgets".
    exit /b 0
)

if exist "wxWidgets" (
    echo [ERROR] A wxWidgets folder exists, but it does not look like a complete wxWidgets checkout.
    echo Remove or rename that folder, then run this script again.
    exit /b 1
)

echo [INFO] Downloading wxWidgets v3.2.8.1...
git clone --branch v3.2.8.1 --depth 1 --recurse-submodules --shallow-submodules https://github.com/wxWidgets/wxWidgets.git wxWidgets
if errorlevel 1 (
    echo [ERROR] wxWidgets download failed.
    exit /b 1
)

echo [OK] wxWidgets is ready in "%CD%\wxWidgets".
exit /b 0
