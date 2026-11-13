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
    COMMAND_FEEDBACK_QUIET_MS,
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
import { captureClientViews, type ClientViewOutcome } from "./observe/bot-view.ts";
import { installDatapack } from "./world/datapack.ts";
import {
    freezeCase,
    readBaseline,
    readBaselineCasesForMerge,
    thawCase,
    writeBaseline,
    type FrozenCase,
} from "./baseline/store.ts";
import { createBot, type BotHandle } from "../../shared/bot/factory.ts";
import { waitForEvent } from "../../shared/bot/wait.ts";
import { writeOpsFile } from "../../shared/servers/ops.ts";
import { allocateVerifiedPort } from "../../shared/servers/port.ts";
import { startCubiumServer } from "../../shared/servers/cubium.ts";
import { removeVanillaWorld, startVanillaServer } from "../../shared/servers/vanilla.ts";
import { makeRunId, vanillaCacheDir } from "../../shared/servers/workspace.ts";

/** 相邻棋盘之间的间隙。 */
const BOARD_GAP = 8;

/**
 * 玩家落脚点相对棋盘中心的 Z 偏移。
 *
 * 玩家与工作区必须分开：用例的 `{x}/{y}/{z}` 是工作区中心，而玩家站在旁边的小平台上，
 * 否则 `setblock {x} {y} {z}` 会把方块放进玩家身体里，`at @s` 的落点也会随时被用例改动。
 */
const PLAYER_ANCHOR_OFFSET_Z = -8;

/** 单条用例在单侧的执行结果。 */
export interface CaseRunResult {
    readonly caseId: string;
    readonly steps: readonly StepResult[];
    readonly probes: readonly ProbeOutcome[];
    readonly clientViews: readonly ClientViewOutcome[];
}

/** 单侧的执行结果。 */
export interface SideResult {
    readonly kind: ServerKind;
    readonly cases: readonly CaseRunResult[];
    readonly error?: string;
}

/** 运行模式。 */
export type RunMode = "diff" | "record" | "regress";

/** 一次运行的报告。 */
export interface CommandReport {
    readonly runId: string;
    readonly generatedAt: string;
    readonly mode: RunMode;
    /** 本次实际启动的一侧；regress 模式下没有 vanilla。 */
    readonly sides: {
        readonly cubium: SideResult;
        readonly vanilla?: SideResult;
    };
    readonly comparisons: readonly CaseComparison[];
}

/** 运行参数。 */
export interface RunnerOptions {
    /** 只跑 id 含该子串的用例；null 表示全部。 */
    readonly caseFilter: string | null;
    /** 运行模式。 */
    readonly mode: RunMode;
}

/**
 * 执行一次对比。
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

    const cubium = await runOneSide("cubium", cases, runId, runDir);
    let vanilla: SideResult | undefined;
    let vanillaComparables: Map<string, ComparableSide>;

    if (options.mode === "regress") {
        const baseline = readBaseline();
        if (baseline === null) {
            throw new Error("regress 模式需要先用 --mode=record 生成 baselines/vanilla.json");
        }
        vanillaComparables = new Map();
        for (const testCase of cases) {
            const frozen = baseline.cases[testCase.id];
            if (frozen === undefined) {
                throw new Error(`基线缺少用例 ${testCase.id}，请重新 record`);
            }
            vanillaComparables.set(testCase.id, thawCase(frozen));
        }
    } else {
        vanilla = await runOneSide("vanilla", cases, runId, runDir);
        vanillaComparables = new Map(
            vanilla.cases.map((item) => [item.caseId, toComparable(item)] satisfies [string, ComparableSide]),
        );
        if (options.mode === "record") {
            // 合并而非覆盖：`--case=` 只重录选中的用例，其余条目保持上次记录。
            const merged: Record<string, FrozenCase> = readBaselineCasesForMerge();
            for (const [caseId, side] of vanillaComparables) {
                merged[caseId] = freezeCase(side);
            }
            writeBaseline(merged);
        }
    }

    const report: CommandReport = {
        runId,
        generatedAt: new Date().toISOString(),
        mode: options.mode,
        sides: vanilla === undefined ? { cubium } : { cubium, vanilla },
        comparisons: compareAll(cases, cubium, vanillaComparables),
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
function selectCases(cases: readonly CommandCase[], caseFilter: string | null): readonly CommandCase[] {
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
    // 数据包必须在服务端启动前就位：vanilla 只在启动时扫描世界级 datapacks，
    // 而世界目录此刻尚未创建，需要先建好目录再放包。
    if (kind === "cubium") {
        installDatapack(path.join(sideDir, "datapacks"));
    } else {
        installDatapack(path.join(vanillaCacheDir(), vanillaWorldName, "datapacks"));
    }

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
    // 定位不计入判定：先把玩家脚下的平台补回去（上一条用例可能拆过），再传送过去。
    // 平台在 y-1、玩家站在 y，因此玩家 spawn 后不会因重力坠落导致坐标漂移。
    const anchor = playerAnchorFor(origin);
    await runStep(
        bot,
        `/setblock ${anchor.x} ${anchor.y - 1} ${anchor.z} minecraft:stone`,
        COMMAND_FEEDBACK_TIMEOUT_MS,
        COMMAND_FEEDBACK_QUIET_MS,
    );
    await runStep(
        bot,
        `/tp @s ${anchor.x} ${anchor.y} ${anchor.z} 0 0`,
        COMMAND_FEEDBACK_TIMEOUT_MS,
        COMMAND_FEEDBACK_QUIET_MS,
    );
    for (const command of testCase.setup) {
        await runStep(
            bot,
            substituteCoordinates(command, origin),
            COMMAND_FEEDBACK_TIMEOUT_MS,
            COMMAND_FEEDBACK_QUIET_MS,
        );
    }

    const steps: StepResult[] = [];
    for (const step of testCase.steps) {
        steps.push(
            await runStep(
                bot,
                substituteCoordinates(step.command, origin),
                COMMAND_FEEDBACK_TIMEOUT_MS,
                COMMAND_FEEDBACK_QUIET_MS,
            ),
        );
    }
    const probes = await runProbes(
        bot,
        testCase.probes,
        origin,
        COMMAND_FEEDBACK_TIMEOUT_MS,
        COMMAND_FEEDBACK_QUIET_MS,
    );
    const clientViews = await captureClientViews(bot, testCase.clientView, origin);
    return { caseId: testCase.id, steps, probes, clientViews };
}

/** bot 的最小形态（避免 runner 依赖 mineflayer 的具体类型）。 */
type BotLike = Parameters<typeof runStep>[0];

/** 按索引分配棋盘原点。 */
function boardOriginForCase(index: number): BoardOrigin {
    return boardOriginFor(index, BOARD_BASE_Y, BOARD_SIZE, BOARD_GAP);
}

/** 玩家落脚点（与工作区同一高度，沿 Z 让开）。 */
function playerAnchorFor(origin: BoardOrigin): BoardOrigin {
    return { x: origin.x, y: origin.y, z: origin.z + PLAYER_ANCHOR_OFFSET_Z };
}

/**
 * 逐用例比对两侧结果。
 *
 * @param cases 用例（顺序与执行结果一致）。
 * @param cubium Cubium 侧结果。
 * @param vanillaComparables vanilla 侧（或冻结基线）的可比形态，按用例 id 索引。
 * @returns 比对结果；Cubium 整体失败时为空。
 */
function compareAll(
    cases: readonly CommandCase[],
    cubium: SideResult,
    vanillaComparables: ReadonlyMap<string, ComparableSide>,
): readonly CaseComparison[] {
    if (cubium.error !== undefined) {
        return [];
    }
    const comparisons: CaseComparison[] = [];
    for (const [index, testCase] of cases.entries()) {
        const left = cubium.cases[index];
        const right = vanillaComparables.get(testCase.id);
        if (left === undefined || right === undefined) {
            continue;
        }
        comparisons.push(
            compareCase(
                {
                    caseId: testCase.id,
                    stepCommands: left.steps.map((step) => step.command),
                    probeCommands: new Map(left.probes.map((probe) => [probe.key, probe.command])),
                    viewKinds: new Map(left.clientViews.map((view) => [view.key, view.kind])),
                    knownDiffs: testCase.knownDiffs,
                },
                toComparable(left),
                right,
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
        viewTexts: new Map(side.clientViews.map((view) => [view.key, view.text])),
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
    console.log(`\nrunId: ${report.runId}（mode=${report.mode}）`);

    const cubiumSide = report.sides.cubium;
    console.log(
        cubiumSide.error === undefined
            ? `\n[cubium] ${cubiumSide.cases.length} 条用例`
            : `\n[cubium] ERROR: ${cubiumSide.error}`,
    );
    const vanillaSide = report.sides.vanilla;
    if (vanillaSide === undefined) {
        console.log("[vanilla] 使用冻结基线（未启动服务端）");
    } else {
        console.log(
            vanillaSide.error === undefined
                ? `[vanilla] ${vanillaSide.cases.length} 条用例`
                : `[vanilla] ERROR: ${vanillaSide.error}`,
        );
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
