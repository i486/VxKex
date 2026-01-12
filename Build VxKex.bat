@echo off
rem Set environment for 64-bit build
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

rem Build the solution for x64
msbuild "VxKex.sln" /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=Windows7.1SDK
if errorlevel 1 (
    echo Failed to build x64 Debug configuration
    exit /b 1
)

rem Set environment for 32-bit build
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x86

rem Build the solution for Win32
msbuild "VxKex.sln" /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=Windows7.1SDK
if errorlevel 1 (
    echo Failed to build Win32 Debug configuration
    exit /b 1
)

rem Set environment for 64-bit build
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x64

rem Build the solution for x64
msbuild "VxKex.sln" /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=Windows7.1SDK
if errorlevel 1 (
    echo Failed to build x64 Release configuration
    exit /b 1
)


rem Set environment for 32-bit build
call "C:\Program Files\Microsoft SDKs\Windows\v7.1\Bin\SetEnv.cmd" /x86

rem Build the solution for Win32
msbuild "VxKex.sln" /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=Windows7.1SDK
if errorlevel 1 (
    echo Failed to build Win32 Release configuration
    exit /b 1
)

echo Build completed successfully for both Win32 and x64 configurations
exit /b 0
