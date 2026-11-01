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
 * fuel_values.ts — 每种物品作为燃料的燃烧时长。
 *
 * 单元是物品 ID。两侧都只输出 `burnTime`，比的是玩家实际感受到的那个数：原版用标签
 * 聚合表达燃料表（`ItemTags.LOGS` 一条覆盖全部原木，再减去不可燃木材），Cubium 侧
 * 无论是标签还是逐条判断，只要最终时长对上就算一致。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * `burnTime` 为 0 表示「不是燃料」——这类物品占绝大多数，两侧都必须显式给出 0 而不是
 * 省略字段。失败侧（物品未注册）还要补齐 `burnTime`，否则「原版是燃料、本侧根本没注册
 * 这个物品」会因为缺键被静默通过。
 */
export function normaliseFuelValues(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    return {
        ok: record["ok"] ?? null,
        burnTime: record["burnTime"] ?? null,
    };
}

normaliseFuelValues.canonicalising = true;

/** 能力声明。 */
export function fuelValuesDeclaration(): HarnessDeclaration {
    return {
        name: "fuel_values",
        probe: "fuel_values",
        oracle: "FuelValuesOracle",
        key: "item",
        units: { kind: "listing" },
        input: (item: string): { item: string } => ({ item }),
        normalise: normaliseFuelValues,
        // oracle 要在 java.io.tmpdir 里建原版服务端世界并持有区域锁：同组必须串行。
        exclusiveGroup: "loot-world",
    };
}