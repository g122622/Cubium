import { query } from "@anthropic-ai/claude-agent-sdk";

const tasklist = [
  "/align 地形生成过程中的结构生成算法与模板解析等（提示：结构生成的nbt模板位于/Users/a0000/MC_Dev/resourcePacks/Vanilla/data/minecraft）",
  "/align 含水方块",
  "/align 实体ai",
  "/align 音效丰富度",
  "/align 物品丰富度",
  "/align 矿车系统",
  "/align 船系统",
  "/align 骑乘系统",
  "/align 天气系统",
  "/align 下界与主世界之间的传送",
];

async function runTask(task: string, iteration: number, taskIndex: number) {
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
  const outerLoops = 3;

  for (let i = 0; i < outerLoops; i++) {
    console.log(
      `\n========== 外层循环第 ${i + 1} / ${outerLoops} 次 ==========\n`,
    );
    for (let j = 0; j < tasklist.length; j++) {
      await runTask(tasklist[j], i, j);
      await runTask("请你检查当前代码能否编译通过（cmake --build build --config RelWithDebInfo -j6），编译时间可能会非常长，等待时间必须10分钟以上，若编译失败则必须修复知道能通过", i, j);
      await runTask("请你检查当前是否有未提交的更改，若有则生成提交信息并提交", i, j);
    }
    console.log(`\n========== 第 ${i + 1} 次循环结束 ==========\n`);
  }

  console.log("所有任务执行完毕。");
}

main().catch(console.error);
