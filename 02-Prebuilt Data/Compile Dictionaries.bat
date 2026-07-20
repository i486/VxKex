@echo off

REM Keep the PUSHD/POPD intact. Otherwise the MAKESFX.bat will break.
PUSHD %~dp0

REM
REM Regenerate KexMLS .bdi files.
REM This should be run automatically while building the KexSetup SFX.
REM

DEL KexDir\Globalization\Dictionaries\*.bdi 2>&1 >NUL
START /WAIT MLSBDIC.EXE /IN:Dictionaries /OUT:KexDir\Globalization\Dictionaries

POPD
EXIT /B %ErrorLevel%