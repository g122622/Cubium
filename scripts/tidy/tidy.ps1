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
#   .\scripts\tidy\tidy.ps1 --jobs 8 src\server\world      # Scan a directory with 8 workers
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
$MaxJobs = [Environment]::ProcessorCount
if ($env:TIDY_JOBS) {
    $parsedJobs = 0
    if (-not [int]::TryParse($env:TIDY_JOBS, [ref]$parsedJobs) -or $parsedJobs -lt 1) {
        Write-Host "ERROR: TIDY_JOBS must be a positive integer." -ForegroundColor Red
        exit 1
    }
    $MaxJobs = $parsedJobs
}

$i = 0
while ($i -lt $Arguments.Count) {
    $arg = $Arguments[$i]
    switch -Regex ($arg) {
        '^(--fix|--fix-errors|--quiet)$' {
            [void]$TidyArgs.Add($arg)
            break
        }
        '^(--checks|--header-filter|--format-style|--line-filter|--warnings-as-errors)$' {
            [void]$TidyArgs.Add($arg)
            $i++
            if ($i -lt $Arguments.Count) {
                [void]$TidyArgs.Add($Arguments[$i])
            }
            break
        }
        '^--jobs$' {
            $i++
            if ($i -ge $Arguments.Count -or -not [int]::TryParse($Arguments[$i], [ref]$MaxJobs) -or $MaxJobs -lt 1) {
                Write-Host "ERROR: --jobs requires a positive integer." -ForegroundColor Red
                exit 1
            }
            break
        }
        '^--jobs=(.*)$' {
            $parsedJobs = 0
            if (-not [int]::TryParse($Matches[1], [ref]$parsedJobs) -or $parsedJobs -lt 1) {
                Write-Host "ERROR: --jobs requires a positive integer." -ForegroundColor Red
                exit 1
            }
            $MaxJobs = $parsedJobs
            break
        }
        '^--(checks|header-filter|format-style|line-filter|warnings-as-errors)=' {
            [void]$TidyArgs.Add($arg)
            break
        }
        '^(-h|--help)$' {
            Write-Host "Usage: .\scripts\tidy\tidy.ps1 [clang-tidy-options] <file(s)>..."
            Write-Host ""
            Write-Host "If no files are specified, scans all .cpp files under src\."
            Write-Host "Directories are scanned recursively for C++ source and header files."
            Write-Host "Parallel workers default to the logical processor count; override with --jobs N or TIDY_JOBS."
            Write-Host "Supports clang-tidy options such as --fix, --checks, and --warnings-as-errors."
            exit 0
            break
        }
        '^-' {
            [void]$TidyArgs.Add($arg)
            break
        }
        default {
            [void]$Files.Add($arg)
            break
        }
    }
    $i++
}

if ($TidyArgs -contains '--fix' -or $TidyArgs -contains '--fix-errors') {
    $MaxJobs = 1
    Write-Host "Auto-fix mode uses one worker to avoid concurrent edits." -ForegroundColor Yellow
}

# Default: scan all project source files
if ($Files.Count -eq 0) {
    Write-Host "No files specified. Scanning all .cpp files under src\..." -ForegroundColor Yellow
    $Files = @(Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Filter "*.cpp" -Recurse | Sort-Object FullName | ForEach-Object { $_.FullName })
}

$ExpandedFiles = [System.Collections.ArrayList]::new()
$SourceExtensions = @('.cpp', '.cc', '.cxx', '.hpp', '.h')
foreach ($file in $Files) {
    $candidate = if ([System.IO.Path]::IsPathRooted($file)) { $file } else { Join-Path $ProjectRoot $file }
    if (Test-Path $candidate -PathType Container) {
        Get-ChildItem -Path $candidate -File -Recurse |
            Where-Object { $SourceExtensions -contains $_.Extension.ToLowerInvariant() } |
            Sort-Object FullName |
            ForEach-Object { [void]$ExpandedFiles.Add($_.FullName) }
    } else {
        [void]$ExpandedFiles.Add($file)
    }
}
$Files = $ExpandedFiles

$Total = $Files.Count
if ($Total -eq 0) {
    Write-Host "ERROR: No source files found." -ForegroundColor Red
    exit 1
}

Write-Host "Scanning $Total file(s)..." -ForegroundColor Green
Write-Host "Parallel workers: $([Math]::Min($MaxJobs, $Total)) (logical processors: $([Environment]::ProcessorCount))" -ForegroundColor Green
Write-Host ""

# ---- Run clang-tidy ----
$Pass = 0
$Fail = 0
$Skip = 0
$FailedFiles = [System.Collections.ArrayList]::new()
$DiagnosticCounts = @{}
$WorkItems = [System.Collections.Queue]::new()
$Running = [System.Collections.ArrayList]::new()
$TotalFileSeconds = 0.0
$WallClock = [System.Diagnostics.Stopwatch]::StartNew()

foreach ($file in $Files) {
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
    $WorkItems.Enqueue([pscustomobject]@{ Path = $absFile; RelativePath = $relPath })
}

while ($WorkItems.Count -gt 0 -or $Running.Count -gt 0) {
    while ($WorkItems.Count -gt 0 -and $Running.Count -lt [Math]::Min($MaxJobs, $Total)) {
        $workItem = $WorkItems.Dequeue()
        $allArgs = $ExtraArgs + $TidyArgs + @($workItem.Path)

        $psi = New-Object System.Diagnostics.ProcessStartInfo
        $psi.FileName = $ClangTidy
        $psi.Arguments = ($allArgs | ForEach-Object { if ($_ -match '\s') { '"{0}"' -f $_ } else { $_ } }) -join ' '
        $psi.UseShellExecute = $false
        $psi.RedirectStandardOutput = $true
        $psi.RedirectStandardError = $true
        $psi.CreateNoWindow = $true
        $psi.WorkingDirectory = $ProjectRoot

        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        $proc = [System.Diagnostics.Process]::Start($psi)
        $stdoutTask = $proc.StandardOutput.ReadToEndAsync()
        $stderrTask = $proc.StandardError.ReadToEndAsync()
        [void]$Running.Add([pscustomobject]@{
            Process = $proc
            StdoutTask = $stdoutTask
            StderrTask = $stderrTask
            Timer = $timer
            WorkItem = $workItem
        })
    }

    $completedAny = $false
    for ($index = $Running.Count - 1; $index -ge 0; $index--) {
        $job = $Running[$index]
        if (-not $job.Process.HasExited) {
            continue
        }

        [void]$job.Process.WaitForExit()
        $job.Timer.Stop()
        $TotalFileSeconds += $job.Timer.Elapsed.TotalSeconds
        $exitCode = $job.Process.ExitCode
        $fullOutput = $job.StdoutTask.Result + $job.StderrTask.Result
        $outputLines = $fullOutput -split "`r?`n"
        $filtered = $outputLines | Where-Object { $_ -match '^(E:\\dev|src[/\\]|Suppressed|\d+ warning|\d+ error)' }
        $diagnosticLines = @($outputLines | Where-Object { $_ -match ':(?:\d+):(?:\d+:)?\s+(?:warning|error):' })
        $diagnosticOutput = $filtered | Where-Object { $_ -notmatch '^(Suppressed|\d+ warnings? generated)' }

        foreach ($line in $outputLines) {
            if ($line -match ':(?:\d+):(?:\d+:)?\s+(?:warning|error):.*\[(?<check>[A-Za-z0-9_.-]+)') {
                $check = $Matches.check
                if (-not $DiagnosticCounts.ContainsKey($check)) { $DiagnosticCounts[$check] = 0 }
                $DiagnosticCounts[$check]++
            }
        }

        if ($exitCode -eq 0) {
            if ($diagnosticLines.Count -gt 0) {
                Write-Host "WARN: $($job.WorkItem.RelativePath)" -ForegroundColor Yellow
                $diagnosticOutput | ForEach-Object { Write-Host $_ }
            } else {
                Write-Host "  OK: $($job.WorkItem.RelativePath)" -ForegroundColor Green
            }
            $Pass++
        } elseif ($exitCode -eq 139 -or $exitCode -lt 0) {
            Write-Host "CRASH: $($job.WorkItem.RelativePath) (segfault)" -ForegroundColor Red
            $Skip++
        } else {
            Write-Host "FAIL: $($job.WorkItem.RelativePath)" -ForegroundColor Red
            if ($diagnosticLines.Count -gt 0) {
                $diagnosticOutput | ForEach-Object { Write-Host $_ }
            } else {
                $outputLines | ForEach-Object { Write-Host $_ }
            }
            $Fail++
            [void]$FailedFiles.Add($job.WorkItem.RelativePath)
        }

        Write-Host ("  Time: {0:N2}s" -f $job.Timer.Elapsed.TotalSeconds) -ForegroundColor DarkGray
        $job.Process.Dispose()
        $Running.RemoveAt($index)
        $completedAny = $true
    }

    if (-not $completedAny -and $Running.Count -gt 0) { Start-Sleep -Milliseconds 100 }
}
$WallClock.Stop()

# ---- Summary ----
Write-Host ""
Write-Host "=========================================="
Write-Host "  Total: $Total  Pass: $Pass  Fail: $Fail  Skip: $Skip" -NoNewline
Write-Host ""
if ($Pass + $Fail -gt 0) {
    Write-Host ("  Wall time: {0:N1}s  Mean per-file time: {1:N2}s  Throughput: {2:N2} files/s" -f `
        $WallClock.Elapsed.TotalSeconds, ($TotalFileSeconds / ($Pass + $Fail)), (($Pass + $Fail) / $WallClock.Elapsed.TotalSeconds))
}
Write-Host "=========================================="

if ($DiagnosticCounts.Count -gt 0) {
    Write-Host ""
    Write-Host "Diagnostics by check:" -ForegroundColor Yellow
    foreach ($check in ($DiagnosticCounts.Keys | Sort-Object)) {
        Write-Host ("  {0}: {1}" -f $check, $DiagnosticCounts[$check])
    }
}

if ($FailedFiles.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed files:" -ForegroundColor Red
    foreach ($f in $FailedFiles) {
        Write-Host "  - $f"
    }
}

exit $Fail
