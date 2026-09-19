/*
 * report.ts — diff 模式的双跑对撞报告。
 *
 * diff 模式把同一套用例在 Cubium 与 vanilla 上各跑一遍，然后**直接对撞两端的快照**。
 * 这与 regress 互补：regress 保证「每一端与自己过去的基线一致」（防回归），
 * diff 保证「两端在同一时点的行为一致」（防实现偏差）。
 *
 * 二者都必要——一个用例可能两侧各自稳定、却互相不同，那正是行为偏差。
 *
 * 注意能直接对撞的前提是快照已归一化（见 snapshot.ts）：绝对 Y 被换成相对地表、
 * 不含实体 id/UUID/耗时。informational 段（含 surfaceY）是唯一豁免项。
 */

import fs from "node:fs";
import path from "node:path";
import type { CaseResult } from "../runner.ts";

/** 单个用例的对撞结论。 */
interface CaseComparison {
    readonly caseId: string;
    readonly title: string;
    /** 两侧都有结果才可比；只有一端运行的用例归入 oneSided。 */
    readonly kind: "identical" | "divergent" | "oneSided";
    readonly details: string;
}

function isPlainObject(value: unknown): value is Record<string, unknown> {
    return typeof value === "object" && value !== null && !Array.isArray(value);
}

/** 收集两端的字段差异（复用与基线比对同构的遍历，但方向是「端 vs 端」）。 */
function collectDifferences(
    left: unknown,
    right: unknown,
    pathText: string,
    out: string[],
): void {
    if (isPlainObject(left) && isPlainObject(right)) {
        const keys = [...new Set([...Object.keys(left), ...Object.keys(right)])].sort();
        for (const key of keys) {
            collectDifferences(left[key], right[key], pathText.length === 0 ? key : `${pathText}.${key}`, out);
        }
        return;
    }
    if (Array.isArray(left) && Array.isArray(right)) {
        const a = [...left].sort();
        const b = [...right].sort();
        if (JSON.stringify(a) !== JSON.stringify(b)) {
            out.push(`${pathText}: cubium=${JSON.stringify(a)} vanilla=${JSON.stringify(b)}`);
        }
        return;
    }
    if (JSON.stringify(left) !== JSON.stringify(right)) {
        out.push(`${pathText}: cubium=${JSON.stringify(left)} vanilla=${JSON.stringify(right)}`);
    }
}

/**
 * 由本次 diff 运行的结果生成对撞报告（Markdown）。
 *
 * @param results 本次运行的全部用例结果（含两端）。
 * @returns Markdown 文本。
 */
export function renderDualRunReport(results: readonly CaseResult[]): string {
    const byCase = new Map<string, Map<string, CaseResult>>();
    for (const result of results) {
        const forCase = byCase.get(result.id) ?? new Map<string, CaseResult>();
        forCase.set(result.serverKind, result);
        byCase.set(result.id, forCase);
    }

    const comparisons: CaseComparison[] = [];
    for (const [caseId, perServer] of [...byCase.entries()].sort((a, b) => a[0].localeCompare(b[0]))) {
        const cubium = perServer.get("cubium");
        const vanilla = perServer.get("vanilla");
        const title = cubium?.title ?? vanilla?.title ?? caseId;

        if (cubium === undefined || vanilla === undefined) {
            comparisons.push({
                caseId,
                title,
                kind: "oneSided",
                details: `仅 ${cubium !== undefined ? "Cubium" : "vanilla"} 运行（用例声明的 servers 不含另一端）`,
            });
            continue;
        }
        if (!cubium.ok || !vanilla.ok) {
            const failed: string[] = [];
            if (!cubium.ok) {
                failed.push(`cubium: ${cubium.failureKind} — ${cubium.failureMessage.split("\n")[0]}`);
            }
            if (!vanilla.ok) {
                failed.push(`vanilla: ${vanilla.failureKind} — ${vanilla.failureMessage.split("\n")[0]}`);
            }
            comparisons.push({
                caseId,
                title,
                kind: "divergent",
                details: `有端未通过：\n${failed.map((line) => `      - ${line}`).join("\n")}`,
            });
            continue;
        }

        const differences: string[] = [];
        if (cubium.snapshot !== null && vanilla.snapshot !== null) {
            // **跳过 informational 段**：其中的 surfaceY 是两侧地形基准的必然差异
            // （Cubium 的 flat 层从 Y=0 起、vanilla 从维度 minY=-64 起，差 64 格），
            // 属设计上豁免比对的字段。这与 baseline/compare.ts 的豁免规则保持一致——
            // 漏掉这条会把「已知且预期」的差异报成实现偏差，让报告失去信号价值。
            collectDifferences(cubium.snapshot.lifecycle, vanilla.snapshot.lifecycle, "lifecycle", differences);
            collectDifferences(cubium.snapshot.world, vanilla.snapshot.world, "world", differences);
            collectDifferences(cubium.snapshot.terrain, vanilla.snapshot.terrain, "terrain", differences);
            collectDifferences(cubium.snapshot.player, vanilla.snapshot.player, "player", differences);
            collectDifferences(cubium.snapshot.extra, vanilla.snapshot.extra, "extra", differences);
        }
        if (differences.length === 0) {
            comparisons.push({ caseId, title, kind: "identical", details: "" });
        } else {
            comparisons.push({
                caseId,
                title,
                kind: "divergent",
                details: differences.map((line) => `      - ${line}`).join("\n"),
            });
        }
    }

    const identical = comparisons.filter((entry) => entry.kind === "identical");
    const divergent = comparisons.filter((entry) => entry.kind === "divergent");
    const oneSided = comparisons.filter((entry) => entry.kind === "oneSided");

    const lines: string[] = [
        "# 双跑对撞报告（Cubium vs vanilla）",
        "",
        `生成时间：${new Date().toISOString()}`,
        "",
        `- 两侧行为一致：**${identical.length}**`,
        `- 存在差异 / 有端未通过：**${divergent.length}**`,
        `- 仅一端运行：${oneSided.length}`,
        "",
        "> 比对的是**归一化后**的状态快照（绝对 Y 已换成相对地表、不含实体 id/UUID/耗时），",
        "> 因此两侧可直接对撞。informational.surfaceY 是唯一豁免项。",
        "",
    ];

    if (divergent.length > 0) {
        lines.push("## 存在差异的用例", "");
        for (const entry of divergent) {
            lines.push(`### ${entry.caseId}`, "", entry.title, "", entry.details, "");
        }
    }

    lines.push("## 两侧一致的用例", "");
    for (const entry of identical) {
        lines.push(`- ${entry.caseId}`);
    }
    lines.push("");

    if (oneSided.length > 0) {
        lines.push("## 仅一端运行", "");
        for (const entry of oneSided) {
            lines.push(`- ${entry.caseId} — ${entry.details}`);
        }
        lines.push("");
    }

    return lines.join("\n");
}

/** 把对撞报告写入文件。 */
export function writeDualRunReport(outputPath: string, results: readonly CaseResult[]): void {
    fs.mkdirSync(path.dirname(outputPath), { recursive: true });
    fs.writeFileSync(outputPath, renderDualRunReport(results));
}

/** 供 runner 打印用的简要结论。 */
export function summarizeDualRun(results: readonly CaseResult[]): string {
    const report = renderDualRunReport(results);
    return report
        .split("\n")
        .filter((line) => line.startsWith("- 两侧") || line.startsWith("- 存在差异") || line.startsWith("- 仅一端"))
        .join("\n");
}
