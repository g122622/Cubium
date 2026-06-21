@echo off
setlocal enabledelayedexpansion

:: ============================================================
:: Cubium - Build Environment Setup Script
:: Automatically configures Visual Studio developer environment
:: before running CMake commands. Works in any terminal (cmd,
:: PowerShell, Git Bash via cmd //c, CI pipelines).
::
:: Usage:
::   configure.bat                  - Configure with relwithdebinfo preset
::   configure.bat build            - Configure + build
::   configure.bat <cmake args...>  - Run cmake with VS env (any args)
:: ============================================================

:: Call VsDevCmd to set up VS environment
call "%~dp0vsenv.bat" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to set up Visual Studio developer environment.
    echo Make sure Visual Studio is installed at:
    echo   D:\Program Files\Microsoft Visual Studio\18\Community
    exit /b 1
)

:: Default: configure with the project's standard preset
if "%~1"=="" (
    echo Configuring with preset windows-clang-relwithdebinfo...
    cmake --preset windows-clang-relwithdebinfo
    exit /b %errorlevel%
)

:: "build" shortcut: configure then build
:: NOTE: We use goto instead of an if-block because set /a expressions
:: containing * (multiplication) fail inside parenthesized blocks with
:: delayed expansion enabled in cmd.exe.
if /i not "%~1"=="build" goto :pass_through

echo Configuring with preset windows-clang-relwithdebinfo...
cmake --preset windows-clang-relwithdebinfo
if errorlevel 1 exit /b %errorlevel%
echo.
echo Building...

:: Record start time (centiseconds since midnight)
set START_TIME=%time%

cmake --build --preset windows-clang-relwithdebinfo
set BUILD_EXIT=!errorlevel!

set END_TIME=%time%

:: Parse start time
for /f "tokens=1-3 delims=:.," %%a in ("!START_TIME!") do (
    set /a START_H=%%a, START_M=1%%b-100, START_S=1%%c-100
)
:: Parse end time
for /f "tokens=1-3 delims=:.," %%a in ("!END_TIME!") do (
    set /a END_H=%%a, END_M=1%%b-100, END_S=1%%c-100
)

:: Calculate total seconds
set /a ELAPSED=((END_H - START_H) * 3600) + ((END_M - START_M) * 60) + (END_S - START_S)
set /a DURATION_M=ELAPSED / 60
set /a DURATION_S=ELAPSED %% 60

:: Pad with leading zero
if !DURATION_M! LSS 10 set DURATION_M=0!DURATION_M!
if !DURATION_S! LSS 10 set DURATION_S=0!DURATION_S!

echo.
echo === Build Summary ===
echo   Duration:  !DURATION_M!:!DURATION_S!
echo   Exit code: !BUILD_EXIT!
exit /b !BUILD_EXIT!

:pass_through
:: Otherwise, pass all args through to cmake
cmake %*
exit /b %errorlevel%
