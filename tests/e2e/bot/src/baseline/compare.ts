/*
 * compare.ts — 快照与基线的结构化比对。
 *
 * 比对忽略 informational 段（其中 surfaceY 两侧必然不同），其余逐字段严格相等。
 */

import type { CaseSnapshot } from "../bot/snapshot.ts";

/** 一处字段差异。 */
export interface Difference {
    /** 字段路径，如 "player.health"。 */
    readonly path: string;
    readonly expected: unknown;
    readonly actual: unknown;
}

/** 比对结果。 */
export interface CompareResult {
    readonly equal: boolean;
    readonly differences: readonly Difference[];
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

function walk(expected: unknown, actual: unknown, path: string, out: Difference[]): void {
    if (isPlainObject(expected) && isPlainObject(actual)) {
        const keys = [...new Set([...Object.keys(expected), ...Object.keys(actual)])].sort();
        for (const key of keys) {
            walk(expected[key], actual[key], path.length === 0 ? key : `${path}.${key}`, out);
        }
        return;
    }
    // 数组：排序后比对（元素顺序通常不是被测语义）。
    if (Array.isArray(expected) && Array.isArray(actual)) {
        const a = [...expected].sort();
        const b = [...actual].sort();
        if (JSON.stringify(a) !== JSON.stringify(b)) {
            out.push({ path, expected: a, actual: b });
        }
        return;
    }
    if (JSON.stringify(expected) !== JSON.stringify(actual)) {
        out.push({ path, expected, actual });
    }
}

/**
 * 比对实际快照与基线快照。
 *
 * @param actual 本次采集的快照。
 * @param baseline 冻结基线中的快照。
 */
export function compareSnapshot(actual: CaseSnapshot, baseline: CaseSnapshot): CompareResult {
    const differences: Difference[] = [];
    // informational 段显式跳过（surfaceY 是可预期的必然差异）。
    walk(baseline.lifecycle, actual.lifecycle, "lifecycle", differences);
    walk(baseline.world, actual.world, "world", differences);
    walk(baseline.player, actual.player, "player", differences);
    walk(baseline.extra, actual.extra, "extra", differences);
    return { equal: differences.length === 0, differences };
}

/** 把差异列表渲染成可读文本。 */
export function renderDifferences(differences: readonly Difference[]): string {
    return differences
        .map(
            (diff) =>
                `  ${diff.path}\n    基线: ${JSON.stringify(diff.expected)}\n    实际: ${JSON.stringify(diff.actual)}`,
        )
        .join("\n");
}
