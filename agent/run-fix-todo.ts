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

import { query } from "@anthropic-ai/claude-agent-sdk";
import { stopHook } from "./hooks/stop";
import { cmakeGuardHook } from "./hooks/cmakeGuard";

(async () => {
    const tasklist = [
        "/mc-fix-todo",
        "/mc-fix-todo",
        "/mc-fix-todo",
        "/mc-fix-todo",
        "/mc-fix-todo",
        "/mc-fix-todo",
        "/mc-fix-todo"
    ]

    async function runTask(task: string, iteration: number, taskIndex: number, shouldEvaluate: boolean) {
        console.log(
            `\n🔁 [循环 ${iteration + 1} | 任务 ${taskIndex + 1}] 开始执行: ${task}\n`,
        );

        try {
            // query 返回异步迭代器，实时产出消息
            for await (const message of query({
                prompt: task,
                options: {
                    // 等价于 --dangerously-skip-permissions：绕过所有权限检查
                    permissionMode: "bypassPermissions",
                    hooks: {
                        Stop: shouldEvaluate ? [{ hooks: [stopHook] }] : [],
                        PreToolUse: [
                        ]
                    },
                    model: "glm-5.1",
                },
            })) {
                // 处理 assistant 消息（Claude 的思考和工具调用）
                if (message.type === "assistant" && message.message?.content) {
                    for (const block of message.message.content) {
                        if ("text" in block) {
                            // Claude 的自然语言推理过程
                            console.log(block.text);
                        } else if ("name" in block && block.type === "tool_use") {
                            // 工具调用，例如 Read、Edit、Bash 等
                            console.log(
                                `\n🛠️ 调用工具: ${block.name} (${block.input ? JSON.stringify(block.input).slice(0, 200) : ""})`,
                            );
                        }
                    }
                }
                // 处理 user 消息（工具执行结果）
                else if (message.type === "user") {
                }
                // 处理最终结果消息
                else if (message.type === "result") {
                    console.log(`\n✅ 任务完成，状态: ${message.subtype}`);
                }
            }
        } catch (error) {
            console.error(
                `❌ 任务执行失败: ${error instanceof Error ? error.message : error}`,
            );
        }
    }

    async function main() {
        for (let i = 0; i < 99999999; i++) {
            await runTask(tasklist[i % tasklist.length], i, (i % tasklist.length), true);
            await runTask(
                `请你检查当前代码能否编译通过（参考CLAUDE.md中的指引），如果不能编译通过，请修复编译错误直到能编译通过。`,
                i, (i % tasklist.length), false);
            await runTask(
                `请你检查当前是否有未提交的更改，若有则生成提交信息并提交，最后推送到远程仓库，并处理可能的冲突。 如果拉取/推送代码的时候因为网络问题没有成功，那么你无需重试，将该提交留在本地后即可停下来了，以后我会帮你去做。另外，/include/minecraft-reborn/version.h这个文件如果git显示未提交，你不用理会，将其留在工作区即可，重点是处理其他文件。
提交代码之前，必须使用clang-format对工作区中已修改的文件进行格式化：
clang - format - i src/common/xxx/Foo.cpp
clang - format - i src/common/xxx/Foo.hpp      
            `
                , i, (i % tasklist.length), false);
        }

        console.log("所有任务执行完毕。");
    }

    main().catch(async (err) => {
        console.error(err);
    });

})();
