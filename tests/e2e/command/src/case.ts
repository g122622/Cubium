/*
 * case.ts — 用例契约、加载与棋盘坐标替换。
 *
 * 用例是**纯数据**（JSON），驱动只认契约不认内容：新增用例不需要改代码，程序生成的
 * 用例也能直接落成同一种文件。
 *
 * 坐标一律写成 `{x}` / `{y}` / `{z}`（可带整数偏移，如 `{x+3}`），由 runner 按用例
 * 专属棋盘替换。用例里不出现绝对坐标——两侧的世界原点与出生点不同，硬编码会让
 * 用例只在其中一侧成立。
 */

import fs from "node:fs";
import path from "node:path";

/** 棋盘原点（用例坐标替换的基准点）。 */
export interface BoardOrigin {
    readonly x: number;
    readonly y: number;
    readonly z: number;
}

/** 一个执行步骤。 */
export interface StepSpec {
    /** 指令原文（含前导 `/`），可含 `{x}` 形式的坐标占位符。 */
    readonly command: string;
}

/** 一个探针：执行一条指令，并把它的反馈作为结构化判定值。 */
export interface ProbeSpec {
    /** 探针键，参与差异路径（`probes.<key>`）。 */
    readonly key: string;
    readonly command: string;
}

/** 客户端视角的观测类型。 */
export type ClientViewKind = "block" | "entity" | "inventory" | "time";

/** 一项客户端视角观测：bot 从自己收到的包看到的世界状态。 */
export interface ClientViewSpec {
    /** 观测键，参与差异路径（`clientView.<key>`）。 */
    readonly key: string;
    readonly kind: ClientViewKind;
    /** block / entity 类观测的中心点，可含坐标占位符。 */
    readonly position?: string;
    /** entity 类观测的半径（格）。 */
    readonly radius?: number;
}

/** 已登记差异。 */
export interface KnownDiffSpec {
    /** 差异路径：`steps[<i>].feedback`、`probes.<key>` 或 `clientView.<key>`；禁止整条用例豁免。 */
    readonly path: string;
    /** 为什么这条差异是已知的、暂不修。 */
    readonly reason: string;
    /** 证据位置（源码文件、issue 等）。 */
    readonly evidence?: string;
}

/** 一条完整用例。 */
export interface CommandCase {
    /** 全局唯一标识，`/` 分隔分类。 */
    readonly id: string;
    readonly title: string;
    readonly category: string;
    /** 判定前的准备指令：不计入差异，任何反馈都被丢弃。 */
    readonly setup: readonly string[];
    /** 判定主体。 */
    readonly steps: readonly StepSpec[];
    /** 结构化状态查询。 */
    readonly probes: readonly ProbeSpec[];
    /** 客户端视角观测（捕捉「服务端改了但没同步」）。 */
    readonly clientView: readonly ClientViewSpec[];
    /** 断言级已知差异。 */
    readonly knownDiffs: readonly KnownDiffSpec[];
}

/** 用例文件根目录。 */
const CASES_ROOT = path.resolve(import.meta.dirname, "..", "cases");

/**
 * 把用例里的坐标占位符替换成棋盘绝对坐标。
 *
 * 支持 `{x}`、`{x+3}`、`{z-12}`；轴外的字母（如 `{w}`）原样保留，避免把普通花括号
 * 文本误当成坐标。
 *
 * @param text 指令原文。
 * @param origin 棋盘原点。
 * @returns 替换后的指令。
 */
export function substituteCoordinates(text: string, origin: BoardOrigin): string {
    return text.replace(/\{(x|y|z)([+-]\d+)?\}/g, (_match: string, axis: string, offset: string | undefined) => {
        const base = axis === "x" ? origin.x : axis === "y" ? origin.y : origin.z;
        return String(base + (offset === undefined ? 0 : Number.parseInt(offset, 10)));
    });
}

/**
 * 按序号分配棋盘原点。
 *
 * 棋盘沿 Z 轴排布，原点之间留出间隙，避免相邻用例的 `fill`/`clone` 范围互相踩踏。
 *
 * @param index 用例序号（0 起）。
 * @param baseY 棋盘基准高度（两侧都存在的高度）。
 * @param boardSize 棋盘边长。
 * @param gap 棋盘之间的间隙。
 * @returns 棋盘原点。
 */
export function boardOriginFor(index: number, baseY: number, boardSize: number, gap: number): BoardOrigin {
    return { x: 0, y: baseY, z: index * (boardSize + gap) };
}

/**
 * 读取所有用例文件并校验契约。
 *
 * 键顺序无关；缺字段、id 重复、knownDiff 路径格式不对都会直接抛错——用例数据是
 * 判定标准，坏数据必须比坏结果更早暴露。
 *
 * @returns 按 id 排序的用例列表。
 */
export function loadCases(): readonly CommandCase[] {
    if (!fs.existsSync(CASES_ROOT)) {
        return [];
    }
    const files = fs
        .readdirSync(CASES_ROOT, { recursive: true, withFileTypes: true })
        .filter((entry) => entry.isFile() && entry.name.endsWith(".json"))
        .map((entry) => path.join(entry.parentPath, entry.name))
        .sort();

    const cases: CommandCase[] = [];
    const seen = new Set<string>();
    for (const file of files) {
        const parsed: unknown = JSON.parse(fs.readFileSync(file, "utf8"));
        const item = parseCaseValue(parsed, file);
        if (seen.has(item.id)) {
            throw new Error(`用例 id 重复：${item.id}（${file}）`);
        }
        seen.add(item.id);
        cases.push(item);
    }
    return cases.sort((left, right) => (left.id < right.id ? -1 : left.id > right.id ? 1 : 0));
}

/**
 * 校验并归一化单个用例对象。
 *
 * @param value 解析后的 JSON 值。
 * @param file 来源文件（错误信息里带上，便于定位坏数据）。
 * @returns 通过校验的用例。
 */
export function parseCaseValue(value: unknown, file: string): CommandCase {
    if (value === null || typeof value !== "object") {
        throw new Error(`用例必须是 JSON 对象：${file}`);
    }
    const source = value as Record<string, unknown>;
    const id = requireString(source, "id", file);
    const title = requireString(source, "title", file);
    const category = requireString(source, "category", file);

    const setup = requireArray(source, "setup", file).map((item, index) => {
        if (typeof item !== "string") {
            throw new Error(`${file}: setup[${index}] 必须是字符串`);
        }
        return item;
    });

    const steps = requireArray(source, "steps", file).map((item, index): StepSpec => {
        if (item === null || typeof item !== "object") {
            throw new Error(`${file}: steps[${index}] 必须是对象`);
        }
        return { command: requireString(item as Record<string, unknown>, "command", `${file} steps[${index}]`) };
    });

    const probes = requireArray(source, "probes", file).map((item, index): ProbeSpec => {
        if (item === null || typeof item !== "object") {
            throw new Error(`${file}: probes[${index}] 必须是对象`);
        }
        const probe = item as Record<string, unknown>;
        return {
            key: requireString(probe, "key", `${file} probes[${index}]`),
            command: requireString(probe, "command", `${file} probes[${index}]`),
        };
    });

    const clientView = parseClientViews(source["clientView"], file);

    const rawDiffs = source["knownDiffs"] ?? [];
    if (!Array.isArray(rawDiffs)) {
        throw new Error(`${file}: knownDiffs 必须是数组`);
    }
    const knownDiffs = rawDiffs.map((item, index): KnownDiffSpec => {
        if (item === null || typeof item !== "object") {
            throw new Error(`${file}: knownDiffs[${index}] 必须是对象`);
        }
        const diff = item as Record<string, unknown>;
        const spec: KnownDiffSpec = {
            path: requireString(diff, "path", `${file} knownDiffs[${index}]`),
            reason: requireString(diff, "reason", `${file} knownDiffs[${index}]`),
        };
        validateKnownDiffPath(spec.path, file);
        const evidence = diff["evidence"];
        if (evidence !== undefined) {
            if (typeof evidence !== "string") {
                throw new Error(`${file}: knownDiffs[${index}].evidence 必须是字符串`);
            }
            return { ...spec, evidence };
        }
        return spec;
    });

    return { id, title, category, setup, steps, probes, clientView, knownDiffs };
}

/** 解析客户端视角观测列表；缺省视为空。 */
function parseClientViews(value: unknown, file: string): readonly ClientViewSpec[] {
    if (value === undefined) {
        return [];
    }
    if (!Array.isArray(value)) {
        throw new Error(`${file}: clientView 必须是数组`);
    }
    return value.map((item, index): ClientViewSpec => {
        if (item === null || typeof item !== "object") {
            throw new Error(`${file}: clientView[${index}] 必须是对象`);
        }
        const source = item as Record<string, unknown>;
        const key = requireString(source, "key", `${file} clientView[${index}]`);
        const kind = requireString(source, "kind", `${file} clientView[${index}]`);
        if (kind !== "block" && kind !== "entity" && kind !== "inventory" && kind !== "time") {
            throw new Error(`${file}: clientView[${index}].kind 必须是 block/entity/inventory/time，实际为 "${kind}"`);
        }
        const position = source["position"];
        if (position !== undefined && typeof position !== "string") {
            throw new Error(`${file}: clientView[${index}].position 必须是字符串`);
        }
        const radius = source["radius"];
        if (radius !== undefined && typeof radius !== "number") {
            throw new Error(`${file}: clientView[${index}].radius 必须是数字`);
        }
        if ((kind === "block" || kind === "entity") && position === undefined) {
            throw new Error(`${file}: clientView[${index}] 的 ${kind} 观测必须给出 position`);
        }
        return {
            key,
            kind,
            ...(position === undefined ? {} : { position }),
            ...(radius === undefined ? {} : { radius }),
        };
    });
}

/** knownDiff 路径必须精确到步骤、探针或客户端观测，不允许整条用例豁免。 */
function validateKnownDiffPath(diffPath: string, file: string): void {
    if (
        /^steps\[\d+\]\.feedback$/.test(diffPath) ||
        /^probes\.[^.]+$/.test(diffPath) ||
        /^clientView\.[^.]+$/.test(diffPath)
    ) {
        return;
    }
    throw new Error(
        `${file}: knownDiffs.path 必须是 "steps[<i>].feedback"、"probes.<key>" 或 "clientView.<key>"，实际为 "${diffPath}"`,
    );
}

/** 取必填字符串字段。 */
function requireString(source: Record<string, unknown>, key: string, where: string): string {
    const value = source[key];
    if (typeof value !== "string" || value.length === 0) {
        throw new Error(`${where}: 缺少字符串字段 ${key}`);
    }
    return value;
}

/** 取必填数组字段。 */
function requireArray(source: Record<string, unknown>, key: string, where: string): unknown[] {
    const value = source[key];
    if (!Array.isArray(value)) {
        throw new Error(`${where}: 缺少数组字段 ${key}`);
    }
    return value;
}
