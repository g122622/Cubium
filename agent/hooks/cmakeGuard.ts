/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

import { query, HookCallback, StopHookInput, PreToolUseHookInput } from "@anthropic-ai/claude-agent-sdk";

/**
 * PreToolUse hook: 拦截 cmake 相关的终端调用
 * 只允许使用 cmake --build --preset windows-clang-relwithdebinfo 进行构建
 */
export const cmakeGuardHook: HookCallback = async (input, toolUseID, { signal }) => {
    const preInput = input as PreToolUseHookInput;

    // 只拦截 Bash 工具调用
    if (preInput.tool_name !== "Bash") {
        return {};
    }

    const toolInput = preInput.tool_input as Record<string, unknown>;
    const command = (toolInput?.command as string) || "";

    // 检查命令是否包含 cmake 关键词（不区分大小写）
    if (!command.toLowerCase().includes("cmake")) {
        return {};
    }

    // 允许的正确构建命令
    const allowedCommand = "cmake --build --preset windows-clang-relwithdebinfo";

    // 检查是否是允许的命令（允许一些合理的变体，如额外的空格）
    const normalizedCommand = command.trim().replace(/\s+/g, " ");
    const normalizedAllowed = allowedCommand.replace(/\s+/g, " ");

    // 如果命令已经是正确的格式，允许执行
    if (normalizedCommand === normalizedAllowed || normalizedCommand.startsWith(normalizedAllowed + " ")) {
        console.log(`✅ cmake 命令检查通过: ${command}`);
        return {};
    }

    // 对非标准 cmake 命令进行软性提示，允许执行但注入提示
    console.log(`⚠️ 检测到非标准 cmake 命令: ${command}`);
    return {
        hookSpecificOutput: {
            hookEventName: preInput.hook_event_name,
            permissionDecision: "allow",
        },
        systemMessage: `【系统提示】检测到 cmake 命令调用: ${command}
推荐使用标准构建命令（及其变种）: ${allowedCommand}。提示：编译时间可能非常长（甚至20min+），必须停下来耐心等待，不要重复启动编译，不要重复启动编译！不要在后台运行编译。如果出现的错误不是编译错误，而是工具链错误，如cmake、ninja、vcpkg等，则你无需修复，直接停下来等我处理就行了`,
    };
};
