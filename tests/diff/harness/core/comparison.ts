/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * comparison.ts — 两侧结果的对齐、投影与比较，一次算出一个摘要。
 *
 * 语义（与旧的逐行比较一致，这里只是把「比较」与「落盘」拆开）：
 *   - 按 `key` 字段对齐两侧的 JSON 行，行序无关；
 *   - 只比参照实现声明过的字段：参照没给的信息不是「不一致」，而是「没得比」；
 *   - 数组必须等长；只有调用方显式声明为无序的路径才按规范序重排；
 *   - 浮点差异由调用方提供的 compareTransform 先处理（例如按固定小数位舍入），
 *     不在比较层统一放宽。
 *
 * 返回摘要而不是把两棵树一起留在内存里：全量差异可能有几千条，报告文件只保留前 200 条，
 * 主进程也只需要条数与几条样本。
 */

/** 一处单元级差异。 */
export interface Difference {
    readonly key: string;
    readonly expected: unknown;
    readonly actual: unknown;
}

/**
 * 两侧结果的投影：对象只保留参照声明过的字段，数组按需规范化。
 *
 * @param template 参照一侧的值，决定「哪些字段算数」
 * @param actual 被比较一侧的值
 * @param unorderedArrays 视为无序的数组路径（用 `/` 连接字段名，根路径为 `""`）
 */
export function projectDeclaredFields(
    template: unknown,
    actual: unknown,
    unorderedArrays: readonly string[] = [],
    path = "",
): unknown {
    if (Array.isArray(template)) {
        if (!Array.isArray(actual) || template.length !== actual.length) {
            return actual;
        }
        const projected = template.map((item, index) =>
            projectDeclaredFields(item, actual[index], unorderedArrays, path),
        );
        return unorderedArrays.includes(path) ? sortCanonically(projected) : projected;
    }
    if (template !== null && typeof template === "object") {
        if (actual === null || typeof actual !== "object" || Array.isArray(actual)) {
            return actual;
        }
        const source = actual as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const [field, expected] of Object.entries(template as Record<string, unknown>)) {
            const childPath = path === "" ? field : `${path}/${field}`;
            result[field] = projectDeclaredFields(expected, source[field], unorderedArrays, childPath);
        }
        return result;
    }
    return actual;
}

/**
 * 按规范顺序重排：先把元素自身递归规范化（对象键排序），再取其 JSON 文本排序。
 *
 * 只保证「同一组元素、不同排列」比较结果相同；元素内部是否还有无序数组由调用方声明。
 */
export function sortCanonically(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value
            .map((item) => sortCanonically(item))
            .sort((left, right) => {
                const leftKey = JSON.stringify(left) ?? "";
                const rightKey = JSON.stringify(right) ?? "";
                return leftKey < rightKey ? -1 : leftKey > rightKey ? 1 : 0;
            });
    }
    if (value !== null && typeof value === "object") {
        const source = value as Record<string, unknown>;
        const result: Record<string, unknown> = {};
        for (const field of Object.keys(source).sort()) {
            result[field] = sortCanonically(source[field]);
        }
        return result;
    }
    return value;
}

/** 规范化后的 JSON 文本：对象键按字典序排列，数组保持原顺序。 */
export function stableStringify(value: unknown): string {
    const normalise = (node: unknown): unknown => {
        if (Array.isArray(node)) {
            return node.map(normalise);
        }
        if (node !== null && typeof node === "object") {
            const record = node as Record<string, unknown>;
            const sorted: Record<string, unknown> = {};
            for (const key of Object.keys(record).sort()) {
                sorted[key] = normalise(record[key]);
            }
            return sorted;
        }
        return node;
    };
    return JSON.stringify(normalise(value));
}

/**
 * 比较前的规范化：对**两侧**的同一个位置各调一次，返回被测侧要参与比较的值。
 *
 * 第一个参数是参照侧的值、第二个是被测侧的值，方便按「参照侧的精度」舍入被测侧而又不改动参照侧。
 */
export type CompareTransform = (expected: unknown, actual: unknown) => unknown;

/** 比较选项，与 harness 定义一一对应。 */
export interface CompareOptions {
    /** 输出中标识单元的字段名，如 "block"。 */
    readonly key: string;
    /** 比较前对两侧输出的规范化（例如舍入浮点、归一化等价类型名）。见 {@link CompareTransform}。 */
    readonly compareTransform?: CompareTransform;
    /** 语义无序的数组路径。 */
    readonly unorderedArrays?: readonly string[];
    /** 成员级缺口：从**参照一侧**剔除的成员（字段名 + 成员集合）。 */
    readonly memberGaps?: { readonly field: string; readonly missing: readonly string[] };
    /** 本项目独有、参照没有的成员：从**被测一侧**剔除。 */
    readonly projectOnly?: {
        readonly field: string;
        readonly extra: readonly string[];
        readonly drop?: (record: Record<string, unknown>, matches: (name: string) => boolean) => Record<string, unknown>;
    };
}

/**
 * 从两个已投影的值里挑出「不相等」的部分，返回只含这些部分的副本。
 *
 * 相等性用 {@link stableStringify} 判定（与比较口径一致：对象键序无关、数组顺序有关），
 * 只在报告里裁剪，不改变判定。裁剪会一路下探对象与**等长数组**：
 *
 *   - 命令树的差异几乎总是 `arguments` 里某几个参数，而 `tree`/`components`/`wire` 有上万字符；
 *     原样塞进报告会让终端完全不可读，也让报告文件失去可检索性；
 *   - 数组只在两侧等长时逐元素裁剪（元素一一对应），不等长就整体保留——长度差本身就是结论；
 *   - 某个字段一侧缺失时保留另一侧的原值，读者能看出「那一侧没有它」。
 */
function differingFields(value: unknown, other: unknown): unknown {
    if (value === null || typeof value !== "object") {
        return value;
    }
    if (Array.isArray(value)) {
        if (!Array.isArray(other) || other.length !== value.length) {
            return value;
        }
        return value.map((item, index) => {
            if (stableStringify(item) === stableStringify(other[index])) {
                return undefined;
            }
            const trimmed = differingFields(item, other[index]);
            return trimmed === undefined ? item : trimmed;
        }).map((item, index) => (item === undefined ? "<相等>" : item));
    }
    if (other === null || typeof other !== "object" || Array.isArray(other)) {
        return value;
    }
    const left = value as Record<string, unknown>;
    const right = other as Record<string, unknown>;
    const result: Record<string, unknown> = {};
    for (const [field, fieldValue] of Object.entries(left)) {
        if (!(field in right)) {
            result[field] = fieldValue;
            continue;
        }
        if (stableStringify(fieldValue) === stableStringify(right[field])) {
            continue;
        }
        const trimmed = differingFields(fieldValue, right[field]);
        result[field] = trimmed === undefined ? fieldValue : trimmed;
    }
    for (const [field, fieldValue] of Object.entries(right)) {
        if (!(field in left)) {
            result[field] = fieldValue;
        }
    }
    return result;
}

/** 一次比较的结果。 */
export interface ComparisonSummary {
    readonly units: number;
    readonly matched: number;
    readonly differences: readonly Difference[];
    /** 被 memberGaps 剔除的成员条数。 */
    readonly memberGapDropped: number;
    /** 被 projectOnly 剔除的成员条数。 */
    readonly projectOnlyDropped: number;
}

/**
 * 对齐两侧 JSON 行并逐字段比较。
 *
 * @param expectedLines 参照实现输出（每行一个对象）
 * @param actualLines 被测实现输出
 * @param options 比较口径
 */
export function compareLines(
    expectedLines: readonly string[],
    actualLines: readonly string[],
    options: CompareOptions,
): ComparisonSummary {
    const transform = options.compareTransform;

    // 成员级缺口：只在参照一侧剔除；被测多出来的成员永远算差异。
    let memberGapDropped = 0;
    let expected = expectedLines;
    if (options.memberGaps !== undefined) {
        const { field, missing } = options.memberGaps;
        const exempt = new Set(missing);
        expected = expectedLines.map((line) => {
            const parsed = JSON.parse(line) as Record<string, unknown>;
            const values = parsed[field];
            if (!Array.isArray(values)) {
                return line;
            }
            const kept = values.filter((value) => !exempt.has(String(value)));
            memberGapDropped += values.length - kept.length;
            return JSON.stringify({ ...parsed, [field]: kept });
        });
    }

    // 本项目独有能力：只从被测一侧剔除，派生字段两侧一起清理。
    let actual = actualLines;
    let projectOnlyDropped = 0;
    if (options.projectOnly !== undefined) {
        const { field, extra, drop } = options.projectOnly;
        const exempt = new Set(extra);
        const matches = (value: string): boolean =>
            exempt.has(value) || [...exempt].some((name) => value.startsWith(`${name} `));
        actual = actualLines.map((line) => {
            const parsed = JSON.parse(line) as Record<string, unknown>;
            const values = parsed[field];
            let next: Record<string, unknown> = parsed;
            if (Array.isArray(values)) {
                const kept = values.filter((value) => !matches(String(value)));
                projectOnlyDropped += values.length - kept.length;
                next = { ...next, [field]: kept };
            }
            return JSON.stringify(drop === undefined ? next : drop(next, matches));
        });
        if (drop !== undefined) {
            expected = expected.map((line) => JSON.stringify(drop(JSON.parse(line) as Record<string, unknown>, matches)));
        }
    }

    const index = (lines: readonly string[], side: string): Map<string, unknown> => {
        const map = new Map<string, unknown>();
        lines.forEach((line, position) => {
            let parsed: unknown;
            try {
                parsed = JSON.parse(line);
            } catch {
                throw new Error(`${side} 第 ${position + 1} 行不是合法 JSON：${line.slice(0, 200)}`);
            }
            const unit = (parsed as Record<string, unknown>)[options.key];
            if (typeof unit !== "string") {
                throw new Error(
                    `${side} 第 ${position + 1} 行缺少字符串字段 ${JSON.stringify(options.key)}：${line.slice(0, 200)}`,
                );
            }
            map.set(unit, parsed);
        });
        return map;
    };

    const reference = index(expected, "参照实现");
    const observed = index(actual, "probe");
    const differences: Difference[] = [];
    const units = [...new Set([...reference.keys(), ...observed.keys()])].sort();
    /** 两侧投影后的规范化文本：相等即该单元通过。 */
    const projection = (value: unknown, transformed: unknown): string =>
        stableStringify(projectDeclaredFields(value, transformed, options.unorderedArrays));
    for (const unit of units) {
        const referenceValue = reference.get(unit);
        const observedValue = observed.get(unit);
        if (referenceValue === undefined) {
            differences.push({ key: unit, expected: "<absent in oracle>", actual: observedValue });
            continue;
        }
        if (observedValue === undefined) {
            differences.push({ key: unit, expected: referenceValue, actual: "<absent in probe>" });
            continue;
        }
        const transformed = transform === undefined ? observedValue : transform(referenceValue, observedValue);
        if (projection(referenceValue, referenceValue) === projection(referenceValue, transformed)) {
            continue;
        }
        // 报告里只保留差异涉及的字段：原始值可能有上万字符（命令树的线协议字节），
        // 把整份原样塞进报告既让终端无法阅读，也让报告文件失去可 grep 性。
        const expectedProjection = projectDeclaredFields(referenceValue, referenceValue, options.unorderedArrays);
        const actualProjection = projectDeclaredFields(referenceValue, transformed, options.unorderedArrays);
        differences.push({
            key: unit,
            expected: differingFields(expectedProjection, actualProjection),
            actual: differingFields(actualProjection, expectedProjection),
        });
    }
    const differing = new Set(differences.map((difference) => difference.key)).size;
    return {
        units: units.length,
        matched: units.length - differing,
        differences,
        memberGapDropped,
        projectOnlyDropped,
    };
}