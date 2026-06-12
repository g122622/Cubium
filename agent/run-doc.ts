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
import { forEachFile } from "./utils/forEachFile";
import { TaskPool } from "./utils/taskPool";
import { stopHook } from "./hooks/stop";
import { cmakeGuardHook } from "./hooks/cmakeGuard";

(async () => {
    const fileSet = new Set<string>();
    await forEachFile("../src/", async (filePath) => {
        if (filePath.endsWith(".md")) {
            fileSet.add(filePath);
        }
    });
    console.log("fileSet.size=", fileSet.size);

    const tasklist = [...fileSet]
        .map(file => `/mc-update-doc ${file} 注意：你被要求审查的文档可能没有上述这些问题，这种情况下你直接放行即可，你不一定必须修改代码。`)

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
                    model: "glm-5",
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
        const outerLoops = 1;
        const innerLoops = 1;
        const concurrency = 1; // 并行度

        for (let i = 0; i < outerLoops; i++) {
            console.log(
                `\n========== 外层循环第 ${i + 1} / ${outerLoops} 次 ==========\n`,
            );

            // 过滤出未完成的任务
            const pendingTasks: { task: string; index: number }[] = [];
            for (let j = 0; j < tasklist.length; j++) {
                pendingTasks.push({ task: tasklist[j], index: j });
            }

            console.log(`📋 待执行任务数: ${pendingTasks.length} / ${tasklist.length}`);

            // 使用 TaskPool 并行执行任务
            const pool = new TaskPool(concurrency);
            let completedCount = 0;

            const taskFunctions = pendingTasks.map(({ task, index }) => async () => {
                for (let k = 0; k < innerLoops; k++) {
                    await runTask(task, i, index, false);
                }
                return { task, index };
            });

            await pool.runAll(taskFunctions, async (result) => {
                if (result.success) {
                    const { task, index } = result.value!;
                    completedCount++;
                    console.log(
                        `✅ [循环 ${i + 1} | 任务 ${index + 1}] 已标记完成 (${completedCount}/${pendingTasks.length}): ${task}`
                    );
                } else {
                    console.error(
                        `❌ [循环 ${i + 1} | 任务 ${result.index + 1}] 执行失败:`,
                        result.error
                    );
                }
            });

            console.log(`\n========== 第 ${i + 1} 次循环结束 ==========\n`);
        }

        console.log("所有任务执行完毕。");
    }

    main().catch(async (err) => {
        console.error(err);
    });

})();
