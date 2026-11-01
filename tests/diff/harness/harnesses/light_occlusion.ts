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
 * light_occlusion.ts — 方块状态的光照阻挡属性。
 *
 * 单元是方块 ID；每个方块输出它全部状态的四个值：lightBlock（每格衰减量）、
 * propagatesSkylightDown（天空光直通）、useShapeForLightOcclusion（按形状遮挡）、
 * canOcclude（遮挡总结开关）。它们决定光照引擎如何让一束光走过这一格。
 *
 * 这四个值是纯数据，抄错没有编译期提示：多衰减一级屋子里就差一格看不清，树叶判成
 * 不透光树冠下就长不出草。只能靠逐条比对发现。
 */

import type { HarnessDeclaration } from "../core/types.ts";

/**
 * 比对前的规范化。
 *
 * 两侧输出都是「状态名 → 四个值」的 map，但键的书写顺序不同（Cubium 走 map 插入序、
 * 原版走 TreeMap 之外的声明序），JSON 对象的键序本身没有语义。这里按名字重排一遍，
 * 让比较只关心内容。
 *
 * 失败侧（方块缺失）也要补齐字段：只带 `missing: true` 而另一边带着整张状态表时，
 * 差异必须体现出来，不能被"缺键"糊过去。
 */
export function normaliseLightOcclusion(_expected: unknown, actual: unknown): unknown {
    if (actual === null || typeof actual !== "object") {
        return actual;
    }
    const record = actual as Record<string, unknown>;
    const values = record["values"];
    if (values === null || typeof values !== "object") {
        return { missing: record["missing"] ?? false, values: null };
    }
    const entries = Object.entries(values as Record<string, unknown>)
        .sort(([left], [right]) => (left < right ? -1 : left > right ? 1 : 0));
    const normalised: Record<string, unknown> = {};
    for (const [name, value] of entries) {
        const fields = (value ?? {}) as Record<string, unknown>;
        normalised[name] = {
            lightBlock: fields["lightBlock"] ?? null,
            propagatesSkylightDown: fields["propagatesSkylightDown"] ?? null,
            useShapeForLightOcclusion: fields["useShapeForLightOcclusion"] ?? null,
            canOcclude: fields["canOcclude"] ?? null,
        };
    }
    return { missing: record["missing"] ?? false, values: normalised };
}

const GAP_REASON =
    "光照阻挡注册未逐方块对齐：原版大量方块在注册时显式调用 noOcclusion()/noCollision()"
    + "（旗帜、床、潜影盒、告示牌、花草、玻璃板、冰、炼药锅等），Cubium 只做了一部分，"
    + "其余仍按材质默认值推导出 canOcclude=true，进而把 lightBlock 算成 15。"
    + "根因已定位在 BlockProperties 的注册缺项，修法是为下列方块补上对应注册开关"
    + "（每个方块按原版 Blocks.java 的 register 调用逐条抄）；这属于跨数十个方块的一轮独立工作，"
    + "本轮先登记，避免新缺陷被掩盖。";

normaliseLightOcclusion.canonicalising = true;

/** 能力声明。 */
export function lightOcclusionDeclaration(): HarnessDeclaration {
    return {
        name: "light_occlusion",
        probe: "light_occlusion",
        oracle: "LightOcclusionOracle",
        // 单元是方块（一次返回它全部状态），不是单个状态。
        key: "block",
        units: { kind: "listing" },
        input: (unit: string): { block: string } => ({ block: unit }),
        normalise: normaliseLightOcclusion,
        // 已定位根因、修法写进 TODO，但需要单独一轮工作的缺口（见 GAP_REASON）。
        // 修好后必须连同清单一起删除，否则新缺陷会被一起吞掉。
        knownDifferences: [
                { key: "minecraft:bamboo", field: "values", reason: GAP_REASON },
                { key: "minecraft:bamboo_mosaic_stairs", field: "values", reason: GAP_REASON },
                { key: "minecraft:barrier", field: "values", reason: GAP_REASON },
                { key: "minecraft:beacon", field: "values", reason: GAP_REASON },
                { key: "minecraft:bell", field: "values", reason: GAP_REASON },
                { key: "minecraft:black_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:black_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:black_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:black_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:black_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_ice", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:blue_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:brown_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:brown_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:brown_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:brown_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:brown_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:bubble_column", field: "values", reason: GAP_REASON },
                { key: "minecraft:budding_amethyst", field: "values", reason: GAP_REASON },
                { key: "minecraft:calcite", field: "values", reason: GAP_REASON },
                { key: "minecraft:cauldron", field: "values", reason: GAP_REASON },
                { key: "minecraft:comparator", field: "values", reason: GAP_REASON },
                { key: "minecraft:conduit", field: "values", reason: GAP_REASON },
                { key: "minecraft:copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:crafter", field: "values", reason: GAP_REASON },
                { key: "minecraft:creeper_wall_head", field: "values", reason: GAP_REASON },
                { key: "minecraft:crimson_fungus", field: "values", reason: GAP_REASON },
                { key: "minecraft:cyan_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:cyan_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:cyan_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:cyan_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:cyan_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:daylight_detector", field: "values", reason: GAP_REASON },
                { key: "minecraft:dirt_path", field: "values", reason: GAP_REASON },
                { key: "minecraft:dragon_wall_head", field: "values", reason: GAP_REASON },
                { key: "minecraft:dried_kelp_block", field: "values", reason: GAP_REASON },
                { key: "minecraft:enchanting_table", field: "values", reason: GAP_REASON },
                { key: "minecraft:end_portal_frame", field: "values", reason: GAP_REASON },
                { key: "minecraft:end_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:end_stone", field: "values", reason: GAP_REASON },
                { key: "minecraft:ender_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:exposed_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:farmland", field: "values", reason: GAP_REASON },
                { key: "minecraft:fire", field: "values", reason: GAP_REASON },
                { key: "minecraft:glass", field: "values", reason: GAP_REASON },
                { key: "minecraft:glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:glow_lichen", field: "values", reason: GAP_REASON },
                { key: "minecraft:gray_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:gray_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:gray_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:gray_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:gray_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:green_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:green_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:green_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:green_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:green_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:honey_block", field: "values", reason: GAP_REASON },
                { key: "minecraft:hopper", field: "values", reason: GAP_REASON },
                { key: "minecraft:iron_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:ladder", field: "values", reason: GAP_REASON },
                { key: "minecraft:lava_cauldron", field: "values", reason: GAP_REASON },
                { key: "minecraft:lectern", field: "values", reason: GAP_REASON },
                { key: "minecraft:lever", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_blue_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_blue_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_blue_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_blue_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_blue_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_gray_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_gray_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_gray_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_gray_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:light_gray_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:lime_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:lime_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:lime_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:lime_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:lime_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:magenta_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:magenta_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:magenta_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:magenta_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:magenta_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:moss_carpet", field: "values", reason: GAP_REASON },
                { key: "minecraft:moving_piston", field: "values", reason: GAP_REASON },
                { key: "minecraft:mushroom_stem", field: "values", reason: GAP_REASON },
                { key: "minecraft:mycelium", field: "values", reason: GAP_REASON },
                { key: "minecraft:nether_quartz_ore", field: "values", reason: GAP_REASON },
                { key: "minecraft:ochre_froglight", field: "values", reason: GAP_REASON },
                { key: "minecraft:orange_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:orange_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:orange_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:orange_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:orange_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:oxidized_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:packed_ice", field: "values", reason: GAP_REASON },
                { key: "minecraft:pearlescent_froglight", field: "values", reason: GAP_REASON },
                { key: "minecraft:piglin_wall_head", field: "values", reason: GAP_REASON },
                { key: "minecraft:pink_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:pink_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:pink_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:pink_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:pink_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:piston", field: "values", reason: GAP_REASON },
                { key: "minecraft:piston_head", field: "values", reason: GAP_REASON },
                { key: "minecraft:player_wall_head", field: "values", reason: GAP_REASON },
                { key: "minecraft:pointed_dripstone", field: "values", reason: GAP_REASON },
                { key: "minecraft:powder_snow", field: "values", reason: GAP_REASON },
                { key: "minecraft:powder_snow_cauldron", field: "values", reason: GAP_REASON },
                { key: "minecraft:purple_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:purple_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:purple_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:purple_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:purple_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:red_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:red_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:red_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:red_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:red_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:redstone_lamp", field: "values", reason: GAP_REASON },
                { key: "minecraft:reinforced_deepslate", field: "values", reason: GAP_REASON },
                { key: "minecraft:repeater", field: "values", reason: GAP_REASON },
                { key: "minecraft:scaffolding", field: "values", reason: GAP_REASON },
                { key: "minecraft:sculk_shrieker", field: "values", reason: GAP_REASON },
                { key: "minecraft:sculk_vein", field: "values", reason: GAP_REASON },
                { key: "minecraft:skeleton_wall_skull", field: "values", reason: GAP_REASON },
                { key: "minecraft:snow", field: "values", reason: GAP_REASON },
                { key: "minecraft:soul_fire", field: "values", reason: GAP_REASON },
                { key: "minecraft:spawner", field: "values", reason: GAP_REASON },
                { key: "minecraft:sticky_piston", field: "values", reason: GAP_REASON },
                { key: "minecraft:stonecutter", field: "values", reason: GAP_REASON },
                { key: "minecraft:tall_seagrass", field: "values", reason: GAP_REASON },
                { key: "minecraft:test_instance_block", field: "values", reason: GAP_REASON },
                { key: "minecraft:tinted_glass", field: "values", reason: GAP_REASON },
                { key: "minecraft:twisting_vines_plant", field: "values", reason: GAP_REASON },
                { key: "minecraft:vault", field: "values", reason: GAP_REASON },
                { key: "minecraft:verdant_froglight", field: "values", reason: GAP_REASON },
                { key: "minecraft:warped_fungus", field: "values", reason: GAP_REASON },
                { key: "minecraft:water", field: "values", reason: GAP_REASON },
                { key: "minecraft:water_cauldron", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_exposed_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_oxidized_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:waxed_weathered_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_copper_bars", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_copper_chain", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_copper_chest", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_copper_grate", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_copper_lantern", field: "values", reason: GAP_REASON },
                { key: "minecraft:weathered_lightning_rod", field: "values", reason: GAP_REASON },
                { key: "minecraft:weeping_vines_plant", field: "values", reason: GAP_REASON },
                { key: "minecraft:white_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:white_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:white_shulker_box", field: "values", reason: GAP_REASON },
                { key: "minecraft:white_stained_glass_pane", field: "values", reason: GAP_REASON },
                { key: "minecraft:white_wall_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:wither_skeleton_wall_skull", field: "values", reason: GAP_REASON },
                { key: "minecraft:yellow_banner", field: "values", reason: GAP_REASON },
                { key: "minecraft:yellow_bed", field: "values", reason: GAP_REASON },
                { key: "minecraft:yellow_shulker_box", field: "values", reason: GAP_REASON },
        { key: "minecraft:yellow_stained_glass_pane", field: "values", reason: GAP_REASON },
        { key: "minecraft:yellow_wall_banner", field: "values", reason: GAP_REASON },
        { key: "minecraft:zombie_wall_head", field: "values", reason: GAP_REASON },
        ],
    };
}