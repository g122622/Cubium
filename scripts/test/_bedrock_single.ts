// 临时：在基岩 BDS 单独跑指定 GameTest 用例，验证基岩行为（权威基准）。
// 用法: node scripts/test/_bedrock_single.ts "<Class:name>" ["<Class:name2>" ...]
import { spawn } from "node:child_process";
import { existsSync } from "node:fs";

const BEDROCK_DIR = "D:/Minecraft/bedrock-server-1.26.43.1";
const BEDROCK_EXE = BEDROCK_DIR + "/bedrock_server.exe";
const tests = process.argv.slice(2);

if (!existsSync(BEDROCK_EXE)) { console.error("bedrock exe missing"); process.exit(2); }
if (tests.length === 0) { console.error("usage: _bedrock_single.ts <Class:name>..."); process.exit(2); }

const proc = spawn(BEDROCK_EXE, [], { cwd: BEDROCK_DIR, stdio: ["pipe", "pipe", "pipe"] });
let stdout = "";
let idx = 0;
const done = new Set<string>();
let stopped = false;

proc.stdout.on("data", (d) => {
  const chunk = d.toString("utf-8");
  stdout += chunk;
  process.stdout.write(chunk);
  // 世界就绪后发第一个测试
  if (/Server started\./i.test(stdout) && idx === 0) {
    setTimeout(() => { console.log("\n[send] gametest run " + tests[0]); proc.stdin.write(`gametest run ${tests[0]}\n`); idx = 1; }, 1500);
  }
  // 检测完成
  for (const t of tests) {
    if (done.has(t)) continue;
    const re = new RegExp("(onTestPassed|onTestFailed|Failed to run|Could not find|Tests failed|Passed without error).*" + t.replace(/[.*+?^${}()|[\]\\]/g, "\\$&"), "i");
    if (re.test(chunk) || (/(onTestPassed|onTestFailed)/i.test(chunk) && idx <= tests.length)) {
      // 简化：onTestPassed/Failed 出现即认为当前测试完成
    }
  }
  if (/(onTestPassed|onTestFailed|Tests failed|Failed to run)/i.test(chunk) && idx < tests.length && !stopped) {
    const next = tests[idx];
    console.log("\n[send] gametest run " + next);
    proc.stdin.write(`gametest run ${next}\n`);
    idx++;
  }
  if (idx >= tests.length && /Tests failed|onTestPassed|onTestFailed/i.test(chunk) && !stopped) {
    // 全部发完，等最后一个结果后 stop
    setTimeout(() => { if (!stopped) { stopped = true; proc.stdin.write("stop\n"); } }, 8000);
  }
});
proc.stderr.on("data", (d) => { process.stderr.write(d); });
proc.on("close", (c) => { console.log("\n[bedrock exit " + c + "]"); });

// 总超时
setTimeout(() => { if (!stopped) { stopped = true; try { proc.stdin.write("stop\n"); } catch {} } }, 180000);
