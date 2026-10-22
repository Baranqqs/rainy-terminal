@echo off
setlocal

where g++ >nul 2>nul
if errorlevel 1 (
    echo g++ was not found. Install MinGW-w64 and try again.
    exit /b 1
)

g++ -std=c++17 -O2 -Wall -Wextra -pedantic ^
    src\main.cpp ^
    src\banner\banner.cpp ^
    src\rain\rain.cpp ^
    src\terminal\terminal.cpp ^
    src\terminal\startup.cpp ^
    -o RainyTerminal.exe

if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

echo Built RainyTerminal.exe
endlocal
