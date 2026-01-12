@echo off
cd /D "%~dp0"

del Archive.7z >nul 2>nul
rd /s /q Archive >nul 2>nul

if /i "%1"=="/RELEASE" (
	SET DBGREL=Release
) ELSE (
	SET DBGREL=Debug
)

md Archive
md Archive\Core
md Archive\Core32
md Archive\Core64
md Archive\Kex32
md Archive\Kex64

REM
REM Bitness Agnostic Data
REM

copy ..\..\%DBGREL%\KexSetup.exe Archive\ >nul

copy "..\..\00-Documentation\Application Compatibility List.docx" Archive\Core\ >nul
copy "..\..\00-Documentation\Changelog.txt" Archive\Core\ >nul

REM
REM 32-bit Core
REM

copy ..\..\%DBGREL%\KexDll.dll Archive\Core32\ >nul
copy ..\..\%DBGREL%\KexShlEx.dll Archive\Core32\ >nul
copy ..\..\%DBGREL%\KexCfg.exe Archive\Core32\ >nul
copy ..\..\%DBGREL%\VxlView.exe Archive\Core32\ >nul
copy ..\..\%DBGREL%\CpiwBypa.dll Archive\Core32\ >nul
copy ..\..\%DBGREL%\VxKexLdr.exe Archive\Core32\ >nul

REM
REM 32-bit Extension
REM

copy ..\..\%DBGREL%\KxAdvapi.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxBase.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxCom.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxCrt.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxCryp.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxDx.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxMi.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxNet.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxNt.dll Archive\Kex32\ >nul
copy ..\..\%DBGREL%\KxUser.dll Archive\Kex32\ >nul

REM
REM 64-bit Core
REM

copy ..\..\x64\%DBGREL%\KexDll.dll Archive\Core64\ >nul
copy ..\..\x64\%DBGREL%\KexShlEx.dll Archive\Core64\ >nul
copy ..\..\x64\%DBGREL%\KexCfg.exe Archive\Core64\ >nul
copy ..\..\x64\%DBGREL%\VxlView.exe Archive\Core64\ >nul
copy ..\..\x64\%DBGREL%\CpiwBypa.dll Archive\Core64\ >nul
copy ..\..\x64\%DBGREL%\VxKexLdr.exe Archive\Core64\ >nul

REM
REM 64-bit Extension
REM

copy ..\..\x64\%DBGREL%\KxAdvapi.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxBase.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxCom.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxCrt.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxCryp.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxDx.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxMi.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxNet.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxNt.dll Archive\Kex64\ >nul
copy ..\..\x64\%DBGREL%\KxUser.dll Archive\Kex64\ >nul

REM
REM Prebuilt DLLs
REM

copy "..\..\02-Prebuilt DLLs\x86\*.dll" Archive\Kex32\ >nul
copy "..\..\02-Prebuilt DLLs\x64\*.dll" Archive\Kex64\ >nul

if %DBGREL%==Debug (
	REM Include PDBs as well

	copy ..\..\%DBGREL%\KexSetup.pdb Archive\ >nul

	copy ..\..\%DBGREL%\KexDll.pdb Archive\Core32\ >nul
	copy ..\..\%DBGREL%\KexShlEx.pdb Archive\Core32\ >nul
	copy ..\..\%DBGREL%\KexCfg.pdb Archive\Core32\ >nul
	copy ..\..\%DBGREL%\VxlView.pdb Archive\Core32\ >nul
	copy ..\..\%DBGREL%\CpiwBypa.pdb Archive\Core32\ >nul
	copy ..\..\%DBGREL%\VxKexLdr.pdb Archive\Core32\ >nul

	copy ..\..\%DBGREL%\KxAdvapi.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxBase.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxCom.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxCrt.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxCryp.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxDx.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxMi.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxNet.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxNt.pdb Archive\Kex32\ >nul
	copy ..\..\%DBGREL%\KxUser.pdb Archive\Kex32\ >nul

	copy ..\..\x64\%DBGREL%\KexDll.pdb Archive\Core64\ >nul
	copy ..\..\x64\%DBGREL%\KexShlEx.pdb Archive\Core64\ >nul
	copy ..\..\x64\%DBGREL%\KexCfg.pdb Archive\Core64\ >nul
	copy ..\..\x64\%DBGREL%\VxlView.pdb Archive\Core64\ >nul
	copy ..\..\x64\%DBGREL%\CpiwBypa.pdb Archive\Core64\ >nul
	copy ..\..\x64\%DBGREL%\VxKexLdr.pdb Archive\Core64\ >nul

	copy ..\..\x64\%DBGREL%\KxAdvapi.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxBase.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxCom.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxCrt.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxCryp.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxDx.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxMi.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxNet.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxNt.pdb Archive\Kex64\ >nul
	copy ..\..\x64\%DBGREL%\KxUser.pdb Archive\Kex64\ >nul

	copy "..\..\02-Prebuilt DLLs\x86\*.pdb" Archive\Kex32\ >nul
	copy "..\..\02-Prebuilt DLLs\x64\*.pdb" Archive\Kex64\ >nul
)

copy /y 7zS2.sfx ..\..\KexSetup_%DBGREL%.exe >nul


REM vtrplnt copies the version resources from the original kexsetup into
REM the packed SFX version.
REM It has to be run now because otherwise it will clobber the appended
REM 7zip data.

vtrplnt.exe /%DBGREL%

REM ===========================================================================
REM FOR FINAL RELEASE: switch to the upper command for higher compression
REM ===========================================================================

REM 7zr a -y Archive.7z .\Archive\* -mmt1 -mx9 -m0=LZMA:d32
7zr a -y Archive.7z .\Archive\* -mmt1 -mx1 -m0=LZMA:d32

if %errorlevel% neq 0 (
	pause
	exit %errorlevel%
)

rd /s /q Archive

copy /b ..\..\KexSetup_%DBGREL%.exe + Archive.7z "..\..\KexSetup_%DBGREL%.exe" >nul

if %errorlevel% neq 0 (
	pause
	exit %errorlevel%
)

del Archive.7z

REM Now auto-increment the build number, but only for debug builds.

if %DBGREL%==Debug (
	"..\..\01-Development Utilities\vautogen\vautogen.exe"

	if %errorlevel% neq 0 (
		pause
		exit %errorlevel%
	)
)