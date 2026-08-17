// 补充发光方块等级测试：覆盖现有 BlockLightEmissionTests 未纳入的发光方块（对齐 wiki 发光方块表）。
//
// 与 BlockLightEmissionTests 同设计：light_box 内部地板 (3,1,3) 放发光方块，等传播稳定后断言该格
// blockLight 等于发光等级。light_box 封顶实心隔绝天空光，blockLight 是唯一光源。
//
// 本组覆盖的发光方块（核查确认已实现且与 wiki 一致）：
//   15: conduit（潮涌核心，ConduitBlock.hpp:126-133 getLightLevel override 恒返回15，无需潮涌框架激活）
//   14: end_rod（末地烛，NetherBlocks.cpp:678 lightLevel(14) 静态，noCollision 但存活）
//   10: crying_obsidian（哭泣的黑曜石，NetherBlocks.cpp:218 lightLevel(10) 静态）
//   5:  amethyst_cluster（紫水晶簇，lightLevel(5) 静态）
//   4:  large_amethyst_bud（大型紫晶芽，lightLevel(4)）
//   2:  medium_amethyst_bud（中型紫晶芽，lightLevel(2)）
//   1:  small_amethyst_bud（小型紫晶芽，lightLevel(1)）
//
// 未纳入（带 state/附着依赖，需单独构造场景，风险高，待后续）：
//   - cave_vines(fruits=true 需朝下附着)、furnace(lit=true 需 state)、glow_lichen(需多面 state)、
//     sea_pickle(需水中 waterlogged)、candle(需 candles count+lit)、respawn_anchor(需 charges)、
//     redstone_ore(lit=true 需 state)、fire/campfire(需附着面)、nether_portal(需 axis)。
//   TODO: 这些带 state 发光方块的等级测试待后续补充，需用 setBlockWithStates 构造对应前置状态。
//
// 已知偏差/未实现（不写测试）：
//   - copper_bulb 氧化变体（exposed/weathered/oxidized）：CopperBulbBlock.hpp:82-88 getLightLevel override 恒
//     `lit?15:0` 不分氧化度（wiki 15/12/8/4），偏差，跳过氧化变体 12/8/4；基础 copper_bulb 点亮=15/未点亮=0
//     与 wiki 一致，单独测试组 CopperBulbEmissionTests 覆盖。
//   - sculk_sensor/calibrated_sculk_sensor：注册但未设 lightLevel 也无 override（实际=0），wiki 期望1，偏差，跳过。
//   - end_portal_frame：静态 lightLevel(1)，但 wiki 是"有眼才发光1"，项目无眼也恒发光1，触发条件偏差，跳过。
//   - sculk_catalyst：实际已实现发光 6（注册处 SculkBlocks.cpp:85 .lightLevel(6)），与 wiki 6 一致，
//     测试见 StatefulEmissionTests.ts（本组此前注释误标「未实现」，已纠正）。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// light_box 内部地板中心格（y=0 stone 地板之上，下方有支撑可放置需附着的方块）。
const PLACE = { x: 3, y: 1, z: 3 };

/**
 * 通用发光等级测试：在 PLACE 放置发光方块，轮询断言该格 blockLight 等于 expected。
 * @param testName 注册测试名
 * @param blockType 方块 typeId
 * @param expected 期望发光等级 (0-15)
 */
function registerExtraEmissionTest(testName: string, blockType: string, expected: number): void {
    function run(test: Test): void {
        test.setBlockType(blockType, PLACE);
        pollUntilSucceed(test, () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === expected, {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
                test.assert(
                    false,
                    `${blockType} expected blockLight=${expected} at source, got ${actual} (lighting not propagated?)`,
                );
            },
        });
    }
    GameTest.register("LightingTests", testName, run).structureName("gametests:light_box").maxTicks(120);
}

// 紫水晶簇发光等级 5（lightLevel(5) 静态，facing 默认 up，下方 stone 支撑）。
registerExtraEmissionTest("light_amethyst_cluster_emits_5", "minecraft:amethyst_cluster", 5);
// 大型紫晶芽发光等级 4。
registerExtraEmissionTest("light_large_amethyst_bud_emits_4", "minecraft:large_amethyst_bud", 4);
// 中型紫晶芽发光等级 2。
registerExtraEmissionTest("light_medium_amethyst_bud_emits_2", "minecraft:medium_amethyst_bud", 2);
// 小型紫晶芽发光等级 1。
registerExtraEmissionTest("light_small_amethyst_bud_emits_1", "minecraft:small_amethyst_bud", 1);
// 末地烛发光等级 14（NetherBlocks.cpp:678 lightLevel(14) 静态，noCollision 装饰光源，无附着存活）。
registerExtraEmissionTest("light_end_rod_emits_14", "minecraft:end_rod", 14);
// 潮涌核心发光等级 15（ConduitBlock.hpp:126-133 getLightLevel override 恒返回15，无需潮涌框架激活，
// setBlockType 直接放即发光；激活态只影响 ConduitEntity 增益，与发光无关）。
registerExtraEmissionTest("light_conduit_emits_15", "minecraft:conduit", 15);
// 哭泣的黑曜石发光等级 10（NetherBlocks.cpp:218 lightLevel(10) 静态，SimpleBlock 无前置）。
registerExtraEmissionTest("light_crying_obsidian_emits_10", "minecraft:crying_obsidian", 10);
