/*
 * run.ts — 指令端到端 diff 的唯一入口。
 *
 * 用法（node >= 24 原生 type stripping，ESM）：
 *   node run.ts                     跑全部用例（当前只有冒烟用例）
 *   node run.ts --case=smoke        只跑 id 含该子串的用例
 *   node run.ts --strict            两侧反馈出现差异时以非零码退出
 *
 * 退出码：0 两侧都完整跑完；1 至少一侧启动/连接/执行失败（或 --strict 下有差异）。
 */

import { collectDiffs, printReport, runCommandHarness } from "./src/runner.ts";

interface CliOptions {
    caseFilter: string | null;
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
    let strict = false;

    for (const arg of argv) {
        if (arg.startsWith("--case=")) {
            caseFilter = arg.slice("--case=".length);
        } else if (arg === "--strict") {
            strict = true;
        } else if (arg === "--help" || arg === "-h") {
            console.log(
                [
                    "用法：node run.ts [选项]",
                    "  --case=<子串>   只跑 id 含该子串的用例",
                    "  --strict        两侧反馈出现差异时以非零码退出",
                ].join("\n"),
            );
            process.exit(0);
        } else {
            throw new Error(`未知参数：${arg}（--help 查看用法）`);
        }
    }
    return { caseFilter, strict };
}

/**
 * 入口。启动两侧服务端、执行用例、打印并落盘报告。
 */
async function main(): Promise<void> {
    const cli = parseArgs(process.argv.slice(2));
    const report = await runCommandHarness({ caseFilter: cli.caseFilter });
    printReport(report);

    const failed = report.results.cubium.error !== undefined || report.results.vanilla.error !== undefined;
    const hasDiff = collectDiffs(report).length > 0;
    if (failed || (cli.strict && hasDiff)) {
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
