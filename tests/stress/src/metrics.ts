import assert from "node:assert/strict";
import fs from "node:fs";
import type { Phase } from "./context.ts";

export interface TickSample {
    timeMs: number;
    tickMs: number;
    intervalMs: number;
    memorySampleMs: number | null;
    rssMiB: number | null;
    commitMiB: number | null;
    players: number;
    chunkHolders: number;
    loadedChunks: number;
    entities: number;
}

/** 使用最近秩法计算分位数；空阶段保持空值，避免伪造零耗时。 */
export function statistics(values: readonly number[]): { count: number; mean: number; p50: number; p95: number; max: number; min: number } | null {
    if (values.length === 0) return null;
    const sorted = [...values].sort((a, b) => a - b);
    return {
        count: values.length, mean: values.reduce((sum, value) => sum + value, 0) / values.length,
        p50: sorted[Math.ceil(sorted.length * 0.5) - 1], p95: sorted[Math.ceil(sorted.length * 0.95) - 1],
        max: sorted[sorted.length - 1], min: sorted[0],
    };
}

/** 读取每 tick 原始记录；强制终止时只忽略末尾未写完整的一行。 */
export function readMetrics(file: string): TickSample[] {
    const content = fs.readFileSync(file, "utf8");
    const lines = content.split(/\r?\n/);
    const header = lines.shift();
    assert.equal(header, "time_ms,tick_ms,interval_ms,memory_sample_ms,rss_mib,commit_mib,players,chunk_holders,loaded_chunks,entities", "Unknown server metrics schema");
    if (!content.endsWith("\n")) lines.pop();
    return lines.filter((line) => line !== "").map((line) => {
        const columns = line.split(",");
        assert.equal(columns.length, 10, "Incomplete metrics row");
        const values = columns.map((column, index) => index >= 3 && index <= 5 && column === "" ? null : Number(column));
        assert(values.every((value) => value === null || Number.isFinite(value)), "Non-numeric server metrics");
        return {
            timeMs: values[0]!, tickMs: values[1]!, intervalMs: values[2]!, memorySampleMs: values[3],
            rssMiB: values[4], commitMiB: values[5], players: values[6]!, chunkHolders: values[7]!,
            loadedChunks: values[8]!, entities: values[9]!,
        };
    });
}

/** tick 按执行区间与阶段的重叠归属；内存只使用阶段内完成的真实样本。 */
export function summarizePhase(phase: Phase, samples: readonly TickSample[]) {
    const selected = samples.filter((sample) => sample.timeMs >= phase.startMs && sample.timeMs <= phase.endMs);
    const overlappingTicks = samples.filter((sample) => sample.timeMs >= phase.startMs && sample.timeMs - sample.tickMs <= phase.endMs);
    return {
        ...phase, durationSeconds: (phase.endMs - phase.startMs) / 1000,
        tickMs: statistics(overlappingTicks.map((sample) => sample.tickMs)),
        intervalMs: statistics(selected.map((sample) => sample.intervalMs)),
        rssMiB: statistics(selected.flatMap((sample) => sample.rssMiB === null ? [] : [sample.rssMiB])),
        commitMiB: statistics(selected.flatMap((sample) => sample.commitMiB === null ? [] : [sample.commitMiB])),
        maxPlayers: Math.max(0, ...selected.map((sample) => sample.players)),
        endLoadedChunks: selected.at(-1)?.loadedChunks ?? null,
        endChunkHolders: selected.at(-1)?.chunkHolders ?? null,
        ticksOver50Ms: overlappingTicks.filter((sample) => sample.tickMs > 50).length,
    };
}

export type PhaseSummary = ReturnType<typeof summarizePhase>;

/** 比较最后五秒恢复窗口与基线，既报告绝对值，也报告相对变化。 */
export function recoverySummary(phases: readonly Phase[], samples: readonly TickSample[], memoryRatio: number, tickLimitMs: number) {
    const baseline = phases.find((phase) => phase.name === "baseline");
    const recovery = phases.find((phase) => phase.name === "recovery");
    if (baseline === undefined || recovery === undefined) return null;
    const before = summarizePhase(baseline, samples);
    const after = summarizePhase({ ...recovery, startMs: Math.max(recovery.startMs, recovery.endMs - 5000) }, samples);
    const ratio = (value: number | undefined, base: number | undefined): number | null =>
        value === undefined || base === undefined || base === 0 ? null : value / base;
    const rssRatio = ratio(after.rssMiB?.p50, before.rssMiB?.p50);
    const commitRatio = ratio(after.commitMiB?.p50, before.commitMiB?.p50);
    return {
        baseline: before, finalWindow: after, rssRatio, commitRatio,
        rssRecovered: rssRatio === null ? null : rssRatio <= memoryRatio,
        commitRecovered: commitRatio === null ? null : commitRatio <= memoryRatio,
        tickRecovered: after.tickMs === null ? null : after.tickMs.p95 <= tickLimitMs,
        tickP95Ratio: ratio(after.tickMs?.p95, before.tickMs?.p95),
        limits: { memoryRatio, tickLimitMs },
    };
}

/** 缺失阶段、缺失内存线程或失去时间覆盖都必须显式报错。 */
export function validateMetrics(phases: readonly Phase[], samples: readonly TickSample[]): void {
    for (const phase of phases) {
        if (phase.endMs - phase.startMs < 1000) continue;
        const overlappingTicks = samples.filter((sample) => sample.timeMs >= phase.startMs && sample.timeMs - sample.tickMs <= phase.endMs);
        assert(overlappingTicks.length > 0, `No server metrics for phase ${phase.name}`);
        assert(overlappingTicks.some((sample) => sample.rssMiB !== null && sample.commitMiB !== null), `No memory samples for phase ${phase.name}`);
        const toleranceMs = Math.min(3000, Math.max(500, (phase.endMs - phase.startMs) * 0.2));
        const spansStart = overlappingTicks.some((sample) => sample.timeMs - sample.tickMs <= phase.startMs + toleranceMs);
        const spansEnd = overlappingTicks.some((sample) => sample.timeMs >= phase.endMs - toleranceMs);
        assert(spansStart, `Missing metrics at start of ${phase.name}`);
        assert(spansEnd, `Missing metrics at end of ${phase.name}`);
        assert(overlappingTicks.every((sample) => sample.memorySampleMs !== null && Math.abs(sample.timeMs - sample.memorySampleMs) < 3000), `Memory sampler stalled during ${phase.name}`);
    }
}
