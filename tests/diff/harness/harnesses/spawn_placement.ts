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
 * 与 Horizon test_spawn_rules_parity 相同：所有实体遍历同一组受控方块柱。
 * 本能力只测不消耗随机数的位置检查；生成谓词和随机数序列另行比较。
 */
interface PlacementCase {
    readonly name: string;
    readonly below: string;
    readonly at: string;
    readonly above: string;
    readonly insideBorder: boolean;
    readonly y: number;
}

const AIR = "minecraft:air";
const STONE = "minecraft:stone";

// 每种状态分别放到脚下、脚部、头部，避免只检查默认空气而漏掉危险方块或支撑面。
const STATES = [
    "air", "stone", "grass_block", "sand", "red_sand", "deepslate", "netherrack", "end_stone",
    "mycelium", "podzol", "mud", "soul_sand", "soul_soil", "glass", "tinted_glass",
    "white_stained_glass", "ice", "packed_ice", "blue_ice", "frosted_ice", "snow_block",
    "oak_leaves", "jungle_leaves", "azalea_leaves", "glowstone", "sea_lantern", "magma_block",
    "oak_slab[type=bottom]", "oak_slab[type=top]", "oak_slab[type=double]",
    "oak_stairs[half=bottom]", "oak_stairs[half=top]", "oak_trapdoor[open=false,half=top]",
    "oak_trapdoor[open=true]", "water", "water[level=3]", "lava", "lava[level=5]",
    "oak_slab[waterlogged=true]", "seagrass", "kelp", "kelp_plant", "bubble_column",
    "fire", "soul_fire", "wither_rose", "sweet_berry_bush", "cactus", "powder_snow",
    "campfire[lit=true]", "campfire[lit=false]", "soul_campfire[lit=true]", "lava_cauldron",
    "rail", "powered_rail", "redstone_wire", "redstone_torch", "redstone_torch[lit=false]",
    "lever", "stone_button", "stone_pressure_plate", "short_grass", "snow[layers=1]",
    "snow[layers=8]", "cobweb", "scaffolding", "barrier", "bedrock", "honey_block", "slime_block",
    "chest", "chest[type=left]", "chest[type=right,facing=west]", "trapped_chest",
    "farmland", "dirt_path", "white_carpet",
];

export const PLACEMENT_CASES: readonly PlacementCase[] = [
    ...STATES.flatMap((state): PlacementCase[] => {
        const block = `minecraft:${state}`;
        return [
            { name: `below/${state}`, below: block, at: AIR, above: AIR, insideBorder: true, y: 64 },
            { name: `feet/${state}`, below: STONE, at: block, above: AIR, insideBorder: true, y: 64 },
            { name: `head/${state}`, below: STONE, at: AIR, above: block, insideBorder: true, y: 64 },
            { name: `water-roof/${state}`, below: AIR, at: "minecraft:water", above: block, insideBorder: true, y: 64 },
        ];
    }),
    ...[true, false].flatMap((insideBorder) =>
        ["air", "water", "lava"].flatMap((fluid) =>
            [-65, -64, 319, 320].map((y): PlacementCase => ({
                name: `bounds/${insideBorder}/${fluid}/${y}`,
                below: STONE, at: `minecraft:${fluid}`, above: AIR, insideBorder, y,
            })))),
];

export function spawnPlacementInput(entity: string): unknown {
    return { entity, cases: PLACEMENT_CASES };
}
