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
 * item_summary.ts — 物品堆叠上限与食物属性（Cubium vs minecraft-data 的 items/foods）。
 *
 * 比对：堆叠上限、是否为食物、饥饿值、饱和度修正值、饱和度。
 *
 * minecraft-data 的 foods 字段语义（用 11 种 vanilla 已知值逐条反推核对过）：
 *   foodPoints      = nutrition（饥饿值恢复量）
 *   saturation      = nutrition × saturationModifier × 16
 *   saturationRatio = saturationModifier × 4 × nutrition
 * 后两者的绝对值都带了 nutrition 与固定系数，不是 vanilla 的原始数值。差分要比 vanilla 口径，
 * 因此反推回：
 *   modifier = saturationRatio / (4 × foodPoints)
 * 再按与 probe **完全相同**的 f32 运算顺序重算饱和度，任何一步舍入差异都会暴露。
 */

import { f32, f32Bits } from "./shared.ts";

interface RawItem {
    name: string;
    stackSize: number;
}

interface RawFood {
    name: string;
    foodPoints: number;
    saturation: number;
    saturationRatio: number;
}

/** 参照数据：minecraft-data 的 items/foods 表。 */
interface MinecraftData {
    itemsArray: RawItem[];
    foodsArray: RawFood[];
}

/**
 * 计算一个物品的参照输出。
 *
 * @param md minecraft-data 的原始表（由调用方加载：主进程不需要它，执行进程才读）
 * @param unit 物品 ID（`minecraft:stone`）
 */
export function itemSummaryReference(md: MinecraftData, unit: string): unknown {
    const name = unit.replace(/^minecraft:/, "");
    const item = md.itemsArray.find((candidate) => candidate.name === name);
    if (item === undefined) {
        return { item: unit, missing: true };
    }
    const food = md.foodsArray.find((candidate) => candidate.name === name);
    if (food === undefined) {
        return { item: unit, maxStackSize: item.stackSize, isFood: false };
    }
    // ratio = modifier × 4 × nutrition，反推修正值后再按 probe 的运算顺序重算饱和度。
    const modifier = f32(f32(food.saturationRatio) / f32(f32(4) * f32(food.foodPoints)));
    const saturation = f32(f32(f32(food.foodPoints) * modifier) * f32(2));
    return {
        item: unit,
        maxStackSize: item.stackSize,
        isFood: true,
        hunger: food.foodPoints,
        saturationModifier: f32Bits(modifier),
        saturation: f32Bits(saturation),
    };
}

/** 按 minecraft-data 枚举全部物品 ID。 */
export function itemSummaryUnits(md: MinecraftData): string[] {
    return md.itemsArray.map((item) => `minecraft:${item.name}`).sort();
}