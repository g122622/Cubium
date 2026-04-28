# 读取 stdin 的 JSON 输入
$rawInput = [Console]::In.ReadToEnd()
if ([string]::IsNullOrWhiteSpace($rawInput)) { exit 0 }

try {
    $data = $rawInput | ConvertFrom-Json
    $command = $data.tool_input.command
} catch {
    exit 0
}

# 忽略大小写匹配 cmake 关键字
if ($command -and $command -match 'cmake') {
    $output = @{
        hookSpecificOutput = @{
            hookEventName = "PreToolUse"
            permissionDecision = "allow"
            additionalContext = "【构建规范提示】检测到 CMake 命令。请注意：本项目只能使用 `cmake --build build --config RelWithDebInfo` 进行构建，请勿使用其他参数或路径。"
        }
    }
    # 压缩输出避免换行干扰 JSON 解析
    Write-Output ($output | ConvertTo-Json -Depth 3 -Compress)
}

# 必须返回 0，确保绝不阻断执行
exit 0
