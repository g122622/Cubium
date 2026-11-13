/*
 * store.ts — vanilla 侧的冻结基线。
 *
 * `record` 模式把 vanilla 的归一化结果存成基线，`regress` 模式只跑 Cubium 就与基线比对，
 * 省掉每次启动 java 服务端的成本。基线里只存**归一化后**的文本，不存原始包或组件：
 * 原始组件跨版本不稳定，而判定本来就只认规范形。
 *
 * 基线与 MC 版本、用例数据结构绑定：任何一项变化都必须重新 record，否则报错而不是
 * 静默比错。
 */

import fs from "node:fs";
import path from "node:path";

import { MC_VERSION } from "../config.ts";
import type { ComparableSide } from "../compare/diff.ts";

/** 基线结构版本；用例契约或存储格式变化时递增。 */
const BASELINE_SCHEMA_VERSION = 1;

/** 基线文件路径。 */
const BASELINE_PATH = path.resolve(import.meta.dirname, "..", "..", "baselines", "vanilla.json");

/** 单条用例的冻结结果。 */
export interface FrozenCase {
    readonly stepTexts: readonly string[];
    readonly stepTimeouts: readonly boolean[];
    readonly probeTexts: Readonly<Record<string, string>>;
    readonly viewTexts: Readonly<Record<string, string>>;
}

/** 基线文件内容。 */
export interface VanillaBaseline {
    readonly schemaVersion: number;
    readonly mcVersion: string;
    readonly generatedAt: string;
    readonly cases: Readonly<Record<string, FrozenCase>>;
}

/**
 * 读取基线。
 *
 * @returns 基线；文件不存在返回 null。
 */
export function readBaseline(): VanillaBaseline | null {
    if (!fs.existsSync(BASELINE_PATH)) {
        return null;
    }
    const raw: unknown = JSON.parse(fs.readFileSync(BASELINE_PATH, "utf8"));
    const baseline = parseBaseline(raw);
    if (baseline.schemaVersion !== BASELINE_SCHEMA_VERSION) {
        throw new Error(
            `基线结构版本不匹配：文件为 ${baseline.schemaVersion}，当前为 ${BASELINE_SCHEMA_VERSION}，请重新 record`,
        );
    }
    if (baseline.mcVersion !== MC_VERSION) {
        throw new Error(`基线 MC 版本不匹配：文件为 ${baseline.mcVersion}，当前为 ${MC_VERSION}，请重新 record`);
    }
    return baseline;
}

/**
 * 读取现有基线用于增量记录。
 *
 * `--mode=record --case=<子串>` 只重录一部分用例，其余条目必须保留；版本不匹配时
 * 直接返回空表（本次记录会整体重建基线）。
 *
 * @returns 用例 id → 冻结结果。
 */
export function readBaselineCasesForMerge(): Record<string, FrozenCase> {
    try {
        const baseline = readBaseline();
        return baseline === null ? {} : { ...baseline.cases };
    } catch {
        return {};
    }
}

/**
 * 写出基线。
 *
 * @param cases 用例 id → 冻结结果。
 */
export function writeBaseline(cases: Readonly<Record<string, FrozenCase>>): void {
    const baseline: VanillaBaseline = {
        schemaVersion: BASELINE_SCHEMA_VERSION,
        mcVersion: MC_VERSION,
        generatedAt: new Date().toISOString(),
        cases,
    };
    fs.mkdirSync(path.dirname(BASELINE_PATH), { recursive: true });
    fs.writeFileSync(BASELINE_PATH, `${JSON.stringify(baseline, null, 2)}\n`, "utf8");
}

/**
 * 把一侧结果冻结成基线条目。
 *
 * @param side 可比形态。
 * @returns 冻结条目。
 */
export function freezeCase(side: ComparableSide): FrozenCase {
    return {
        stepTexts: [...side.stepTexts],
        stepTimeouts: [...side.stepTimeouts],
        probeTexts: Object.fromEntries(side.probeTexts),
        viewTexts: Object.fromEntries(side.viewTexts),
    };
}

/**
 * 解冻基线条目。
 *
 * @param frozen 冻结条目。
 * @returns 可比形态。
 */
export function thawCase(frozen: FrozenCase): ComparableSide {
    return {
        stepTexts: frozen.stepTexts,
        stepTimeouts: frozen.stepTimeouts,
        probeTexts: new Map(Object.entries(frozen.probeTexts)),
        viewTexts: new Map(Object.entries(frozen.viewTexts)),
    };
}

/** 校验并归一化基线对象。 */
function parseBaseline(value: unknown): VanillaBaseline {
    if (value === null || typeof value !== "object") {
        throw new Error("基线文件不是 JSON 对象");
    }
    const source = value as Record<string, unknown>;
    const cases = source["cases"];
    if (cases === null || typeof cases !== "object") {
        throw new Error("基线缺少 cases 字段");
    }
    return {
        schemaVersion: Number(source["schemaVersion"]),
        mcVersion: String(source["mcVersion"]),
        generatedAt: String(source["generatedAt"]),
        cases: cases as Record<string, FrozenCase>,
    };
}
