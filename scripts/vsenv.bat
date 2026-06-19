@echo off
:: ============================================================
:: VsDevCmd wrapper - Sets up Visual Studio developer environment
:: This script calls VsDevCmd.bat silently and can be used by
:: other scripts (via `call vsenv.bat`) to get VS env vars.
::
REM Can also be sourced in PowerShell:
REM   cmd /c '"D:\path\vsenv.bat" && set' | ForEach-Object { ... }
:: ============================================================

set "VSBASE=D:\Program Files\Microsoft Visual Studio\18\Community"
set "VSDEVCMD=%VSBASE%\Common7\Tools\VsDevCmd.bat"

if not exist "%VSDEVCMD%" (
    echo ERROR: VsDevCmd.bat not found at: %VSDEVCMD%
    echo Please verify your Visual Studio installation path.
    exit /b 1
)

call "%VSDEVCMD%" -arch=amd64 -host_arch=amd64 -no_logo
exit /b %errorlevel%
