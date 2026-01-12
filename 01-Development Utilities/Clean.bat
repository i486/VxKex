::
:: Prepares the VxKex source tree for distribution.
::
:: If you move this script somewhere else, update the "cd /D ..." line.
::

@echo off
cd /D %~dp0\..

del KexSetup_Debug.exe KexSetup_Release.exe

rmdir /S /Q ipch
del /A /F /S /Q *.user *.sdf

for /R /D %%f in (x64) do (
    if not "%%f"=="%cd%\02-Prebuilt DLLs\x64" (
        rmdir /S /Q "%%f"
    )
)

for /R /D %%f in (Debug) do (
    rmdir /S /Q "%%f"
)

for /R /D %%f in (Release) do (
    rmdir /S /Q "%%f"
)

pause