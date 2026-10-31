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

/** 原版 `EntityTypeTags` 静态声明的全部标签。
 *
 * 全集在这里显式列出，而不是靠 oracle 的 `list` 枚举：原版 `Registry#getTags()`
 * 只返回**运行时已实加载**的少数标签（实测 4 个），拿它当全集会让能力只覆盖那一小撮。
 */
export const ENTITY_TYPE_TAGS: readonly string[] = [
    "minecraft:accepts_iron_golem_gift",
    "minecraft:aquatic",
    "minecraft:arrows",
    "minecraft:arthropod",
    "minecraft:axolotl_always_hostiles",
    "minecraft:axolotl_hunt_targets",
    "minecraft:beehive_inhabitors",
    "minecraft:boat",
    "minecraft:burn_in_daylight",
    "minecraft:can_breathe_under_water",
    "minecraft:can_equip_harness",
    "minecraft:can_equip_saddle",
    "minecraft:can_float_while_ridden",
    "minecraft:can_turn_in_boats",
    "minecraft:can_wear_horse_armor",
    "minecraft:can_wear_nautilus_armor",
    "minecraft:candidate_for_iron_golem_gift",
    "minecraft:cannot_be_pushed_onto_boats",
    "minecraft:deflects_projectiles",
    "minecraft:dismounts_underwater",
    "minecraft:fall_damage_immune",
    "minecraft:followable_friendly_mobs",
    "minecraft:freeze_hurts_extra_types",
    "minecraft:freeze_immune_entity_types",
    "minecraft:frog_food",
    "minecraft:ignores_poison_and_regen",
    "minecraft:illager",
    "minecraft:illager_friends",
    "minecraft:immune_to_infested",
    "minecraft:immune_to_oozing",
    "minecraft:impact_projectiles",
    "minecraft:inverted_healing_and_harm",
    "minecraft:nautilus_hostiles",
    "minecraft:no_anger_from_wind_charge",
    "minecraft:non_controlling_rider",
    "minecraft:not_scary_for_pufferfish",
    "minecraft:powder_snow_walkable_mobs",
    "minecraft:raiders",
    "minecraft:redirectable_projectile",
    "minecraft:sensitive_to_bane_of_arthropods",
    "minecraft:sensitive_to_impaling",
    "minecraft:sensitive_to_smite",
    "minecraft:skeletons",
    "minecraft:undead",
    "minecraft:wither_friends",
    "minecraft:zombies",
];

/** `members` 是语义无序数组。 */
export const ENTITY_TYPE_TAGS_UNORDERED: readonly string[] = ["members"];

/** 标签 ID → 探针与 oracle 共用的一行输入。 */
export function entityTypeTagsInput(unit: string): { tag: string } {
    return { tag: unit };
}