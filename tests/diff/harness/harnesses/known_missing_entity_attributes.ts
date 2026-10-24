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
 * known_missing_entity_attributes.ts — 原版有默认属性、Cubium 侧的实体尚未实现的类型。
 *
 * 这是**实现缺口**，不是参照数据缺失：这些类型在 Cubium 的实体注册表里还不存在
 * （船/竹筏、展示实体、药水投射物、方块展示架等）。清单显式列出、每次运行输出条数；
 * 补齐后必须删掉对应条目，否则新实现的类型会被静默跳过。
 */
export const KNOWN_MISSING: readonly string[] = [
    // 凋灵在 Cubium 里直接继承 MobEntity（没有 ATTACK_DAMAGE 属性），而原版默认供应器里
    // 的 2.0 只是 Monster 工厂的默认值、凋灵本身也不读它（伤害来自颅骨）。这项既不
    // 影响玩法也无法在不改继承链的前提下对齐，先记为已知缺口。
    "minecraft:wither",
    // 盔甲架在 Cubium 里直接继承 Entity（没有 LivingEntity 的属性系统），
    // 迁移计划见 entities/effect/EffectEntities.hpp 的 ArmorStandEntity 注释。
    "minecraft:armor_stand",
    "minecraft:acacia_boat",
    "minecraft:acacia_chest_boat",
    "minecraft:allay",
    "minecraft:bamboo_chest_raft",
    "minecraft:bamboo_raft",
    "minecraft:birch_boat",
    "minecraft:birch_chest_boat",
    "minecraft:block_display",
    "minecraft:breeze_wind_charge",
    "minecraft:cherry_boat",
    "minecraft:cherry_chest_boat",
    "minecraft:command_block_minecart",
    "minecraft:creaking",
    "minecraft:dark_oak_boat",
    "minecraft:dark_oak_chest_boat",
    "minecraft:interaction",
    "minecraft:item_display",
    "minecraft:jungle_boat",
    "minecraft:jungle_chest_boat",
    "minecraft:lingering_potion",
    "minecraft:mangrove_boat",
    "minecraft:mangrove_chest_boat",
    "minecraft:mannequin",
    "minecraft:marker",
    "minecraft:oak_boat",
    "minecraft:oak_chest_boat",
    "minecraft:pale_oak_boat",
    "minecraft:pale_oak_chest_boat",
    "minecraft:splash_potion",
    "minecraft:spruce_boat",
    "minecraft:spruce_chest_boat",
    "minecraft:text_display",
];
