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

/**
 * enchantment_selection.ts — 附魔台可选附魔列表能力的输入构造。
 *
 * 单元形如 `物品 id|附魔等级`，由探针与 oracle 的 `list` 各自枚举同一张「物品 × 等级」表。
 */

/** 附魔选择能力的一个单元。 */
export interface EnchantmentSelectionUnit {
    readonly item: string;
    readonly level: number;
}

/**
 * 单元字符串 → 探针与 oracle 共用的一行输入。
 *
 * `item` 按字符串传递、`level` 按整数传递：物品 id 本身含 `:`，用 `|` 分隔最右一段作为等级，
 * 避免 id 中出现的字符与分隔符冲突。
 */
export function enchantmentSelectionInput(unit: string): EnchantmentSelectionUnit {
    const separator = unit.lastIndexOf("|");
    if (separator < 0) {
        throw new Error(`enchantment_selection: 单元应为 '物品id|等级'：${unit}`);
    }
    const level = Number(unit.slice(separator + 1));
    if (!Number.isInteger(level)) {
        throw new Error(`enchantment_selection: 等级应为整数：${unit}`);
    }
    return { item: unit.slice(0, separator), level };
}
/** 覆盖用的物品表，与探针 / oracle 的 `list` 逐项一致。 */
const ITEMS = [
    "minecraft:book",
    "minecraft:diamond_sword",
    "minecraft:diamond_pickaxe",
    "minecraft:diamond_axe",
    "minecraft:diamond_helmet",
    "minecraft:diamond_boots",
    "minecraft:bow",
    "minecraft:fishing_rod",
    "minecraft:trident",
    "minecraft:elytra",
    "minecraft:shield",
    "minecraft:air",
    "minecraft:dirt",
    "minecraft:stick",
] as const;

/** 覆盖附魔等级代价曲线的关键取值，与探针 / oracle 的 `list` 逐项一致。 */
const LEVELS = [0, 1, 5, 10, 15, 20, 25, 30, 35, 40, 50, 100] as const;

/**
 * 生成「物品 × 等级」的全部单元。
 *
 * 这里是 `computed` 而不是 `listing`：单元数量由两张表相乘得出，在 TS 侧枚举能让
 * 单元顺序与探针 / oracle 的 `list` 完全一致，而 `computed` 的求值早于注入
 * `MC_DIFF_VANILLA_DATAPACK`，本函数不读数据包，因此不受影响。
 */
export function enchantmentSelectionUnits(): string[] {
    const units: string[] = [];
    for (const item of ITEMS) {
        for (const level of LEVELS) {
            units.push(`${item}|${level}`);
        }
    }
    return units;
}