/* diff.ts 的单元测试：差异定位、knownDiff 命中与过期判定。 */

import assert from "node:assert/strict";
import { test } from "node:test";

import type { KnownDiffSpec } from "../case.ts";
import { compareCase, partitionDiffs, type CaseMeta, type ComparableSide } from "./diff.ts";

/** 构造一侧结果。 */
function side(
    stepTexts: readonly string[],
    probeTexts: Record<string, string>,
    stepTimeouts: readonly boolean[] = [],
): ComparableSide {
    return {
        stepTexts,
        stepTimeouts,
        probeTexts: new Map(Object.entries(probeTexts)),
    };
}

/** 构造用例元数据。 */
function meta(knownDiffs: readonly KnownDiffSpec[] = []): CaseMeta {
    return {
        caseId: "test/case",
        stepCommands: ["/a", "/b"],
        probeCommands: new Map([
            ["p1", "/probe1"],
            ["p2", "/probe2"],
        ]),
        knownDiffs,
    };
}

test("两侧完全一致时没有差异", () => {
    const comparison = compareCase(meta(), side(["x"], { p1: "v" }), side(["x"], { p1: "v" }));
    assert.equal(comparison.diffs.length, 0);
    assert.equal(comparison.staleKnownDiffs.length, 0);
});

test("步骤差异按 steps[i].feedback 定位", () => {
    const comparison = compareCase(meta(), side(["same", "left"], { p1: "v" }), side(["same", "right"], { p1: "v" }));
    assert.equal(comparison.diffs.length, 1);
    assert.equal(comparison.diffs[0]?.path, "steps[1].feedback");
    assert.equal(comparison.diffs[0]?.command, "/b");
});

test("探针差异按 probes.<key> 定位", () => {
    const comparison = compareCase(meta(), side([], { p1: "left", p2: "same" }), side([], { p1: "right", p2: "same" }));
    assert.equal(comparison.diffs.length, 1);
    assert.equal(comparison.diffs[0]?.path, "probes.p1");
    assert.equal(comparison.diffs[0]?.command, "/probe1");
});

test("一侧缺失的探针算差异", () => {
    const comparison = compareCase(meta(), side([], { p1: "v" }), side([], {}));
    assert.equal(comparison.diffs.length, 1);
    assert.equal(comparison.diffs[0]?.vanilla, "<无此探针>");
});

test("knownDiff 精确命中后被豁免但仍记录", () => {
    const known: KnownDiffSpec = { path: "steps[0].feedback", reason: "已知" };
    const comparison = compareCase(meta([known]), side(["left"], {}), side(["right"], {}));
    assert.equal(comparison.diffs.length, 1);
    assert.equal(comparison.diffs[0]?.known, known);
    assert.equal(partitionDiffs(comparison).unknown.length, 0);
    assert.equal(comparison.staleKnownDiffs.length, 0);
});

test("knownDiff 不能跨步骤生效", () => {
    const known: KnownDiffSpec = { path: "steps[0].feedback", reason: "已知" };
    const comparison = compareCase(meta([known]), side(["l0", "l1"], {}), side(["r0", "r1"], {}));
    const { known: matched, unknown } = partitionDiffs(comparison);
    assert.equal(matched.length, 1);
    assert.equal(unknown.length, 1);
    assert.equal(unknown[0]?.path, "steps[1].feedback");
});

test("不再出现的登记项被标记为过期", () => {
    const known: KnownDiffSpec = { path: "probes.p1", reason: "已修" };
    const comparison = compareCase(meta([known]), side([], { p1: "v" }), side([], { p1: "v" }));
    assert.equal(comparison.diffs.length, 0);
    assert.equal(comparison.staleKnownDiffs.length, 1);
});

test("任一侧无反馈超时归入 timeout 分类", () => {
    const comparison = compareCase(meta(), side([""], {}, [true]), side(["msg"], {}));
    assert.equal(comparison.diffs[0]?.category, "timeout");
});
