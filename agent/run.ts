import { query, HookCallback, StopHookInput, PreToolUseHookInput } from "@anthropic-ai/claude-agent-sdk";
import fs from 'fs/promises';
import { initQQBot } from './qqBot';

(async () => {

  /**
   * 清洗 JSONL 文件内容，转换为人类可读的对话文本。
   * - 保留用户直接输入的消息（role: user, type: text）
   * - 保留助手的直接回复（role: assistant, type: text）
   * - 跳过所有工具调用（tool_use）和工具返回结果（tool_result）
   * - 保留待办事项（type: attachment, todo_reminder）和最后提示（last-prompt）作为摘要
   *
   * @param filePath - JSONL 文件路径
   * @returns 清洗后的纯文本字符串
   */
  async function cleanJsonlFile(filePath: string): Promise<string> {
    /**
     * 从消息的 content 数组中提取所有 type === 'text' 的文本内容。
     * 跳过 tool_use 和 tool_result。
     */
    function extractTextFromContent(content: any[]): string {
      if (!Array.isArray(content)) return '';
      const textParts: string[] = [];
      for (const item of content) {
        if (item.type === 'text' && typeof item.text === 'string') {
          textParts.push(item.text);
        }
      }
      return textParts.join(' ').trim();
    }

    const content = await fs.readFile(filePath, 'utf-8');
    const lines = content.split(/\r?\n/).filter(line => line.trim().length > 0);

    const outputParts: string[] = [];

    for (const line of lines) {
      let obj: any;
      try {
        obj = JSON.parse(line);
      } catch {
        continue; // 忽略无效 JSON 行
      }

      // 1. 用户消息（直接提问或指令）
      if (obj.type === 'user' && obj.message?.role === 'user') {
        const userText = extractTextFromContent(obj.message.content);
        if (userText) {
          outputParts.push(`<User> ${userText} </User>\n`);
        }
      }

      // 2. 助手消息（直接回复）
      else if (obj.type === 'assistant' && obj.message?.role === 'assistant') {
        const assistantText = extractTextFromContent(obj.message.content);
        if (assistantText) {
          outputParts.push(`<Assistant> ${assistantText} </Assistant>\n`);
        }
      }

      // 3. 待办事项附件
      else if (obj.type === 'attachment' && obj.attachment?.type === 'todo_reminder') {
        const todoItems = obj.attachment.content;
        if (Array.isArray(todoItems) && todoItems.length > 0) {
          outputParts.push(`<TodoList> 待办事项:\n`);
          for (const item of todoItems) {
            const status = item.status === 'completed' ? '[x]' : '[ ]';
            outputParts.push(`${status} ${item.content}\n`);
          }
          outputParts.push(`</TodoList>\n`);
        }
      }

      // 4. 最后提示（last-prompt）
      else if (obj.type === 'last-prompt' && obj.lastPrompt) {
        outputParts.push(`<LastPrompt> ${obj.lastPrompt} </LastPrompt>\n`);
      }
    }

    return outputParts.join('').trim();
  }


  const STOP_HOOK_PROMPT = `You are evaluating whether Claude should stop working.

请严格根据当前会话上下文，逐项检查以下完工标准：
1. 功能完整性：所有用户请求的任务是否已完全实现？任务列表是否彻底清空？计划是否全部实现？
2. 集成检查：新代码是否已正确接入现有游戏流程，没有形成无法被调用的'孤岛代码'？
3. 测试用例：测试是否完备、逻辑合理，且包含足够的边界/异常场景，足以暴露潜在缺陷？
4. 代码清理：已废弃的旧逻辑、死代码、调试输出或临时文件是否已彻底清除？
5. 文档同步：相关模块及路径下的各级 README 是否已按最新实现更新？
 如果你拿不准，无法完整确定上述是否全部完成，请你保守地返回false，并在reason中指出需要复查的地方

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
      `${STOP_HOOK_PROMPT}

## 当前会话上下文
- 会话 ID: ${sessionId}
- 会话记录: 
<Transcript>
${await cleanJsonlFile(transcriptPath)}
</Transcript>

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

  const stopHook: HookCallback = async (input, toolUseID, { signal }) => {
    const stopInput = input as StopHookInput;
    console.log("\n🛑 Stop hook 触发，正在评估是否应该停止...");
    console.log(`📋 stop_hook_active: ${stopInput.stop_hook_active}`);
    console.log(`📁 transcript_path: ${stopInput.transcript_path}`);

    if (stopInput.last_assistant_message) {
      console.log(`📝 最后消息摘要: ${stopInput.last_assistant_message.slice(0, 200)}...`);
    }

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

  /**
   * PreToolUse hook: 拦截 cmake 相关的终端调用
   * 只允许使用 cmake --build build --config RelWithDebInfo 进行构建
   */
  const cmakeGuardHook: HookCallback = async (input, toolUseID, { signal }) => {
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
    const allowedCommand = "cmake --build build --config RelWithDebInfo";

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
推荐使用标准构建命令（及其变种）: ${allowedCommand}。`,
    };
  };

  const tasklist = [
    // //"/align 地形生成过程中的结构生成算法、模板解析、jigsaw系统、结构完整度（重点）等（提示：结构生成的nbt模板在mac环境位于/Users/a0000/MC_Dev/resourcePacks/Vanilla/data/minecraft/structures，在Windows环境位于D:/Minecraft/MC_Dev/resourcePacks/Vanilla/data/minecraft/structures，你需要充分探索这个目录。另外，随机数必须使用项目封装好的，不要自建随机数生成器）",
    // //"/align 含水方块",
    // //"/align 实体ai",
    //  "/align 音效丰富度，以及触发音效的时机是否完整",
    // // "/align 物品丰富度",
    // // "/align 矿车系统",
    // "/align 骑乘系统&boat系统&交通工具系统等",
    // //"/align 天气系统",
    // //"/align 库存系统",
    // "/align 容器系统",
    // "/align 下界与主世界之间的传送",

    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
    "/fix-todo",
  ];

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
          for (const block of message.message.content) {
            if (block.type === "tool_result") {
              const resultSummary =
                typeof block.content === "string"
                  ? block.content.slice(0, 300)
                  : JSON.stringify(block.content).slice(0, 300);
              // console.log(
              //   `📋 工具结果: ${resultSummary}${block.content?.length > 300 ? "…" : ""}`,
              // );
            }
          }
        }
        // 处理最终结果消息
        else if (message.type === "result") {
          console.log(`\n✅ 任务完成，状态: ${message.subtype}`);
          if (message.result) {
            console.log(message.result);
          }
        }
      }
    } catch (error) {
      console.error(
        `❌ 任务执行失败: ${error instanceof Error ? error.message : error}`,
      );
    }
  }

  async function main() {
    // 初始化 QQ 机器人（劫持 console 方法）
    await initQQBot();

    const outerLoops = 300;
    const innerLoops = 300;

    for (let i = 0; i < outerLoops; i++) {
      console.log(
        `\n========== 外层循环第 ${i + 1} / ${outerLoops} 次 ==========\n`,
      );
      for (let j = 0; j < tasklist.length; j++) {
        for (let k = 0; k < innerLoops; k++) {
          await runTask(tasklist[j], i, j, true);
          await runTask(
            `请你检查当前代码能否编译通过（cmake --build build --config RelWithDebInfo），
编译时间可能会非常长（若当前为macos系统，则该命令带上-j6后缀；若为Windows系统，则不带上任何表示构建并行度的后缀）
等待时间必须10分钟以上，若编译失败则必须修复知道能通过`,
            i, j, false);
          await runTask("请你检查当前是否有未提交的更改，若有则生成提交信息并提交", i, j, false);
        }
      }
      console.log(`\n========== 第 ${i + 1} 次循环结束 ==========\n`);
    }

    console.log("所有任务执行完毕。");
  }

  main().catch(console.error);

})();
