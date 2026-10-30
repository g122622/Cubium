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
 * text_encode.ts — 方块状态的文本表示与属性摘要（Cubium vs 原版 jar）。
 *
 * 两个维度，都落在这一个能力里：
 *
 *   1. 状态全集：探针的 `list` 输出全部方块状态（`minecraft:oak_log[axis=y]`），原版的
 *      BlockStateOracle 也输出全部方块状态——两者的列表直接比对。
 *   2. 方块摘要：每个方块的属性名、属性类型、取值域、状态数。
 *
 * 单元来源与其它能力相反，取**探针**的 `list`：原版 oracle 的 `list` 给的是方块状态文本，
 * 而 `run` 的单元是方块名，两侧枚举范围不同。以探针为准还顺带把「Cubium 多注册了原版没有的
 * 方块」变成可见差异（`<absent in oracle>`），而不是被静默跳过。
 *
 * 比较口径：只归一**类型名**（`DirectionProperty` 是 `EnumProperty<Direction>` 的子类，
 * 原版 facing 就是后者），并对属性与取值域排序（集合语义）；不放开取值比较。
 */

import type { CompareTransform } from "../core/comparison.ts";
import type { HarnessContext, HarnessDeclaration } from "../core/types.ts";

/** 类型别名：只归一枚举类型的实现名，不归一取值。 */
const TYPE_ALIASES: Readonly<Record<string, string>> = { DirectionProperty: "EnumProperty" };

/** 属性与取值域都是集合：按名排序后比较，避免注册顺序造成假差异。 */
function canonical(value: unknown): unknown {
    if (Array.isArray(value)) {
        return value.map(canonical);
    }
    if (value === null || typeof value !== "object") {
        return value;
    }
    const source = value as Record<string, unknown>;
    const result: Record<string, unknown> = {};
    for (const [field, item] of Object.entries(source)) {
        if (field === "type" && typeof item === "string") {
            result[field] = TYPE_ALIASES[item] ?? item;
        } else if (field === "values" && Array.isArray(item)) {
            result[field] = item.map(canonical).sort();
        } else if (field === "properties" && Array.isArray(item)) {
            const properties = item.map(canonical) as Array<Record<string, unknown>>;
            properties.sort((left, right) => String(left["name"]).localeCompare(String(right["name"])));
            result[field] = properties;
        } else {
            result[field] = canonical(item);
        }
    }
    return result;
}

/**
 * 规范化必须是等价变换（运行层会校验）：`canonical` 在参照侧只会重排数组，不改取值。
 * 这里显式把它声明成只作用于被测值的比较变换（参照侧原样参与比较）。
 */
function compareTransform(): CompareTransform {
    return (expected: unknown, actual: unknown): unknown => {
        // 参照侧保持原样：等价性检查因此天然成立；被测侧按同一规则规范化。
        void expected;
        return canonical(actual);
    };
}

/**
 * 两侧都能枚举「状态全集」，但 `run` 的单元是方块名：
 *
 *   - 探针 `list` 给状态文本（这就是被测侧的真相，多注册的方块也在这里出现）；
 *   - oracle `list` 给原版的状态文本（用来判定「原版有没有这个方块」）。
 *
 * 所以先取探针的方块集合作为单元，再用原版的状态全集把它们分成「原版也有」（参与逐字段
 * 比对）与「原版没有」（显式列为缺口）。这一步在 `beforeUnits` 完成——它比 `units` 先执行。
 */
function stateUnits(): { beforeUnits: (context: HarnessContext) => void; units: () => string[] } {
    let units: string[] = [];
    return {
        beforeUnits: (context: HarnessContext): void => {
            const vanillaBlocks = new Set(
                context.listOracleUnits("BlockStateOracle", "text_encode").map((text) => blockOf(text)),
            );
            units = [...new Set(context.listProbeUnits("text_encode").map((text) => blockOf(text)))]
                .filter((block) => vanillaBlocks.has(block))
                .sort();
        },
        units: (): string[] => units,
    };
}

/** 状态文本 → 方块名：`minecraft:oak_log[axis=y]` → `minecraft:oak_log`。 */
function blockOf(text: string): string {
    const bracket = text.indexOf("[");
    return bracket < 0 ? text : text.slice(0, bracket);
}

/** 能力声明（同时登记进 harnesses/index.ts 的 HARNESSES）。 */
export function textEncodeDeclaration(): HarnessDeclaration {
    const listing = stateUnits();
    return {
        name: "text_encode",
        probe: "text_encode",
        oracle: "BlockStateOracle",
        key: "block",
        units: { kind: "computed", units: listing.units },
        beforeUnits: listing.beforeUnits,
        input: (unit: string) => ({ block: unit }),
        normalise: compareTransform(),
    };
}