/*
 * runner.ts — 用例编排。
 *
 * 每个用例的完整生命周期：
 *   分配端口 → 建独立工作目录 → 起服务端 → 连 bot → 等 spawn → 探测地表基准
 *   → 执行用例 → 采快照 → 比对/写入基线 → 回收（失败时先落诊断产物）
 *
 * 隔离策略：每用例一个服务端进程 + 一个独立游戏目录（用完即删），
 * 因此用例之间不存在方块/实体/世界状态的互相污染。
 */

import { execFileSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import type { Bot } from "mineflayer";
import {
    BASELINE_DIR,
    CASE_TIMEOUT_MS,
    E2E_SERVER_PROFILE,
    REPO_ROOT,
    SPAWN_TIMEOUT_MS,
} from "./config.ts";
import type { CaseContext, CaseDefinition, ServerKind } from "./case.ts";
import { createBot, type BotHandle } from "./bot/factory.ts";
import { TimeoutError, BotFailureError, waitForCondition, waitForEvent } from "./bot/wait.ts";
import { captureSnapshot, type CaseSnapshot } from "./bot/snapshot.ts";
import { AssertionError } from "./assert/expect.ts";
import { detectSurfaceY, waitForAreaLoaded } from "./assert/surface.ts";
import { startCubiumServer } from "./servers/cubium.ts";
import { removeVanillaWorld, startVanillaServer } from "./servers/vanilla.ts";
import type { ServerProcess } from "./servers/server-process.ts";
import { allocatePort } from "./servers/port.ts";
import { artifactDirFor, makeRunId, pruneArtifacts, removeDirQuietly, runDirFor } from "./servers/workspace.ts";
import { compareSnapshot, renderDifferences } from "./baseline/compare.ts";
import {
    baselinePath,
    loadBaseline,
    makeBaseline,
    saveBaseline,
    type Baseline,
    type BaselineEntry,
} from "./baseline/store.ts";
import { writeArtifacts } from "./diagnostics/artifacts.ts";

/** 运行模式。 */
export type RunMode = "regress" | "refresh" | "diff";

/** 运行参数。 */
export interface RunnerOptions {
    readonly mode: RunMode;
    /** 用例 id 子串过滤（null 表示全部）。 */
    readonly caseFilter: string | null;
    /** refresh 模式是否已确认（未确认时拒绝写基线）。 */
    readonly accept: boolean;
    /** 是否保留成功用例的诊断产物。 */
    readonly keepArtifacts: boolean;
    /**
     * 本次实际启动哪些服务端。
     *
     * 日常回归只含 cubium（vanilla 仅用于刷基线与双跑对比，见 Phase 4）。
     * 用例自身的 servers 字段声明「适用于哪些服务端」，两者取交集决定实际运行组合。
     */
    readonly enabledServers: readonly ServerKind[];
}

/** 单条用例的结果。 */
export interface CaseResult {
    readonly id: string;
    readonly title: string;
    readonly serverKind: ServerKind;
    readonly ok: boolean;
    readonly failureKind: string;
    readonly failureMessage: string;
    readonly elapsedMs: number;
    readonly snapshot: CaseSnapshot | null;
}

const TRACE_LIMIT = 20_000;

/**
 * 抑制 protodef 的 TCP 粘包调试噪声。
 *
 * protodef/src/serializer.js:76 在「一次 TCP 读取包含多个包」时打印
 * "Chunk size is N but only M was read ; partial packet ..."。这是**正常的粘包现象**
 * （nmp 会自行分流剩余字节，不影响解析），但每个用例都会刷出数十行，淹没用例结果。
 *
 * 只过滤这一条已知消息——mineflayer 的区块加载警告、协议错误等一律保持可见
 * （这也是 hideErrors 保持 false 的原因）。
 */
function installNoiseFilter(): void {
    const originalLog = console.log.bind(console);
    console.log = (...args: unknown[]): void => {
        const first = args[0];
        if (typeof first === "string" && first.startsWith("Chunk size is ")) {
            return;
        }
        originalLog(...args);
    };
}

/**
 * 用例执行前等待加载完成的区域半径。
 *
 * 取 3（即 7x7 列）：既覆盖各用例最大的采样范围（5x5），又远小于 viewDistance=4 的
 * 推送范围（9x9），不至于因边缘列延迟而超时。
 */
const LOADED_AREA_RADIUS = 3;
const LOADED_AREA_TIMEOUT_MS = 30_000;

/** 等待 bot 稳定落地（onGround=true）的超时。 */
const SETTLE_TIMEOUT_MS = 10_000;

/**
 * MC 协议允许的最大用户名长度。
 * 依据：vanilla ServerboundHelloPacket 的 `readUtf(16)`。
 */
const MAX_USERNAME_LENGTH = 16;

/** 用户名统一前缀（用于在服务端日志中识别 e2e 连接）。计入 MAX_USERNAME_LENGTH。 */
const USERNAME_PREFIX = "e2e_";

/** 中文运行模式名。 */
function modeLabel(mode: RunMode): string {
    return { regress: "回归比对", refresh: "刷新基线", diff: "双跑对比" }[mode];
}

/**
 * 执行一条用例。
 */
async function runSingleCase(
    definition: CaseDefinition,
    serverKind: ServerKind,
    runId: string,
    options: RunnerOptions,
): Promise<CaseResult> {
    const startedAtMs = Date.now();
    const runDir = runDirFor(runId, definition.id);
    removeDirQuietly(runDir);

    let server: ServerProcess | null = null;
    let handle: BotHandle | null = null;
    /** vanilla 侧本用例的世界名（用于结束后清理共享 cwd 下的世界目录）。 */
    let vanillaWorldName = "";
    let snapshot: CaseSnapshot | null = null;
    let baselineSnapshot: CaseSnapshot | null = null;
    let differencesText = "";
    let failureKind = "";
    let failureMessage = "";
    let surfaceY = 0;

    try {
        const port = await allocatePort();
        if (serverKind === "cubium") {
            server = await startCubiumServer({
                runDir,
                port,
                profile: E2E_SERVER_PROFILE,
                maxPlayers: 4,
            });
        } else {
            // vanilla 的工作目录是跨用例共享的（bundler 只解包一次），靠 worldName 隔离世界。
            // 名字里带上 runId 后缀，避免与其他运行的残留世界目录冲突。
            vanillaWorldName = `e2e_${sanitizeName(definition.id)}_${runId.replace(/\D/g, "").slice(-6)}`;
            server = await startVanillaServer({
                runDir,
                port,
                profile: E2E_SERVER_PROFILE,
                maxPlayers: 4,
                worldName: vanillaWorldName,
            });
        }

        handle = createBot({ port, username: `e2e_${sanitizeName(definition.id)}`, traceLimit: TRACE_LIMIT });
        const { bot, trace } = handle;

        await waitForEvent(bot, "spawn", {
            timeoutMs: SPAWN_TIMEOUT_MS,
            what: "spawn 事件",
            describe: () => describeBotState(bot, trace.count("login"), trace.count("update_health")),
        });

        const position = bot.entity.position;
        const spawnX = Math.floor(position.x);
        const spawnZ = Math.floor(position.z);
        surfaceY = await detectSurfaceY(bot, spawnX, spawnZ, {
            scanTop: 64,
            scanBottom: -70,
            timeoutMs: 20_000,
            pollMs: 100,
        });
        // 出生列可读不代表邻域已送达——区块是逐 tick 推送的。用例若在此时读邻域方块，
        // 会得到大量"未加载"的假失败。此处统一等待一个 7x7 区域加载完成。
        await waitForAreaLoaded(bot, spawnX, spawnZ, surfaceY, LOADED_AREA_RADIUS, {
            timeoutMs: LOADED_AREA_TIMEOUT_MS,
            pollMs: 100,
        });

        // 等待 bot 稳定落地。spawn 事件到达时 bot 可能仍处于下落的某一帧（onGround=false），
        // 这是时序相关的瞬态量——直接采快照会造成基线抖动（实测：同一用例 3 次运行中
        // 有 1 次 onGround 为 false 而其余为 true）。等它落到地表后再执行用例，
        // 既消除抖动，也让所有用例都在稳定状态下运行。
        await waitForCondition(() => bot.entity?.onGround === true, {
            timeoutMs: SETTLE_TIMEOUT_MS,
            pollMs: 50,
            what: "bot 稳定落地（onGround === true）",
        });

        const context: CaseContext = {
            bot,
            trace,
            serverKind,
            surfaceY,
            spawnX,
            spawnZ,
            runDir,
        };
        const extra = await withTimeout(definition.run(context), CASE_TIMEOUT_MS, definition.id);

        snapshot = captureSnapshot({
            bot,
            surfaceY,
            spawned: true,
            kickReason: null,
            sawLoginPacket: trace.has("login"),
            sawUpdateHealthPacket: trace.count("update_health") > 0,
            extra,
        });

        // 基线比对 / 写入
        const baselineFile = baselinePath(BASELINE_DIR, serverKind);
        if (options.mode === "regress" || options.mode === "diff") {
            const loaded = loadBaseline(baselineFile);
            if (loaded.kind === "missing") {
                throw new BaselineMissingError(`基线文件不存在：${baselineFile}（先跑 --mode=refresh）`);
            }
            if (loaded.kind === "stale") {
                throw new BaselineMissingError(`基线已过期：${loaded.reason}（重新跑 --mode=refresh）`);
            }
            const expected = loaded.baseline.cases[definition.id];
            if (expected === undefined) {
                throw new BaselineMissingError(`基线中没有用例 ${definition.id}（重新跑 --mode=refresh）`);
            }
            baselineSnapshot = expected.snapshot;
            const result = compareSnapshot(snapshot, expected.snapshot);
            if (!result.equal) {
                differencesText = renderDifferences(result.differences);
                throw new AssertionError(`快照与基线不一致：\n${differencesText}`);
            }
        }
    } catch (err) {
        failureMessage = (err as Error).message;
        failureKind =
            err instanceof AssertionError
                ? "断言失败"
                : err instanceof TimeoutError
                  ? "等待超时"
                  : err instanceof BotFailureError
                    ? "bot 故障"
                    : err instanceof BaselineMissingError
                      ? "基线问题"
                      : server !== null && server.state === "crashed"
                        ? "服务端崩溃"
                        : "其他错误";
    } finally {
        if (handle !== null) {
            await handle.dispose();
        }
        if (server !== null) {
            await server.stop();
        }
        if (vanillaWorldName.length > 0) {
            removeVanillaWorld(vanillaWorldName);
        }
    }

    const ok = failureKind.length === 0;
    if (!ok && server !== null) {
        const artifactDir = artifactDirFor(runId, definition.id);
        writeArtifacts(artifactDir, {
            caseId: definition.id,
            title: definition.title,
            serverKind,
            failureKind,
            failureMessage,
            serverLog: server.logs.join("\n"),
            botTrace: handle?.trace.toJsonl() ?? "",
            packetSummary: handle?.trace.summary() ?? "",
            actualSnapshot: JSON.stringify(snapshot, null, 2),
            baselineSnapshot: baselineSnapshot === null ? "" : JSON.stringify(baselineSnapshot, null, 2),
            differences: differencesText,
            meta: JSON.stringify(
                {
                    runId,
                    runDir,
                    failureKind,
                    elapsedMs: Date.now() - startedAtMs,
                    serverStartupMs: server.startupMs,
                    serverExit: server.exitInfo,
                    surfaceY,
                },
                null,
                2,
            ),
        });
    }
    if (ok && !options.keepArtifacts) {
        removeDirQuietly(runDir);
    }

    return {
        id: definition.id,
        title: definition.title,
        serverKind,
        ok,
        failureKind,
        failureMessage,
        elapsedMs: Date.now() - startedAtMs,
        snapshot,
    };
}

/** 基线缺失/过期——与环境有关，退出码与断言失败区分开。 */
class BaselineMissingError extends Error {}

/**
 * 由用例 id 生成合法的 MC 用户名。
 *
 * **必须 ≤ 16 字符**：vanilla 的 ServerboundHelloPacket 用 `readUtf(16)` 读取用户名
 * （ServerboundHelloPacket.java），超长会被直接拒绝并断开——报
 * `Failed to decode packet 'serverbound/minecraft:hello'`，错误信息完全指不到用户名上。
 * Cubium 侧不校验该长度，故这个约束只在双跑对比时才会暴露。
 */
function sanitizeName(id: string): string {
    // 不含前缀——调用方负责拼接（用户名总长须 ≤ MAX_USERNAME_LENGTH）。
    return id.replace(/[^a-zA-Z0-9_]/g, "_").slice(0, MAX_USERNAME_LENGTH - USERNAME_PREFIX.length);
}

/** 等待 spawn 失败时的现场描述。 */
function describeBotState(bot: Bot, loginCount: number, healthCount: number): string {
    const state = (bot._client as unknown as { state?: string }).state ?? "<未知>";
    return [
        `  连接阶段: ${state}`,
        `  收到 login(cb 48): ${loginCount} 次`,
        `  收到 update_health(即 set_health cb 102): ${healthCount} 次  ← 为 0 则 mineflayer 永不触发 spawn`,
        `  已收到包类型: ${healthCount === 0 ? "见 bot-trace.jsonl" : ""}`,
    ].join("\n");
}

/** 给用例执行加超时。 */
async function withTimeout<T>(promise: Promise<T>, timeoutMs: number, label: string): Promise<T> {
    let timer: NodeJS.Timeout | null = null;
    try {
        return await Promise.race([
            promise,
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new TimeoutError(`用例 ${label} 执行超时（${timeoutMs}ms）`)),
                    timeoutMs,
                );
            }),
        ]);
    } finally {
        if (timer !== null) {
            clearTimeout(timer);
        }
    }
}

/** 汇总结果。 */
export interface RunSummary {
    readonly exitCode: number;
    readonly results: readonly CaseResult[];
    readonly runId: string;
}

/**
 * 运行全部匹配的用例。
 *
 * @param definitions 全部用例。
 * @param options 运行参数。
 */
export async function runCases(
    definitions: readonly CaseDefinition[],
    options: RunnerOptions,
): Promise<RunSummary> {
    installNoiseFilter();
    const runId = makeRunId();
    pruneArtifacts(10);

    const selected = definitions.filter(
        (definition) => options.caseFilter === null || definition.id.includes(options.caseFilter),
    );
    if (selected.length === 0) {
        console.error(`没有匹配的用例（过滤条件：${options.caseFilter ?? "<无>"}）`);
        return { exitCode: 2, results: [], runId };
    }

    console.log(`运行模式：${modeLabel(options.mode)}    用例数：${selected.length}    runId：${runId}\n`);

    const results: CaseResult[] = [];
    for (const definition of selected) {
        // 用例声明的适用服务端 ∩ 本次启用的服务端。
        const applicable = options.enabledServers.filter((kind) => definition.servers.includes(kind));
        for (const serverKind of applicable) {
            process.stdout.write(`  [${serverKind}] ${definition.id} ... `);
            const result = await runSingleCase(definition, serverKind, runId, options);
            results.push(result);
            if (result.ok) {
                console.log(`✓ 通过 (${result.elapsedMs}ms)`);
            } else {
                console.log(`✗ ${result.failureKind} (${result.elapsedMs}ms)`);
                console.log(
                    result.failureMessage
                        .split("\n")
                        .map((line) => `      ${line}`)
                        .join("\n"),
                );
            }
        }
    }

    // refresh：把结果写回基线
    if (options.mode === "refresh") {
        if (!options.accept) {
            console.error("\n拒绝写入基线：refresh 模式需要显式 --accept 确认。");
            return { exitCode: 2, results, runId };
        }
        const passed = results.filter((result) => result.ok);
        const failed = results.filter((result) => !result.ok);
        if (failed.length > 0) {
            console.error(`\n拒绝写入基线：有 ${failed.length} 条用例未通过（基线只应记录全部通过的运行）。`);
            return { exitCode: 2, results, runId };
        }
        for (const serverKind of ["cubium", "vanilla"] as const) {
            const forServer = passed.filter((result) => result.serverKind === serverKind);
            if (forServer.length === 0) {
                continue;
            }
            // 合并写入而非全量覆盖：只跑部分用例（--case 过滤）时，其余用例的既有基线
            // 条目必须保留，否则一次局部刷新会把整份基线削成只剩本次跑到的几条。
            const existing = loadBaseline(baselinePath(BASELINE_DIR, serverKind));
            const mergedCases: Record<string, BaselineEntry> =
                existing.kind === "ok" ? { ...existing.baseline.cases } : {};
            for (const result of forServer) {
                if (result.snapshot !== null) {
                    // 只存快照，不存耗时等运行度量（见 baseline/store.ts 的 BaselineEntry 说明）。
                    mergedCases[result.id] = { snapshot: result.snapshot };
                }
            }
            const baseline: Baseline = makeBaseline(serverKind, gitCommit(), `由 ${runId} 更新`);
            saveBaseline(baselinePath(BASELINE_DIR, serverKind), { ...baseline, cases: mergedCases });
            console.log(
                `已写入基线：${baselinePath(BASELINE_DIR, serverKind)}` +
                    `（本次 ${forServer.length} 条，合计 ${Object.keys(mergedCases).length} 条）`,
            );
        }
    }

    const failedCount = results.filter((result) => !result.ok).length;
    console.log(`\n合计：${results.length} 条，通过 ${results.length - failedCount}，失败 ${failedCount}`);

    // 退出码语义：0 全通过；1 有用例失败；2 环境/基线问题
    const envProblem = results.some((result) => result.failureKind === "基线问题");
    return { exitCode: failedCount === 0 ? 0 : envProblem ? 2 : 1, results, runId };
}

function gitCommit(): string {
    try {
        return execFileSync("git", ["rev-parse", "--short", "HEAD"], { cwd: REPO_ROOT }).toString().trim();
    } catch {
        return "";
    }
}

/** 供调试：列出诊断产物目录下的文件。 */
export function listArtifacts(dir: string): string[] {
    if (!fs.existsSync(dir)) {
        return [];
    }
    return fs.readdirSync(path.join(dir));
}
