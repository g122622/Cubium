/*
 * run.ts — e2e 测试的唯一入口。
 *
 * 用法（node >= 22 原生 type stripping，ESM，运行时零外部依赖）：
 *   node run.ts                              回归比对（默认：只跑 Cubium，与冻结基线比）
 *   node run.ts --mode=refresh --accept      刷新基线（仅在全部用例通过时写入）
 *   node run.ts --mode=diff                  双跑对比（Cubium vs vanilla）
 *   node run.ts --case=handshake/spawn       只跑 id 含该子串的用例
 *   node run.ts --keep-artifacts             保留成功用例的工作目录
 *   node run.ts --with-vanilla               把 vanilla 也纳入本次运行范围
 *
 * 退出码：0 全部通过；1 有用例失败；2 环境/基线问题（如基线缺失或过期）。
 */

import { runCases, type RunMode } from "./src/runner.ts";
import { ALL_CASES } from "./src/scenarios/index.ts";
import type { ServerKind } from "./src/case.ts";

interface CliOptions {
    mode: RunMode;
    caseFilter: string | null;
    accept: boolean;
    keepArtifacts: boolean;
    withVanilla: boolean;
}

function parseArgs(argv: readonly string[]): CliOptions {
    let mode: RunMode = "regress";
    let caseFilter: string | null = null;
    let accept = false;
    let keepArtifacts = false;
    let withVanilla = false;

    for (const arg of argv) {
        if (arg.startsWith("--mode=")) {
            const value = arg.slice("--mode=".length);
            if (value !== "regress" && value !== "refresh" && value !== "diff") {
                throw new Error(`未知的 mode：${value}（可选 regress / refresh / diff）`);
            }
            mode = value;
        } else if (arg.startsWith("--case=")) {
            caseFilter = arg.slice("--case=".length);
        } else if (arg === "--accept") {
            accept = true;
        } else if (arg === "--keep-artifacts") {
            keepArtifacts = true;
        } else if (arg === "--with-vanilla") {
            withVanilla = true;
        } else if (arg === "--help" || arg === "-h") {
            console.log(
                [
                    "用法：node run.ts [选项]",
                    "  --mode=regress|refresh|diff   运行模式（默认 regress）",
                    "  --case=<子串>                 只跑 id 含该子串的用例",
                    "  --accept                      refresh 模式下确认写入基线",
                    "  --keep-artifacts              保留成功用例的工作目录",
                    "  --with-vanilla                把 vanilla 服务端纳入运行范围",
                ].join("\n"),
            );
            process.exit(0);
        } else {
            throw new Error(`未知参数：${arg}（--help 查看用法）`);
        }
    }
    // diff 模式必须两侧都跑；其余模式由 --with-vanilla 决定。
    const withVanillaEffective = withVanilla || mode === "diff";
    void withVanillaEffective;
    return { mode, caseFilter, accept, keepArtifacts, withVanilla: withVanillaEffective };
}

async function main(): Promise<void> {
    const cli = parseArgs(process.argv.slice(2));
    // 日常回归只启 Cubium；vanilla 仅在显式要求时启动（它需要 java 且启动更慢）。
    const enabledServers: ServerKind[] = cli.withVanilla ? ["cubium", "vanilla"] : ["cubium"];

    const summary = await runCases(ALL_CASES, {
        mode: cli.mode,
        caseFilter: cli.caseFilter,
        accept: cli.accept,
        keepArtifacts: cli.keepArtifacts,
        enabledServers,
    });
    if (summary.exitCode !== 0) {
        console.log(`诊断产物目录：build/e2e/artifacts/${summary.runId}/`);
    }
    process.exit(summary.exitCode);
}

await main();
