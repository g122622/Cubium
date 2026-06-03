import { query, HookCallback, StopHookInput, PreToolUseHookInput } from "@anthropic-ai/claude-agent-sdk";
import { cleanJsonlFile } from "../utils/cleanJsonlFile";

const STOP_HOOK_PROMPT = `You are evaluating whether Claude should stop working.

请严格根据当前会话上下文，逐项检查以下完工标准：
1. 功能完整性：所有用户请求的任务是否已完全实现？任务列表是否彻底清空？计划是否全部实现？
2. 集成检查：新代码是否已正确接入现有游戏流程，没有形成无法被调用的'孤岛代码'？
3. 测试用例：测试是否完备、逻辑合理，且包含足够的边界/异常场景，足以暴露潜在缺陷？
4. 代码清理：已废弃的旧逻辑、死代码、调试输出或临时文件是否已彻底清除？
5. 文档同步：相关模块及路径下的各级 README 是否已按最新实现更新？
 如果你拿不准，无法完整确定上述是否全部完成，请你保守地返回false，并在reason中指出需要复查的地方

例外情况：如果编译命令出现的错误不是编译或链接错误，而是工具链错误，如cmake、ninja、vcpkg等，则你无需修复，直接停下来等我处理就行了。

请仅返回标准 JSON，严禁包含 markdown 格式或额外解释，并且注意尽量不要在json的reason字段中出现特殊字符，如果无法避免出现特殊字符，记得增加转义：
{"ok": true} 或 {"ok": false, "reason": "未达标的具体项及下一步明确操作指令"}`;

interface StopEvaluationResult {
    ok: boolean;
    reason?: string;
}

async function evaluateShouldStop(
    transcriptPath: string,
    lastAssistantMessage: string | undefined,
    sessionId: string,
): Promise<StopEvaluationResult> {
    console.log("\n🔍 启动评估代理检查完工标准...");

    // 构建评估上下文
    const evaluationPrompt =
        `
## 你的任务
${STOP_HOOK_PROMPT}

## 当前会话上下文
- 会话 ID: ${sessionId}
- 会话记录: 
<Transcript>
${await cleanJsonlFile(transcriptPath)}
</Transcript>
- 最后助手消息: 
<LastAssistantMessage>
${lastAssistantMessage ? lastAssistantMessage : "无"}
</LastAssistantMessage>

## 你的任务
${STOP_HOOK_PROMPT}
`;

    console.log(`📋 评估提示语:\n${evaluationPrompt}...\n`);

    try {
        let resultText = "";

        // 启动一个不带 hook 的专用评估代理
        for await (const message of query({
            prompt: evaluationPrompt,
            options: {
                permissionMode: "bypassPermissions",
                // 不带 hook，避免递归
            },
        })) {
            if (message.type === "assistant" && message.message?.content) {
                for (const block of message.message.content) {
                    if ("text" in block) {
                        resultText += block.text;
                    } else if ("name" in block && block.type === "tool_use") {
                        // 工具调用，例如 Read、Edit、Bash 等
                        console.log(
                            `\n🛠️ 审查代理调用工具: ${block.name} (${block.input ? JSON.stringify(block.input).slice(0, 200) : ""})`,
                        );
                    }
                }
            } else if (message.type === "result") {
                if (message.subtype === "success" && "result" in message && message.result) {
                    resultText = message.result;
                }
            }
        }

        // 解析 JSON 结果
        // 尝试从结果中提取 JSON
        const jsonMatch = resultText.match(/\{[^{}]*"ok"[^{}]*\}/s);
        if (jsonMatch) {
            const parsed = JSON.parse(jsonMatch[0]) as StopEvaluationResult;
            console.log(`📊 评估结果: ok=${parsed.ok}${parsed.reason ? `, reason=${parsed.reason}` : ""}`);
            return parsed;
        }

        console.warn("⚠️ 无法解析评估结果，默认不允许停止");
        console.log(`原始结果: ${resultText.slice(0, 500)}`);
        return {
            ok: false,
            reason: "评估结果无法解析为有效 JSON: " + resultText.slice(0, 500),
        };
    } catch (error) {
        console.error("⚠️ 评估代理执行出错，默认允许停止:", error instanceof Error ? error.message : error);
        return { ok: true };
    }
}

export const stopHook: HookCallback = async (input, toolUseID, { signal }) => {
    const stopInput = input as StopHookInput;
    console.log("\n🛑 Stop hook 触发，正在评估是否应该停止...");
    console.log(`📋 stop_hook_active: ${stopInput.stop_hook_active}`);
    console.log(`📁 transcript_path: ${stopInput.transcript_path}`);

    // if (stopInput.last_assistant_message) {
    //   console.log(`📝 最后消息摘要: ${stopInput.last_assistant_message.slice(0, 200)}...`);
    // }

    // 启动评估代理
    const evaluation = await evaluateShouldStop(
        stopInput.transcript_path,
        stopInput.last_assistant_message,
        stopInput.session_id,
    );

    // 根据评估结果决定是否阻止停止
    if (!evaluation.ok) {
        console.log(`\n🚫 阻止停止: ${evaluation.reason}`);
        const reason = `停止请求被拒绝。原因: ${evaluation.reason}\n\n请继续完成相关工作，确保所有完工标准都已满足。`;
        return {
            decision: "block",
            systemMessage: reason,
            stopReason: reason,
            reason: reason
        };
    }

    console.log("✅ 评估通过，允许停止");
    return {};
};
