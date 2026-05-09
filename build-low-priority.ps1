# 以低优先级运行 CMake 构建，不影响前台应用性能
# 用法: .\build-low-priority.ps1 [build_args...]
# 示例: .\build-low-priority.ps1 --config Release

param(
    [Parameter(ValueFromRemainingArguments)]
    [string[]]$BuildArgs
)

# 默认构建目录
$BuildDir = "build"

if (-not (Test-Path $BuildDir)) {
    Write-Host "构建目录不存在，请先运行 cmake -B build 配置项目" -ForegroundColor Yellow
    exit 1
}

Write-Host "以低优先级启动构建..." -ForegroundColor Cyan

# 使用低优先级启动 cmake --build
$process = Start-Process -FilePath "cmake" `
    -ArgumentList @("--build", $BuildDir) + $BuildArgs `
    -Priority BelowNormal `
    -NoNewWindow `
    -PassThru `
    -Wait

exit $process.ExitCode
