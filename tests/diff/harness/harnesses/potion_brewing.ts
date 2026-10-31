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
 * potion_brewing.ts — 酿造配方能力的输入构造。
 *
 * 单元是「药水 × 试剂」的笛卡尔积：药水取原版全部 46 种，试剂取原版全部酿造材料。
 * 两侧对每组输入回答同三个问题：试剂是不是酿造材料、这组输入能否酿造、酿造后得到哪种药水。
 *
 * 试剂清单是**原版数据表**（`PotionBrewing.addContainer/addMix` 里出现的物品），
 * 由 `list` 不可枚举，因此在这里显式列出；只要两侧读同一份清单，比对就是公平的。
 */

/** 原版全部可用于酿造的物品（含容器材料与药水材料）。 */
const REAGENTS: readonly string[] = [
    "minecraft:nether_wart",
    "minecraft:redstone",
    "minecraft:glowstone_dust",
    "minecraft:fermented_spider_eye",
    "minecraft:gunpowder",
    "minecraft:dragon_breath",
    "minecraft:sugar",
    "minecraft:rabbit_foot",
    "minecraft:glistering_melon_slice",
    "minecraft:spider_eye",
    "minecraft:pufferfish",
    "minecraft:magma_cream",
    "minecraft:golden_carrot",
    "minecraft:blaze_powder",
    "minecraft:ghast_tear",
    "minecraft:turtle_helmet",
    "minecraft:phantom_membrane",
    "minecraft:breeze_rod",
    "minecraft:stone",
];

/** 原版全部药水 ID。 */
export const POTION_IDS: readonly string[] = [
    "minecraft:water",
    "minecraft:mundane",
    "minecraft:thick",
    "minecraft:awkward",
    "minecraft:night_vision",
    "minecraft:long_night_vision",
    "minecraft:invisibility",
    "minecraft:long_invisibility",
    "minecraft:leaping",
    "minecraft:long_leaping",
    "minecraft:strong_leaping",
    "minecraft:fire_resistance",
    "minecraft:long_fire_resistance",
    "minecraft:swiftness",
    "minecraft:long_swiftness",
    "minecraft:strong_swiftness",
    "minecraft:slowness",
    "minecraft:long_slowness",
    "minecraft:strong_slowness",
    "minecraft:turtle_master",
    "minecraft:long_turtle_master",
    "minecraft:strong_turtle_master",
    "minecraft:water_breathing",
    "minecraft:long_water_breathing",
    "minecraft:healing",
    "minecraft:strong_healing",
    "minecraft:harming",
    "minecraft:strong_harming",
    "minecraft:poison",
    "minecraft:long_poison",
    "minecraft:strong_poison",
    "minecraft:regeneration",
    "minecraft:long_regeneration",
    "minecraft:strong_regeneration",
    "minecraft:strength",
    "minecraft:long_strength",
    "minecraft:strong_strength",
    "minecraft:weakness",
    "minecraft:long_weakness",
    "minecraft:luck",
    "minecraft:slow_falling",
    "minecraft:long_slow_falling",
    "minecraft:wind_charged",
    "minecraft:weaving",
    "minecraft:oozing",
    "minecraft:infested",
];

/** 单元名：`<药水短名>+<试剂短名>`，报告里能直接看出是哪一组。 */
function unitName(potion: string, reagent: string): string {
    return `${potion.slice("minecraft:".length)}+${reagent.slice("minecraft:".length)}`;
}

/** 全部单元（药水 × 试剂）。 */
export function potionBrewingUnits(): string[] {
    const units: string[] = [];
    for (const potion of POTION_IDS) {
        for (const reagent of REAGENTS) {
            units.push(unitName(potion, reagent));
        }
    }
    return units;
}

/** 单元名 → 探针与 oracle 共用的一行输入。 */
export function potionBrewingInput(unit: string): Record<string, string> {
    const separator = unit.indexOf("+");
    if (separator < 0) {
        throw new Error(`potion_brewing：非法单元 ${unit}`);
    }
    return {
        unit,
        potion: `minecraft:${unit.slice(0, separator)}`,
        reagent: `minecraft:${unit.slice(separator + 1)}`,
    };
}