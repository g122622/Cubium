/*
 * runner.ts — 指令端到端 diff 的编排。
 *
 * 一条 run 的生命周期：
 *   加载用例 → 分配棋盘 → 两侧各自：启动服务端 → 连 bot → 逐用例执行
 *   → 归一化 → 逐步骤比对 → 落盘报告 → 停服清理。
 *
 * 两侧**串行**运行且共享同一份用例与棋盘坐标：并行会让端口、世界目录与 bot 日志互相
 * 干扰，而指令 diff 的规模还远没到需要并行的时候。
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
    BOARD_BASE_Y,
    BOARD_SIZE,
    type ServerKind,
} from "./config.ts";
import {
    boardOriginFor,
    loadCases,
    substituteCoordinates,
    type BoardOrigin,
    type CommandCase,
} from "./case.ts";
import { normalizeFeedback } from "./compare/normalize.ts";
import { compareCase, partitionDiffs, type CaseComparison, type ComparableSide } from "./compare/diff.ts";
import { runProbes, type ProbeOutcome } from "./observe/probes.ts";
import { createBot, type BotHandle } from "../../shared/bot/factory.ts";
import { waitForEvent } from "../../shared/bot/wait.ts";
import { writeOpsFile } from "../../shared/servers/ops.ts";
import { allocateVerifiedPort } from "../../shared/servers/port.ts";
import { startCubiumServer } from "../../shared/servers/cubium.ts";
import { removeVanillaWorld, startVanillaServer } from "../../shared/servers/vanilla.ts";
import { makeRunId, vanillaCacheDir } from "../../shared/servers/workspace.ts";

/** 相邻棋盘之间的间隙。 */
const BOARD_GAP = 8;

/** 单条用例在单侧的执行结果。 */
export interface CaseRunResult {
    readonly caseId: string;
    readonly steps: readonly StepResult[];
    readonly probes: readonly ProbeOutcome[];
}

/** 单侧的执行结果。 */
export interface SideResult {
    readonly kind: ServerKind;
    readonly cases: readonly CaseRunResult[];
    readonly error?: string;
}

/** 一次运行的报告。 */
export interface CommandReport {
    readonly runId: string;
    readonly generatedAt: string;
    readonly sides: Readonly<Record<ServerKind, SideResult>>;
    readonly comparisons: readonly CaseComparison[];
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
    const cases = selectCases(loadCases(), options.caseFilter);
    if (cases.length === 0) {
        throw new Error("没有可执行的用例：cases/ 目录为空");
    }

    const runId = makeRunId();
    const runDir = path.join(COMMAND_WORK_ROOT, "runs", runId);
    fs.mkdirSync(runDir, { recursive: true });

    const sides: Partial<Record<ServerKind, SideResult>> = {};
    for (const kind of ["cubium", "vanilla"] as const) {
        sides[kind] = await runOneSide(kind, cases, runId, runDir);
    }
    const cubium = sides["cubium"]!;
    const vanilla = sides["vanilla"]!;

    const report: CommandReport = {
        runId,
        generatedAt: new Date().toISOString(),
        sides: { cubium, vanilla },
        comparisons: compareAll(cases, cubium, vanilla),
    };
    writeReport(runId, report, runDir);
    return report;
}

/**
 * 按子串筛选用例。
 *
 * 过滤器匹配不到任何用例时直接报错：静默跑 0 条会让「过滤词打错」看起来像通过。
 *
 * @param cases 全部用例。
 * @param caseFilter 子串；null 表示不过滤。
 * @returns 选中的用例。
 */
export function selectCases(cases: readonly CommandCase[], caseFilter: string | null): readonly CommandCase[] {
    if (caseFilter === null) {
        return cases;
    }
    const selected = cases.filter((item) => item.id.includes(caseFilter));
    if (selected.length === 0) {
        throw new Error(
            `没有 id 含 "${caseFilter}" 的用例（现有：${cases.map((item) => item.id).join(", ") || "无"}）`,
        );
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
                traceLimit: 4_000,
            },
            mineflayer.createBot,
        );
        await waitForEvent(handle.bot, "spawn", {
            timeoutMs: COMMAND_SPAWN_TIMEOUT_MS,
            what: `${kind} bot spawn`,
        });

        const caseResults: CaseRunResult[] = [];
        for (const [index, testCase] of cases.entries()) {
            caseResults.push(await runOneCase(handle.bot, testCase, boardOriginForCase(index)));
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
 * 执行单条用例：先把 bot 钉到棋盘中心，再跑 setup、steps 与 probes。
 *
 * @param bot 已就绪的 bot。
 * @param testCase 用例。
 * @param origin 本用例的棋盘原点。
 * @returns 执行结果。
 */
async function runOneCase(bot: BotLike, testCase: CommandCase, origin: BoardOrigin): Promise<CaseRunResult> {
    // 传送本身不计入判定：两侧的 tp 反馈文案存在已知差异，而它只是棋盘定位手段。
    await runStep(
        bot,
        substituteCoordinates("/tp @s {x} {y} {z} 0 0", origin),
        COMMAND_FEEDBACK_TIMEOUT_MS,
    );
    for (const command of testCase.setup) {
        await runStep(bot, substituteCoordinates(command, origin), COMMAND_FEEDBACK_TIMEOUT_MS);
    }

    const steps: StepResult[] = [];
    for (const step of testCase.steps) {
        steps.push(await runStep(bot, substituteCoordinates(step.command, origin), COMMAND_FEEDBACK_TIMEOUT_MS));
    }
    const probes = await runProbes(bot, testCase.probes, origin, COMMAND_FEEDBACK_TIMEOUT_MS);
    return { caseId: testCase.id, steps, probes };
}

/** bot 的最小形态（避免 runner 依赖 mineflayer 的具体类型）。 */
type BotLike = Parameters<typeof runStep>[0];

/** 按索引分配棋盘原点。 */
function boardOriginForCase(index: number): BoardOrigin {
    return boardOriginFor(index, BOARD_BASE_Y, BOARD_SIZE, BOARD_GAP);
}

/**
 * 逐用例比对两侧结果。
 *
 * @param cases 用例（顺序与执行结果一致）。
 * @param cubium Cubium 侧结果。
 * @param vanilla vanilla 侧结果。
 * @returns 比对结果；任一侧整体失败时为空。
 */
function compareAll(
    cases: readonly CommandCase[],
    cubium: SideResult,
    vanilla: SideResult,
): readonly CaseComparison[] {
    if (cubium.error !== undefined || vanilla.error !== undefined) {
        return [];
    }
    const comparisons: CaseComparison[] = [];
    for (const [index, testCase] of cases.entries()) {
        const left = cubium.cases[index];
        const right = vanilla.cases[index];
        if (left === undefined || right === undefined) {
            continue;
        }
        comparisons.push(
            compareCase(
                {
                    caseId: testCase.id,
                    stepCommands: left.steps.map((step) => step.command),
                    probeCommands: new Map(left.probes.map((probe) => [probe.key, probe.command])),
                    knownDiffs: testCase.knownDiffs,
                },
                toComparable(left),
                toComparable(right),
            ),
        );
    }
    return comparisons;
}

/** 把一侧结果转成可比较形态。 */
function toComparable(side: CaseRunResult): ComparableSide {
    return {
        stepTexts: side.steps.map((step) => normalizeFeedback(step.feedback)),
        stepTimeouts: side.steps.map((step) => step.timedOut),
        probeTexts: new Map(side.probes.map((probe) => [probe.key, probe.text])),
    };
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
 * 统计新差异数量（已登记的 knownDiff 不计入）。
 *
 * @param report 运行报告。
 * @returns 新差异条数。
 */
export function countUnknownDiffs(report: CommandReport): number {
    return report.comparisons.reduce((total, item) => total + partitionDiffs(item).unknown.length, 0);
}

/**
 * 打印报告摘要。
 *
 * @param report 运行报告。
 */
export function printReport(report: CommandReport): void {
    console.log(`\nrunId: ${report.runId}`);

    for (const kind of ["cubium", "vanilla"] as const) {
        const side = report.sides[kind];
        if (side.error !== undefined) {
            console.log(`\n[${kind}] ERROR: ${side.error}`);
            continue;
        }
        console.log(`\n[${kind}] ${side.cases.length} 条用例`);
    }

    let unknownTotal = 0;
    let knownTotal = 0;
    let staleTotal = 0;
    for (const comparison of report.comparisons) {
        const { known, unknown } = partitionDiffs(comparison);
        if (known.length === 0 && unknown.length === 0 && comparison.staleKnownDiffs.length === 0) {
            console.log(`\n✓ ${comparison.caseId}`);
            continue;
        }

        console.log(`\n${unknown.length === 0 ? "≈" : "✗"} ${comparison.caseId}`);
        for (const diff of unknown) {
            console.log(`  [新差异] ${diff.path} (${diff.category}) ${diff.command}`);
            console.log(`    cubium: ${formatText(diff.cubium)}`);
            console.log(`    vanilla: ${formatText(diff.vanilla)}`);
        }
        for (const diff of known) {
            console.log(`  [已登记] ${diff.path} ${diff.known?.reason ?? ""}`);
        }
        for (const stale of comparison.staleKnownDiffs) {
            console.log(`  [登记过期] ${stale.path} —— 本次未出现该差异，可删除登记`);
        }
        unknownTotal += unknown.length;
        knownTotal += known.length;
        staleTotal += comparison.staleKnownDiffs.length;
    }

    console.log(
        `\n合计：新差异 ${unknownTotal}，已登记 ${knownTotal}，登记过期 ${staleTotal}。` +
            `\n报告：build/e2e/command/artifacts/${report.runId}/report.json`,
    );
}

/** 多行文本在报告里压成一行，避免刷屏。 */
function formatText(text: string): string {
    return text.length === 0 ? "<无反馈>" : text.replace(/\n/g, " ⏎ ");
}
