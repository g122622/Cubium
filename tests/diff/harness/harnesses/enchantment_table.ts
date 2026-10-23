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
 * enchantment_table.ts — 附魔等级（Cubium vs minecraft-data 的 enchantments）。
 *
 * 只比 maxLevel：minecraft-data 的 minCost/maxCost 是同一组 {a,b}，没有区分
 * 「最小代价」与「最大代价」的公式，直接拿来比会把参照数据的绝对值当成公式，
 * 得到看似精确实则错误的结论。等级上限是硬数据，可以逐条比。
 */

import { loadMinecraftData } from "../vanilla.ts";
import type { HarnessSpec } from "../runner.ts";

interface RawEnchantment {
    name: string;
    maxLevel: number;
}

export function createEnchantmentTableHarness(): HarnessSpec {
    const md = loadMinecraftData()("1.21.11") as unknown as { enchantmentsArray: RawEnchantment[] };
    const enchantments = md.enchantmentsArray;
    const byName = new Map(enchantments.map((enchantment) => [enchantment.name, enchantment]));

    return {
        name: "enchantment_table",
        domain: "enchantment_table",
        key: "enchantment",
        units: () => enchantments.map((enchantment) => `minecraft:${enchantment.name}`).sort(),
        input: (unit) => ({ enchantment: unit, levels: [1] }),
        oracle: (unit) => {
            const enchantment = byName.get(unit.replace(/^minecraft:/, ""));
            if (enchantment === undefined) {
                return { enchantment: unit, missing: true };
            }
            return { enchantment: unit, minLevel: 1, maxLevel: enchantment.maxLevel };
        },
    } satisfies HarnessSpec;
}