# ============================================================
# Cubium - Build Environment Setup (PowerShell)
# Automatically configures Visual Studio developer environment
# before running CMake commands.
#
# Usage:
#   .\configure.ps1                         - Configure with relwithdebinfo preset
#   .\configure.ps1 -Build                  - Configure + build
#   .\configure.ps1 -CmakeArgs @("--preset","windows-clang-debug")
#                                           - Run cmake with custom args
# ============================================================

param(
    [switch]$Build,
    [string[]]$CmakeArgs
)

$ErrorActionPreference = "Stop"

# --- Set up VS developer environment ---
$VSBasedir = "D:\Program Files\Microsoft Visual Studio\18\Community"
$VsDevCmd = Join-Path $VSBasedir "Common7\Tools\VsDevCmd.bat"

if (-not (Test-Path $VsDevCmd)) {
    Write-Error "VsDevCmd.bat not found at: $VsDevCmd"
    Write-Error "Please verify your Visual Studio installation path."
    exit 1
}

# Capture env vars from VsDevCmd by running it in a child cmd and exporting the diff
$tempScript = [System.IO.Path]::GetTempFileName() + ".bat"
$envFile    = [System.IO.Path]::GetTempFileName()

@"
@echo off
call "$VsDevCmd" -arch=amd64 -host_arch=amd64 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set > "$envFile"
exit /b 0
"@ | Set-Content $tempScript

$proc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c `"$tempScript`"" -NoNewWindow -Wait -PassThru
if ($proc.ExitCode -ne 0) {
    Write-Error "Failed to set up Visual Studio developer environment."
    Remove-Item $tempScript, $envFile -ErrorAction SilentlyContinue
    exit 1
}

# Parse the exported env vars and set them in the current PowerShell session
$beforeKeys = @() + (Get-ChildItem env:).Name
$envContent = Get-Content $envFile
foreach ($line in $envContent) {
    if ($line -match '^([^=]+)=(.*)$') {
        $key = $Matches[1]
        $val = $Matches[2]
        # Skip problematic CI/terminal vars that shouldn't be overwritten
        if ($key -in @('PROMPT', 'COMSPEC', 'PATHEXT', 'TEMP', 'TMP', 'HOME', 'USERPROFILE')) { continue }
        [System.Environment]::SetEnvironmentVariable($key, $val, 'Process')
    }
}

Remove-Item $tempScript, $envFile -ErrorAction SilentlyContinue
Write-Host "Visual Studio developer environment configured." -ForegroundColor Green

# --- Run CMake ---
$Preset = "windows-clang-relwithdebinfo"

if ($Build) {
    Write-Host "Configuring with preset $Preset..."
    & cmake --preset $Preset
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host ""
    Write-Host "Building..."
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    & cmake --build --preset $Preset
    $buildExit = $LASTEXITCODE
    $sw.Stop()
    $duration = "{0:00}:{1:00}" -f [math]::Floor($sw.Elapsed.TotalMinutes), $sw.Elapsed.Seconds
    Write-Host ""
    Write-Host "=== Build Summary ===" -ForegroundColor Cyan
    Write-Host "  Duration:  $duration" -ForegroundColor Cyan
    Write-Host "  Exit code: $buildExit" -ForegroundColor Cyan
    exit $buildExit
}

if ($CmakeArgs) {
    Write-Host "Running cmake with args: $($CmakeArgs -join ' ')"
    & cmake @CmakeArgs
    exit $LASTEXITCODE
}

# Default: configure only
Write-Host "Configuring with preset $Preset..."
& cmake --preset $Preset
exit $LASTEXITCODE
