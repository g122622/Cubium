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
import { Level } from "level";
import { stopHook } from "./hooks/stop";
import { cmakeGuardHook } from "./hooks/cmakeGuard";

(async () => {
    const fileSet = new Set<string>();
    await forEachFile("../src/", async (filePath) => {
        if (filePath.endsWith(".cpp") || filePath.endsWith(".hpp")) {
            if (filePath.includes("util\\") || filePath.includes("main.") || filePath.includes("common\\core")) {
                return;
            }
            // 去掉扩展名
            const pathWithoutExt = filePath.split(".").slice(0, -1).join(".");
            fileSet.add(pathWithoutExt);
        }
    });
    console.log("fileSet.size=", fileSet.size);

    const tasklist = [...fileSet]
        .map(file => `/mc-improve-code-style ${file}.hpp/cpp 注意：你被要求审查的文件可能没有上述这些问题，这种情况下你直接放行即可，你不一定必须修改代码。`)
    // .slice(84, 9999)
    // .slice(259, 9999);

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
                            { hooks: [cmakeGuardHook] },
                        ]
                    },
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
                    // for (const block of message.message.content) {
                    //   if (typeof block !== "string" && block.type === "tool_result") {
                    //     const resultSummary =
                    //       typeof block.content === "string"
                    //         ? block.content.slice(0, 300)
                    //         : JSON.stringify(block.content).slice(0, 300);
                    //     // console.log(
                    //     //   `📋 工具结果: ${resultSummary}${block.content?.length > 300 ? "…" : ""}`,
                    //     // );
                    //   }
                    // }
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

    const DB_PATH = "./agent-taskdb";
    const db = new Level(DB_PATH, { valueEncoding: "utf8" });
    await db.open();
    console.log(`✅ LevelDB 已打开: ${DB_PATH}`);

    /**
     * 检查任务是否已完成
     * @returns true 表示该任务已经执行过，应跳过
     */
    async function isTaskDone(task: string): Promise<boolean> {
        // return false;
        try {
            const val = await db.get(task);
            return val === "1";
        } catch {
            // key 不存在时 level 抛出 KeyError，表示未执行过
            return false;
        }
    }

    /**
     * 标记任务为已完成
     */
    async function markTaskDone(task: string): Promise<void> {
        await db.put(task, "1");
    }

    async function main() {
        const outerLoops = 1;
        const innerLoops = 1;

        for (let i = 0; i < outerLoops; i++) {
            console.log(
                `\n========== 外层循环第 ${i + 1} / ${outerLoops} 次 ==========\n`,
            );
            for (let j = 0; j < tasklist.length; j++) {
                // 检查 LevelDB：如果该任务已完成则跳过
                if (await isTaskDone(tasklist[j])) {
                    console.log(`⏭️ [循环 ${i + 1} | 任务 ${j + 1}] 已完成，跳过: ${tasklist[j]}`);
                    continue;
                }

                for (let k = 0; k < innerLoops; k++) {
                    await runTask(tasklist[j], i, j, false);
                }

                // 任务执行完毕，标记为已完成
                await markTaskDone(tasklist[j]);
                console.log(`✅ [循环 ${i + 1} | 任务 ${j + 1}] 已标记完成: ${tasklist[j]}`);

                if ((j % 30 === 0 && j !== 0) || j === tasklist.length - 1) {
                    console.log("\n🔨 进行编译检查，确保没有引入编译错误...");
                    await runTask("编译项目，并检查是否通过，若不通过则收敛编译错误，直到通过。", i, j, false);
                }
            }
            console.log(`\n========== 第 ${i + 1} 次循环结束 ==========\n`);
        }

        console.log("所有任务执行完毕。");
        await db.close();
    }

    main().catch(async (err) => {
        console.error(err);
        await db.close();
    });

})();
