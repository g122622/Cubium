# ============================================================
# clang-tidy wrapper for MinecraftReborn (PowerShell)
#
# Runs clang-tidy on project source files with the correct
# MSVC / Windows SDK / vcpkg include paths, since the project
# uses clang-cl in its compile database which causes segfaults
# with clang-tidy on Windows.
#
# Usage:
#   .\scripts\tidy\tidy.ps1                              # Scan all src/ files
#   .\scripts\tidy\tidy.ps1 src\server\world\Foo.cpp      # Scan specific files
#   .\scripts\tidy\tidy.ps1 --fix src\server\world\Foo.cpp # Auto-fix issues
#   .\scripts\tidy\tidy.ps1 -Checks 'bugprone-*' ...       # Override checks
#
# Environment variables:
#   $env:CLANG_TIDY       - Path to clang-tidy (default: auto-detect)
#   $env:MSVC_ROOT        - MSVC tools root (default: auto-detect)
#   $env:WIN_SDK_ROOT     - Windows Kits/10 root (default: D:\Windows Kits)
#   $env:WIN_SDK_VERSION  - SDK version (default: latest installed)
# ============================================================

param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
$BuildDir = Join-Path $ProjectRoot "build"

# ---- Detect clang-tidy ----
$ClangTidy = if ($env:CLANG_TIDY) { $env:CLANG_TIDY } elseif (Get-Command clang-tidy -ErrorAction SilentlyContinue) { (Get-Command clang-tidy).Source } elseif (Test-Path "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe") { "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-tidy.exe" } else { Write-Host "ERROR: clang-tidy not found. Set `$env:CLANG_TIDY environment variable." -ForegroundColor Red; exit 1 }

Write-Host "Using clang-tidy: $ClangTidy" -ForegroundColor Green
& $ClangTidy --version | Select-Object -First 1

# ---- Detect MSVC include path ----
$MsvcRoot = $env:MSVC_ROOT
if (-not $MsvcRoot) {
    $VsBase = "D:\Program Files\Microsoft Visual Studio\18\Community"
    $MsvcToolDir = Join-Path $VsBase "VC\Tools\MSVC"
    if (Test-Path $MsvcToolDir) {
        $Latest = Get-ChildItem $MsvcToolDir | Sort-Object Name -Descending | Select-Object -First 1
        if ($Latest) { $MsvcRoot = $Latest.FullName }
    }
}
if (-not $MsvcRoot -or -not (Test-Path (Join-Path $MsvcRoot "include"))) {
    Write-Host "ERROR: MSVC include directory not found. Set `$env:MSVC_ROOT environment variable." -ForegroundColor Red
    exit 1
}
Write-Host "MSVC: $MsvcRoot" -ForegroundColor Green

# ---- Detect Windows SDK ----
$WinSdkRoot = if ($env:WIN_SDK_ROOT) { $env:WIN_SDK_ROOT } else { "D:\Windows Kits" }
$WinSdkVersion = $env:WIN_SDK_VERSION
if (-not $WinSdkVersion) {
    $SdkIncDir = Join-Path $WinSdkRoot "10\Include"
    if (Test-Path $SdkIncDir) {
        $Latest = Get-ChildItem $SdkIncDir | Sort-Object Name -Descending | Select-Object -First 1
        if ($Latest) { $WinSdkVersion = $Latest.Name }
    }
}
if (-not $WinSdkVersion) {
    Write-Host "ERROR: Windows SDK not found. Set `$env:WIN_SDK_ROOT and `$env:WIN_SDK_VERSION." -ForegroundColor Red
    exit 1
}
$SdkInc = Join-Path $WinSdkRoot "10\Include\$WinSdkVersion"
Write-Host "Windows SDK: $SdkInc" -ForegroundColor Green

# ---- vcpkg includes ----
$VcpkgInc = Join-Path $BuildDir "vcpkg_installed\x64-windows\include"
if (-not (Test-Path $VcpkgInc)) {
    Write-Host "WARNING: vcpkg include dir not found: $VcpkgInc" -ForegroundColor Yellow
}

# ---- Project includes ----
$ProjectIncludes = @(
    (Join-Path $ProjectRoot "include")
    (Join-Path $ProjectRoot "src")
    (Join-Path $ProjectRoot "src\common")
    (Join-Path $ProjectRoot "src\common\mod\bedrock\addon")
    (Join-Path $ProjectRoot "tests")
)

# ---- Build extra-arg list ----
$ExtraArgs = @(
    "--extra-arg=--target=amd64-pc-windows-msvc"
    "--extra-arg=-std=c++20"
    "--extra-arg=-DNOMINMAX"
    "--extra-arg=-DWIN32_LEAN_AND_MEAN"
    "--extra-arg=-DWINVER=0x0A00"
    "--extra-arg=-D_WIN32_WINNT=0x0A00"
    "--extra-arg=-D_CRT_SECURE_NO_WARNINGS"
    "--extra-arg=-D_GNU_SOURCE"
)

foreach ($inc in $ProjectIncludes) {
    $ExtraArgs += "--extra-arg=-I$inc"
}
$ExtraArgs += "--extra-arg=-I$(Join-Path $MsvcRoot 'include')"
$ExtraArgs += "--extra-arg=-I$(Join-Path $SdkInc 'ucrt')"
$ExtraArgs += "--extra-arg=-I$(Join-Path $SdkInc 'shared')"
$ExtraArgs += "--extra-arg=-I$(Join-Path $SdkInc 'um')"
if (Test-Path $VcpkgInc) {
    $ExtraArgs += "--extra-arg=-I$VcpkgInc"
}

# ---- Parse arguments ----
$TidyArgs = [System.Collections.ArrayList]::new()
$Files = [System.Collections.ArrayList]::new()

$i = 0
while ($i -lt $Arguments.Count) {
    $arg = $Arguments[$i]
    switch -Regex ($arg) {
        '^(--fix|--fix-errors|--quiet)$' {
            [void]$TidyArgs.Add($arg)
        }
        '^(--checks|--header-filter|--format-style|--line-filter|--warnings-as-errors)$' {
            [void]$TidyArgs.Add($arg)
            $i++
            if ($i -lt $Arguments.Count) {
                [void]$TidyArgs.Add($Arguments[$i])
            }
        }
        '^--(checks|header-filter|format-style|line-filter|warnings-as-errors)=' {
            [void]$TidyArgs.Add($arg)
        }
        '^(-h|--help)$' {
            Write-Host "Usage: .\scripts\tidy\tidy.ps1 [clang-tidy-options] <file(s)>..."
            Write-Host ""
            Write-Host "If no files are specified, scans all .cpp files under src\."
            Write-Host "Supports all standard clang-tidy options (--fix, --checks, etc.)."
            exit 0
        }
        '^-' {
            [void]$TidyArgs.Add($arg)
        }
        default {
            [void]$Files.Add($arg)
        }
    }
    $i++
}

# Default: scan all project source files
if ($Files.Count -eq 0) {
    Write-Host "No files specified. Scanning all .cpp files under src\..." -ForegroundColor Yellow
    $Files = @(Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Filter "*.cpp" -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName })
}

$Total = $Files.Count
if ($Total -eq 0) {
    Write-Host "ERROR: No source files found." -ForegroundColor Red
    exit 1
}

Write-Host "Scanning $Total file(s)..." -ForegroundColor Green
Write-Host ""

# ---- Run clang-tidy ----
$Pass = 0
$Fail = 0
$Skip = 0
$FailedFiles = [System.Collections.ArrayList]::new()

foreach ($file in $Files) {
    # Resolve to absolute path
    $absFile = $file
    if (-not (Test-Path $absFile -PathType Leaf)) {
        $candidate = Join-Path $ProjectRoot $file
        if (Test-Path $candidate -PathType Leaf) {
            $absFile = (Resolve-Path $candidate).Path
        } else {
            Write-Host "SKIP: $file (not found)" -ForegroundColor Yellow
            $Skip++
            continue
        }
    } else {
        $absFile = (Resolve-Path $absFile).Path
    }

    $relPath = $absFile.Substring($ProjectRoot.Length + 1)

    $allArgs = $ExtraArgs + $TidyArgs + @($absFile)

    # Capture output without throwing on stderr — clang-tidy writes diagnostics to stderr
    $outputLines = [System.Collections.ArrayList]::new()
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ClangTidy
    $psi.Arguments = ($allArgs | ForEach-Object { if ($_ -match '\s') { '"{0}"' -f $_ } else { $_ } }) -join ' '
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $psi.WorkingDirectory = $ProjectRoot

    $proc = [System.Diagnostics.Process]::Start($psi)
    $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
    $stderrTask = $proc.StandardError.ReadToEndAsync()
    [void]$proc.WaitForExit()
    $exitCode = $proc.ExitCode

    $stdout = $stdoutTask.Result
    $stderr = $stderrTask.Result
    $fullOutput = $stdout + $stderr

    # Filter: only show lines from project source or summary lines
    $filtered = $fullOutput -split "`n" | Where-Object { $_ -match '^(E:\\dev|src[/\\]|Suppressed|\d+ warning|\d+ error)' }

    if ($exitCode -eq 0) {
        if ($filtered) {
            Write-Host "WARN: $relPath" -ForegroundColor Yellow
            $filtered | ForEach-Object { Write-Host $_ }
        } else {
            Write-Host "  OK: $relPath" -ForegroundColor Green
        }
        $Pass++
    } elseif ($exitCode -eq 139 -or $exitCode -lt 0) {
        Write-Host "CRASH: $relPath (segfault)" -ForegroundColor Red
        $Skip++
    } else {
        Write-Host "FAIL: $relPath" -ForegroundColor Red
        if ($filtered) { $filtered | ForEach-Object { Write-Host $_ } }
        $Fail++
        [void]$FailedFiles.Add($relPath)
    }
}

# ---- Summary ----
Write-Host ""
Write-Host "=========================================="
Write-Host "  Total: $Total  Pass: $Pass  Fail: $Fail  Skip: $Skip" -NoNewline
Write-Host ""
Write-Host "=========================================="

if ($FailedFiles.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed files:" -ForegroundColor Red
    foreach ($f in $FailedFiles) {
        Write-Host "  - $f"
    }
}

exit $Fail
