/*
 * runner.ts — 指令端到端 diff 的编排骨架。
 *
 * 一条 run 的生命周期：
 *   分配端口 → 建独立工作目录 → 写离线 OP → 启动服务端 → 连 bot → 逐条执行用例
 *   → 采集反馈 → 落盘报告 → 停服并清理。
 *
 * 首版只有冒烟用例，验证「两侧能同时跑、反馈能采集、差异能定位」这条链路；
 * 用例加载、归一化与 knownDiff 在后续阶段接入。
 */

import fs from "node:fs";
import path from "node:path";

import mineflayer from "mineflayer";

import { runStep, type StepResult } from "./driver/chat-driver.ts";
import {
    COMMAND_BOT_USERNAME,
    COMMAND_FEEDBACK_TIMEOUT_MS,
    COMMAND_MAX_PLAYERS,
    COMMAND_PROFILE,
    COMMAND_SPAWN_TIMEOUT_MS,
    COMMAND_WORK_ROOT,
    type ServerKind,
} from "./config.ts";
import { createBot, type BotHandle } from "../../shared/bot/factory.ts";
import { waitForEvent } from "../../shared/bot/wait.ts";
import { writeOpsFile } from "../../shared/servers/ops.ts";
import { allocateVerifiedPort } from "../../shared/servers/port.ts";
import { startCubiumServer } from "../../shared/servers/cubium.ts";
import { removeVanillaWorld, startVanillaServer } from "../../shared/servers/vanilla.ts";
import { makeRunId, vanillaCacheDir } from "../../shared/servers/workspace.ts";

/** 一条用例：一段按顺序执行的指令序列。 */
export interface CommandCase {
    /** 用例标识，`/` 分隔分类；`--case` 按子串匹配。 */
    readonly id: string;
    readonly commands: readonly string[];
}

/**
 * 首版用例：覆盖传送、方块写入、条件判断与广播。
 *
 * 这就是 diff 的判定单元——同一段指令在两侧的结果必须同形。
 */
export const CASES: readonly CommandCase[] = [
    {
        id: "smoke/basic_feedback",
        commands: [
            "/tp @s 0 100 0 0 0",
            "/setblock 0 100 0 minecraft:stone",
            "/execute if block 0 100 0 minecraft:stone run say e2e-ok",
            "/setblock 0 100 0 minecraft:air",
        ],
    },
];

/** 单条用例在单侧的执行结果。 */
export interface CaseResult {
    readonly caseId: string;
    readonly steps: readonly StepResult[];
}

/** 单侧的执行结果。 */
export interface SideResult {
    readonly kind: ServerKind;
    readonly cases: readonly CaseResult[];
    readonly error?: string;
}

/** 一次运行的报告。 */
export interface CommandReport {
    readonly runId: string;
    readonly generatedAt: string;
    readonly results: Readonly<Record<ServerKind, SideResult>>;
}

/** 运行参数。 */
export interface RunnerOptions {
    /** 只跑 id 含该子串的用例；null 表示全部。 */
    readonly caseFilter: string | null;
}

/**
 * 执行一次两侧对比。
 *
 * @param options 运行参数。
 * @returns 运行报告（同时已落盘）。
 */
export async function runCommandHarness(options: RunnerOptions): Promise<CommandReport> {
    const cases = selectCases(options.caseFilter);
    const runId = makeRunId();
    const runDir = path.join(COMMAND_WORK_ROOT, "runs", runId);
    fs.mkdirSync(runDir, { recursive: true });

    const results: Partial<Record<ServerKind, SideResult>> = {};
    for (const kind of ["cubium", "vanilla"] as const) {
        results[kind] = await runOneSide(kind, cases, runId, runDir);
    }

    const report: CommandReport = {
        runId,
        generatedAt: new Date().toISOString(),
        results: results as Record<ServerKind, SideResult>,
    };
    writeReport(runId, report, runDir);
    return report;
}

/**
 * 按子串筛选用例。
 *
 * 过滤器匹配不到任何用例时直接报错：静默跑 0 条会让「过滤词打错」看起来像通过。
 *
 * @param caseFilter 子串；null 表示不过滤。
 * @returns 选中的用例。
 */
function selectCases(caseFilter: string | null): readonly CommandCase[] {
    if (caseFilter === null) {
        return CASES;
    }
    const selected = CASES.filter((item) => item.id.includes(caseFilter));
    if (selected.length === 0) {
        throw new Error(`没有 id 含 "${caseFilter}" 的用例（现有：${CASES.map((item) => item.id).join(", ")}）`);
    }
    return selected;
}

/**
 * 在单侧服务端上执行全部选中用例。
 *
 * @param kind 服务端种类。
 * @param cases 选中的用例。
 * @param runId 本次运行 id。
 * @param runDir 本次运行的根目录。
 * @returns 单侧结果；启动/连接失败时 error 字段带原因。
 */
async function runOneSide(
    kind: ServerKind,
    cases: readonly CommandCase[],
    runId: string,
    runDir: string,
): Promise<SideResult> {
    const sideDir = path.join(runDir, kind);
    fs.mkdirSync(sideDir, { recursive: true });

    const port = await allocateVerifiedPort(20, 400);
    const opsPath =
        kind === "cubium" ? path.join(sideDir, "ops.json") : path.join(vanillaCacheDir(), "ops.json");
    writeOpsFile(opsPath, [COMMAND_BOT_USERNAME], kind);

    const vanillaWorldName = `command-${runId}`;
    const server =
        kind === "cubium"
            ? await startCubiumServer({
                  runDir: sideDir,
                  port,
                  profile: COMMAND_PROFILE,
                  maxPlayers: COMMAND_MAX_PLAYERS,
              })
            : await startVanillaServer({
                  runDir: sideDir,
                  port,
                  profile: COMMAND_PROFILE,
                  maxPlayers: COMMAND_MAX_PLAYERS,
                  worldName: vanillaWorldName,
              });

    let handle: BotHandle | null = null;
    try {
        handle = createBot(
            {
                port,
                username: COMMAND_BOT_USERNAME,
                traceLimit: 2_000,
            },
            mineflayer.createBot,
        );
        await waitForEvent(handle.bot, "spawn", {
            timeoutMs: COMMAND_SPAWN_TIMEOUT_MS,
            what: `${kind} bot spawn`,
        });

        const caseResults: CaseResult[] = [];
        for (const testCase of cases) {
            const steps: StepResult[] = [];
            for (const command of testCase.commands) {
                steps.push(await runStep(handle.bot, command, COMMAND_FEEDBACK_TIMEOUT_MS));
            }
            caseResults.push({ caseId: testCase.id, steps });
        }
        return { kind, cases: caseResults };
    } catch (error) {
        return {
            kind,
            cases: [],
            error: error instanceof Error ? error.message : String(error),
        };
    } finally {
        await handle?.dispose();
        await server.stop();
        if (kind === "vanilla") {
            removeVanillaWorld(vanillaWorldName);
        }
    }
}

/**
 * 把报告与两侧服务端日志写入诊断目录。
 *
 * 日志必须复制进 artifacts：`runs/` 目录会按保留数量轮转，而报告引用的诊断现场
 * 应当与报告同寿。
 *
 * @param runId 本次运行 id。
 * @param report 报告。
 * @param runDir 本次运行的根目录。
 */
function writeReport(runId: string, report: CommandReport, runDir: string): void {
    const dir = path.join(COMMAND_WORK_ROOT, "artifacts", runId);
    fs.mkdirSync(dir, { recursive: true });
    fs.writeFileSync(path.join(dir, "report.json"), `${JSON.stringify(report, null, 2)}\n`, "utf8");

    for (const kind of ["cubium", "vanilla"] as const) {
        const logPath = path.join(runDir, kind, "server.log");
        if (!fs.existsSync(logPath)) {
            continue;
        }
        fs.mkdirSync(path.join(dir, kind), { recursive: true });
        fs.copyFileSync(logPath, path.join(dir, kind, "server.log"));
    }
}

/**
 * 打印报告摘要。
 *
 * @param report 运行报告。
 */
export function printReport(report: CommandReport): void {
    console.log(`\nrunId: ${report.runId}`);
    for (const kind of ["cubium", "vanilla"] as const) {
        const side = report.results[kind];
        console.log(`\n[${kind}]`);
        if (side.error !== undefined) {
            console.log(`  ERROR: ${side.error}`);
            continue;
        }
        for (const item of side.cases) {
            console.log(`  ${item.caseId}`);
            for (const step of item.steps) {
                const text = step.feedback.map((entry) => entry.text).join(" | ");
                console.log(`    ${step.command}`);
                console.log(`      -> ${step.timedOut ? "<无反馈>" : text} (${step.elapsedMs}ms)`);
            }
        }
    }

    const diffs = collectDiffs(report);
    console.log(diffs.length === 0 ? "\n反馈文本：两侧一致。" : `\n反馈差异（${diffs.length}）：\n${diffs.join("\n")}`);
}

/**
 * 收集两侧反馈差异（逐用例逐步骤比对）。
 *
 * @param report 运行报告。
 * @returns 差异描述行；空数组表示无差异或数据不完整。
 */
export function collectDiffs(report: CommandReport): string[] {
    const cubium = report.results.cubium;
    const vanilla = report.results.vanilla;
    if (cubium.error !== undefined || vanilla.error !== undefined) {
        return [];
    }
    const lines: string[] = [];
    const caseCount = Math.min(cubium.cases.length, vanilla.cases.length);
    for (let caseIndex = 0; caseIndex < caseCount; caseIndex += 1) {
        const left = cubium.cases[caseIndex];
        const right = vanilla.cases[caseIndex];
        if (left === undefined || right === undefined) {
            continue;
        }
        const stepCount = Math.max(left.steps.length, right.steps.length);
        for (let stepIndex = 0; stepIndex < stepCount; stepIndex += 1) {
            const leftText = left.steps[stepIndex]?.feedback.map((entry) => entry.text).join("\n") ?? "<无此步>";
            const rightText = right.steps[stepIndex]?.feedback.map((entry) => entry.text).join("\n") ?? "<无此步>";
            if (leftText === rightText) {
                continue;
            }
            lines.push(
                `  ${left.caseId} #${stepIndex} ${left.steps[stepIndex]?.command ?? ""}` +
                    `\n    cubium: ${leftText}\n    vanilla: ${rightText}`,
            );
        }
    }
    return lines;
}
