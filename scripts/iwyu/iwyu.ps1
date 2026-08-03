# ============================================================
# clang-include-cleaner wrapper for Cubium (PowerShell)
#
# 离线分析 #include 冗余/缺失，基于 compile_commands.json。
# 不挂钩编译，不影响日常构建。
#
# 与 tidy.ps1 的关键差异：
#   - 走 -p build/ 消费 compile_commands.json（取 -D 宏、项目 -I、vcpkg -isystem）
#   - 只用 --extra-arg 补 MSVC/SDK/clang-builtin 系统头（项目 -I/vcpkg 已在 database）
#   - 用 --ignore-headers 排除第三方头噪音
#
# Usage:
#   .\scripts\iwyu\iwyu.ps1                              # 扫试点 src\common\core
#   .\scripts\iwyu\iwyu.ps1 src\common\core\Result.cpp    # 扫指定文件
#   .\scripts\iwyu\iwyu.ps1 src\common\core               # 扫目录下所有 .cpp
#   .\scripts\iwyu\iwyu.ps1 --edit src\common\core\       # 应用建议（危险！先 git status 干净）
#   .\scripts\iwyu\iwyu.ps1 --remove src\common\core\     # 只建议移除头
#   .\scripts\iwyu\iwyu.ps1 --insert src\common\core\     # 只建议插入头
#   .\scripts\iwyu\iwyu.ps1 --print src\common\core\      # 打印最终代码（默认 --print=changes）
#   .\scripts\iwyu\iwyu.ps1 --html=report.html ...        # 输出 HTML 报告
#   .\scripts\iwyu\iwyu.ps1 --only-headers='common/.*'    # 只分析匹配后缀的头
#   .\scripts\iwyu\iwyu.ps1 --ignore-headers='...'        # 覆盖默认第三方头排除
#
# Environment variables:
#   $env:CLANG_INCLUDE_CLEANER  - 工具路径（默认自动检测）
#   $env:MSVC_ROOT              - MSVC 工具链根目录（默认自动检测最新版本）
#   $env:WIN_SDK_ROOT           - Windows Kits/10 根目录（默认 D:\Windows Kits）
#   $env:WIN_SDK_VERSION        - SDK 版本号（默认自动检测最新版本）
#   $env:IWYU_BUILD_DIR         - compile_commands.json 目录（默认 build）
# ============================================================

param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Arguments
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = (Resolve-Path (Join-Path $ScriptDir "..\..")).Path
$BuildDir = if ($env:IWYU_BUILD_DIR) { $env:IWYU_BUILD_DIR } else { Join-Path $ProjectRoot "build" }
$PilotDir = "src\common\core"

# 默认排除的第三方头（单一大正则，匹配"头的后缀"）
$IgnoreHeadersDefault = "build/vcpkg_installed|third_party/|perfetto|tracy|_deps/|OffsetAllocator|quickjs"

# ---- Detect clang-include-cleaner ----
$ClangIc = $env:CLANG_INCLUDE_CLEANER
if (-not $ClangIc) {
    $Cmd = Get-Command clang-include-cleaner -ErrorAction SilentlyContinue
    if ($Cmd) { $ClangIc = $Cmd.Source }
    elseif (Test-Path "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-include-cleaner.exe") {
        $ClangIc = "D:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\Llvm\x64\bin\clang-include-cleaner.exe"
    } else {
        Write-Host "ERROR: clang-include-cleaner not found. Set `$env:CLANG_INCLUDE_CLEANER environment variable." -ForegroundColor Red
        exit 1
    }
}

Write-Host "Using clang-include-cleaner: $ClangIc" -ForegroundColor Green
& $ClangIc --version | Select-Object -First 1

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

# ---- Detect clang builtin headers ----
# 从 --version 动态提取 LLVM 主版本号，拼 lib\clang\<ver>\include（含 stddef.h/stdint.h 等）
$LlvmVersion = $null
$VersionLine = (& $ClangIc --version | Select-String 'LLVM version (\d+)')
if ($VersionLine) { $LlvmVersion = $VersionLine.Matches.Groups[1].Value }
$ClangLibInclude = $null
if ($LlvmVersion) {
    $ClangBinDir = Split-Path -Parent $ClangIc
    $Candidate = Join-Path (Split-Path -Parent $ClangBinDir) "lib\clang\$LlvmVersion\include"
    if (Test-Path $Candidate) {
        $ClangLibInclude = $Candidate
        Write-Host "Clang builtin: $ClangLibInclude" -ForegroundColor Green
    }
}

# ---- Build extra-arg list（只补系统头，项目 -I/vcpkg 已在 compile_commands.json） ----
$ExtraArgs = [System.Collections.ArrayList]::new()
[void]$ExtraArgs.Add("--extra-arg=-DNOMINMAX")
[void]$ExtraArgs.Add("--extra-arg=-DWIN32_LEAN_AND_MEAN")
[void]$ExtraArgs.Add("--extra-arg=-D_CRT_SECURE_NO_WARNINGS")
[void]$ExtraArgs.Add("--extra-arg=-I$(Join-Path $MsvcRoot 'include')")
[void]$ExtraArgs.Add("--extra-arg=-I$(Join-Path $SdkInc 'ucrt')")
[void]$ExtraArgs.Add("--extra-arg=-I$(Join-Path $SdkInc 'shared')")
[void]$ExtraArgs.Add("--extra-arg=-I$(Join-Path $SdkInc 'um')")
if ($ClangLibInclude) {
    [void]$ExtraArgs.Add("--extra-arg=-I$ClangLibInclude")
}

# ---- Parse arguments ----
$IwyuArgs = [System.Collections.ArrayList]::new()
$Files = [System.Collections.ArrayList]::new()
$EditMode = $false
$PrintModeSet = $false
$IgnoreSet = $false

$i = 0
while ($i -lt $Arguments.Count) {
    $arg = $Arguments[$i]
    switch -Regex ($arg) {
        '^--edit$' {
            $EditMode = $true
            [void]$IwyuArgs.Add($arg)
        }
        '^(--remove|--insert)$' {
            [void]$IwyuArgs.Add($arg)
        }
        '^--print(=.*)?$' {
            [void]$IwyuArgs.Add($arg)
            $PrintModeSet = $true
        }
        '^(--html=.*|--only-headers=.*)$' {
            [void]$IwyuArgs.Add($arg)
        }
        '^--ignore-headers=.*$' {
            [void]$IwyuArgs.Add($arg)
            $IgnoreSet = $true
        }
        '^--build-dir=.*$' {
            $BuildDir = $arg.Substring(("--build-dir=").Length)
        }
        '^(-h|--help)$' {
            Write-Host "Usage: iwyu.ps1 [options] <file(s) or dir(s)>..."
            Write-Host "If no files/dirs specified, scans src\common\core. Directories expand to *.cpp."
            Write-Host "Options: --print[=changes] --remove --insert --edit --html=<f> --only-headers=<r> --ignore-headers=<r> --build-dir=<d>"
            exit 0
        }
        '^-' {
            [void]$IwyuArgs.Add($arg)
        }
        default {
            [void]$Files.Add($arg)
        }
    }
    $i++
}

# 默认 --print=changes（若用户未指定 print/edit/html 任一输出模式）
if (-not $PrintModeSet -and -not $EditMode) {
    $hasHtml = $false
    foreach ($a in $IwyuArgs) { if ($a -like '--html=*') { $hasHtml = $true } }
    if (-not $hasHtml) {
        [void]$IwyuArgs.Add("--print=changes")
    }
}

# 默认 --ignore-headers（若用户未覆盖）
if (-not $IgnoreSet) {
    [void]$IwyuArgs.Add("--ignore-headers=$IgnoreHeadersDefault")
}

# ---- compile_commands.json 存在性检查 ----
$CompileDb = Join-Path $BuildDir "compile_commands.json"
if (-not (Test-Path $CompileDb)) {
    Write-Host "ERROR: $CompileDb not found." -ForegroundColor Red
    Write-Host "Run '.\scripts\configure.sh build' first (CMakePresets now exports compile_commands.json)."
    exit 1
}

# ---- 默认扫试点子目录 ----
if ($Files.Count -eq 0) {
    Write-Host "No files specified. Scanning pilot dir $PilotDir ..." -ForegroundColor Yellow
    [void]$Files.Add((Join-Path $ProjectRoot $PilotDir))
}

# ---- 展开目录为 .cpp 列表 ----
$Expanded = [System.Collections.ArrayList]::new()
foreach ($f in $Files) {
    if (Test-Path $f -PathType Container) {
        Get-ChildItem -Path $f -Filter "*.cpp" -Recurse -File | Sort-Object FullName | ForEach-Object {
            [void]$Expanded.Add($_.FullName)
        }
    } else {
        [void]$Expanded.Add($f)
    }
}
$Files = $Expanded

if ($Files.Count -eq 0) {
    Write-Host "ERROR: No .cpp source files found." -ForegroundColor Red
    exit 1
}

# ---- --edit 安全闸：工作树脏则交互确认 ----
if ($EditMode) {
    & git -C $ProjectRoot diff --quiet 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Host "WARNING: Working tree has uncommitted changes." -ForegroundColor Red
        $resp = Read-Host "  --edit will modify source files in place. Continue? [y/N]"
        if ($resp -notmatch '^[Yy]$') {
            Write-Host "Aborted."
            exit 1
        }
    }
    Write-Host "EDIT MODE: will modify source files in place." -ForegroundColor Red
}

# ---- 去重 clang-include-cleaner --print=changes 的重复输出 ----
# LLVM 20 的 include-cleaner 对单个翻译单元会输出 N 份完全相同的建议块
# （N 不固定，与文件复杂度相关：试点见过 3 次、12 次等；非多配置导致，是工具固有行为）。
# 用"保留首次出现顺序"的去重：整段重复时每段内行相同，去重后只剩唯一建议行。
function Dedupe-ChangesOutput {
    param([string]$Text)
    if (-not $Text) { return $Text }
    $lines = $Text -split "`r?`n"
    $seen = @{}
    $result = [System.Collections.ArrayList]::new()
    foreach ($ln in $lines) {
        if (-not $seen.ContainsKey($ln)) {
            $seen[$ln] = $true
            [void]$result.Add($ln)
        }
    }
    return ($result -join "`n")
}

# 是否对 --print=changes 输出去重（仅默认建议模式；--edit/--html/--print 最终代码模式不去重）
$ShouldDedupe = -not $EditMode
if ($PrintModeSet) {
    foreach ($a in $IwyuArgs) {
        if ($a -eq '--print') { $ShouldDedupe = $false }
    }
}
foreach ($a in $IwyuArgs) {
    if ($a -like '--html=*') { $ShouldDedupe = $false }
}

Write-Host "Scanning $($Files.Count) file(s)..." -ForegroundColor Green
Write-Host ""

# ---- Run clang-include-cleaner ----
$Pass = 0    # CLEAN
$Suggest = 0
$Fail = 0
$Skip = 0
$FailedFiles = [System.Collections.ArrayList]::new()

foreach ($absFile in $Files) {
    if (-not (Test-Path $absFile)) {
        Write-Host "SKIP: $absFile (not found)" -ForegroundColor Yellow
        $Skip++
        continue
    }
    $absFile = (Resolve-Path $absFile).Path
    $relPath = $absFile.Substring($ProjectRoot.Length + 1)

    $allArgs = @("-p", $BuildDir) + $ExtraArgs + $IwyuArgs + @($absFile)

    # 异步捕获 stdout/stderr，避免 PowerShell 在 stderr 上抛异常
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $ClangIc
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

    $fullOutput = $stdoutTask.Result + $stderrTask.Result

    if ($ShouldDedupe -and $exitCode -eq 0 -and $fullOutput) {
        $fullOutput = Dedupe-ChangesOutput -Text $fullOutput
    }

    if ($exitCode -eq 0) {
        if ($fullOutput) {
            Write-Host "SUGGEST: $relPath" -ForegroundColor Yellow
            Write-Host $fullOutput
            $Suggest++
        } else {
            Write-Host "  CLEAN: $relPath" -ForegroundColor Green
            $Pass++
        }
    } elseif ($exitCode -eq 139 -or $exitCode -lt 0) {
        Write-Host "CRASH: $relPath (exit $exitCode)" -ForegroundColor Red
        $Skip++
    } else {
        Write-Host "ERROR: $relPath (exit $exitCode)" -ForegroundColor Red
        $fullOutput.Split("`n") | Select-Object -First 20 | ForEach-Object { Write-Host $_ }
        $Fail++
        [void]$FailedFiles.Add($relPath)
    }
}

# ---- Summary ----
Write-Host ""
Write-Host "=========================================="
Write-Host "  Total: $($Files.Count)  Clean: $Pass  Suggest: $Suggest  Fail: $Fail  Skip: $Skip"
Write-Host "=========================================="

if ($FailedFiles.Count -gt 0) {
    Write-Host ""
    Write-Host "Failed files:" -ForegroundColor Red
    foreach ($f in $FailedFiles) { Write-Host "  - $f" }
}

exit $Fail
