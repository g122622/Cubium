/*
 * diff.ts — 断言级比对与已知差异豁免。
 *
 * 判定单元是「用例的一个步骤」或「一个探针」，不是整条用例：knownDiff 必须写到
 * `steps[i].feedback` / `probes.<key>`，避免一条豁免掩盖同用例里新出现的差异。
 *
 * 已知差异有两种失效方向，都会进报告：
 *   - **命中**：差异仍在，按登记豁免，但单独列出以便定期复查看是否该修了；
 *   - **过期**：登记了却不再出现（多半是 Cubium 已修好），提示删除登记。
 */

import type { KnownDiffSpec } from "../case.ts";

/** 差异分类。 */
export type DiffCategory = "feedback" | "probe" | "clientView" | "timeout";

/** 一条实际差异。 */
export interface DiffEntry {
    /** 差异路径：`steps[<i>].feedback` 或 `probes.<key>`。 */
    readonly path: string;
    readonly category: DiffCategory;
    /** 产生该差异的指令原文。 */
    readonly command: string;
    readonly cubium: string;
    readonly vanilla: string;
    /** 命中的已知差异登记；未登记为 undefined。 */
    readonly known: KnownDiffSpec | undefined;
}

/** 单条用例的比对结果。 */
export interface CaseComparison {
    readonly caseId: string;
    /** 实际差异（含已登记的）。 */
    readonly diffs: readonly DiffEntry[];
    /** 登记了但本次没有出现的差异路径。 */
    readonly staleKnownDiffs: readonly KnownDiffSpec[];
}

/** 参与比对的一侧结果（步骤、探针与客户端观测的归一化文本）。 */
export interface ComparableSide {
    /** 步骤归一化文本，下标与用例 steps 对齐。 */
    readonly stepTexts: readonly string[];
    /** 步骤是否无反馈超时，下标与用例 steps 对齐。 */
    readonly stepTimeouts: readonly boolean[];
    /** 探针归一化文本。 */
    readonly probeTexts: ReadonlyMap<string, string>;
    /** 客户端视角观测文本。 */
    readonly viewTexts: ReadonlyMap<string, string>;
}

/** 一次比对需要的用例元数据。 */
export interface CaseMeta {
    readonly caseId: string;
    readonly stepCommands: readonly string[];
    readonly probeCommands: ReadonlyMap<string, string>;
    /** 客户端观测键 → 观测类型（报告里作为 command 列的替代）。 */
    readonly viewKinds: ReadonlyMap<string, string>;
    readonly knownDiffs: readonly KnownDiffSpec[];
}

/**
 * 比对两侧结果。
 *
 * @param meta 用例元数据。
 * @param cubium Cubium 侧结果。
 * @param vanilla vanilla 侧结果。
 * @returns 差异与过期登记。
 */
export function compareCase(meta: CaseMeta, cubium: ComparableSide, vanilla: ComparableSide): CaseComparison {
    const knownByPath = new Map(meta.knownDiffs.map((item) => [item.path, item]));
    const hitPaths = new Set<string>();
    const diffs: DiffEntry[] = [];

    const stepCount = Math.max(cubium.stepTexts.length, vanilla.stepTexts.length);
    for (let index = 0; index < stepCount; index += 1) {
        const left = cubium.stepTexts[index];
        const right = vanilla.stepTexts[index];
        if (left === undefined || right === undefined || left === right) {
            continue;
        }
        const path = `steps[${index}].feedback`;
        const known = knownByPath.get(path);
        if (known !== undefined) {
            hitPaths.add(path);
        }
        const timedOut = cubium.stepTimeouts[index] === true || vanilla.stepTimeouts[index] === true;
        diffs.push({
            path,
            category: timedOut ? "timeout" : "feedback",
            command: meta.stepCommands[index] ?? "<缺失步骤>",
            cubium: left,
            vanilla: right,
            known,
        });
    }

    for (const [key, right] of vanilla.probeTexts) {
        const left = cubium.probeTexts.get(key);
        if (left === undefined || left === right) {
            continue;
        }
        const path = `probes.${key}`;
        const known = knownByPath.get(path);
        if (known !== undefined) {
            hitPaths.add(path);
        }
        diffs.push({
            path,
            category: "probe",
            command: meta.probeCommands.get(key) ?? "<未知探针>",
            cubium: left,
            vanilla: right,
            known,
        });
    }
    // Cubium 侧独有的探针（vanilla 没跑到）同样是差异。
    for (const [key, left] of cubium.probeTexts) {
        if (vanilla.probeTexts.has(key)) {
            continue;
        }
        const path = `probes.${key}`;
        const known = knownByPath.get(path);
        if (known !== undefined) {
            hitPaths.add(path);
        }
        diffs.push({
            path,
            category: "probe",
            command: meta.probeCommands.get(key) ?? "<未知探针>",
            cubium: left,
            vanilla: "<无此探针>",
            known,
        });
    }

    for (const [key, right] of vanilla.viewTexts) {
        const left = cubium.viewTexts.get(key);
        if (left === undefined || left === right) {
            continue;
        }
        const path = `clientView.${key}`;
        const known = knownByPath.get(path);
        if (known !== undefined) {
            hitPaths.add(path);
        }
        diffs.push({
            path,
            category: "clientView",
            command: `<clientView:${meta.viewKinds.get(key) ?? "未知"}>`,
            cubium: left,
            vanilla: right,
            known,
        });
    }
    // Cubium 侧独有的观测（vanilla 没读到）同样是差异。
    for (const [key, left] of cubium.viewTexts) {
        if (vanilla.viewTexts.has(key)) {
            continue;
        }
        const path = `clientView.${key}`;
        const known = knownByPath.get(path);
        if (known !== undefined) {
            hitPaths.add(path);
        }
        diffs.push({
            path,
            category: "clientView",
            command: `<clientView:${meta.viewKinds.get(key) ?? "未知"}>`,
            cubium: left,
            vanilla: "<无此观测>",
            known,
        });
    }

    const staleKnownDiffs = meta.knownDiffs.filter((item) => !hitPaths.has(item.path));
    return { caseId: meta.caseId, diffs, staleKnownDiffs };
}

/**
 * 把比对结果分成「已登记」与「新差异」两组。
 *
 * @param comparison 单用例比对结果。
 * @returns 分组后的差异。
 */
export function partitionDiffs(comparison: CaseComparison): {
    readonly known: readonly DiffEntry[];
    readonly unknown: readonly DiffEntry[];
} {
    return {
        known: comparison.diffs.filter((item) => item.known !== undefined),
        unknown: comparison.diffs.filter((item) => item.known === undefined),
    };
}
