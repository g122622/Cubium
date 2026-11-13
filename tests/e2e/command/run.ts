/*
 * run.ts — 指令端到端 diff 的唯一入口。
 *
 * 用法（node >= 24 原生 type stripping，ESM）：
 *   node run.ts                         双跑对比（Cubium vs vanilla），默认模式
 *   node run.ts --mode=record           双跑并刷新 vanilla 基线 baselines/vanilla.json
 *   node run.ts --mode=regress          只跑 Cubium，与冻结基线比对（快，日常回归用）
 *   node run.ts --case=smoke            只跑 id 含该子串的用例
 *   node run.ts --strict                出现未登记差异时以非零码退出
 *
 * 退出码：0 两侧都完整跑完且无未登记差异（或未开 --strict）；1 至少一侧启动/连接/
 * 执行失败，或 --strict 下出现未登记差异。
 */

import { countUnknownDiffs, printReport, runCommandHarness, type RunMode } from "./src/runner.ts";

interface CliOptions {
    caseFilter: string | null;
    mode: RunMode;
    strict: boolean;
}

/**
 * 解析命令行参数。
 *
 * @param argv 去掉 node 与脚本路径后的参数列表。
 * @returns 解析结果。
 */
function parseArgs(argv: readonly string[]): CliOptions {
    let caseFilter: string | null = null;
    let mode: RunMode = "diff";
    let strict = false;

    for (const arg of argv) {
        if (arg.startsWith("--case=")) {
            caseFilter = arg.slice("--case=".length);
        } else if (arg.startsWith("--mode=")) {
            const value = arg.slice("--mode=".length);
            if (value !== "diff" && value !== "record" && value !== "regress") {
                throw new Error(`未知的 mode：${value}（可选 diff / record / regress）`);
            }
            mode = value;
        } else if (arg === "--strict") {
            strict = true;
        } else if (arg === "--help" || arg === "-h") {
            console.log(
                [
                    "用法：node run.ts [选项]",
                    "  --mode=diff|record|regress  运行模式（默认 diff）",
                    "  --case=<子串>               只跑 id 含该子串的用例",
                    "  --strict                    出现未登记差异时以非零码退出",
                ].join("\n"),
            );
            process.exit(0);
        } else {
            throw new Error(`未知参数：${arg}（--help 查看用法）`);
        }
    }
    return { caseFilter, mode, strict };
}

/**
 * 入口。启动服务端、执行用例、打印并落盘报告。
 */
async function main(): Promise<void> {
    const cli = parseArgs(process.argv.slice(2));
    const report = await runCommandHarness({ caseFilter: cli.caseFilter, mode: cli.mode });
    printReport(report);

    const failed = report.sides.cubium.error !== undefined || report.sides.vanilla?.error !== undefined;
    const unknown = countUnknownDiffs(report);
    if (failed || (cli.strict && unknown > 0)) {
        process.exit(1);
    }
    process.exit(0);
}

try {
    await main();
} catch (error) {
    console.error(error instanceof Error ? error.message : String(error));
    process.exit(1);
}
