// 方块光发光等级测试：验证各发光方块发出的方块光等级与原版 Wiki 一致。
//
// 设计：在 light_box（7×7×7 封顶实心盒子，隔绝天空光）内部地板 (3,1,3) 放置发光方块，
// 等待方块光传播稳定后，断言该方块所在格的 blockLight 等于其发光等级（光源照亮自身格）。
// light_box 封顶实心保证 skyLight=0（无 skyAccess），blockLight 是唯一光源，不受天空光干扰。
//
// 发光等级来源（Wiki tech_亮度.txt 发光方块表，Cubium 核查实现一致）：
//   15: 荧石/南瓜灯/海晶灯/熔岩/灯笼/菌光体/蛙明灯/信标
//   14: 火把/末地烛
//   10: 灵魂火把/灵魂灯笼/哭泣黑曜石/岩浆块(实为3,见下)
//   7:  末影箱/附魔台
//   3:  岩浆块
//   1:  棕色蘑菇/龙蛋
//
// 红石灯(lit=15/unlit=0)、红石火把(lit=7/unlit=0) 为动态发光，依赖红石信号触发 LIT 状态切换，
// 其发光等级随状态动态变化（RedstoneLampBlock/RedstoneTorchBlock 已 override getLightLevel）。
// TODO: 动态发光测试（放红石灯 + 拉杆触发点亮/熄灭，断言 blockLight 在 15/0 间切换）待后续补充，
//       需构造红石信号链，时序与红石更新耦合，单独成 BlockLightDynamicTests。
//
// 光照重算时序：setBlock 后光照变更入队 m_lightQueue，需若干世界 tick 由 ServerWorld::tick
// 批量传播。用 pollUntilSucceed 轮询直到 blockLight 达预期值（避免首 tick 光照未稳定）。
//
// 跨服务端：blockLight 是 Cubium 专有扩展，基岩 BDS 的 Block 无此属性（读得 undefined→-1）。
// 故本组测试仅在 Cubium 端验证光照引擎正确性，基岩端对比归类为 one-sided（仅 Cubium 跑）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// light_box 内部地板中心格（y=0 石头地板之上，有支撑可放置需附着的方块）。
const PLACE = { x: 3, y: 1, z: 3 };

/**
 * 通用发光等级测试：在 PLACE 放置发光方块，轮询断言该格 blockLight 等于 expected。
 * @param testName 注册测试名
 * @param blockType 方块 typeId（如 "minecraft:glowstone"）
 * @param expected 期望发光等级 (0-15)
 */
function registerEmissionTest(testName: string, blockType: string, expected: number): void {
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

// 等级 15 的发光方块
registerEmissionTest("light_glowstone_emits_15", "minecraft:glowstone", 15);
registerEmissionTest("light_jack_o_lantern_emits_15", "minecraft:jack_o_lantern", 15);
registerEmissionTest("light_sea_lantern_emits_15", "minecraft:sea_lantern", 15);
registerEmissionTest("light_lantern_emits_15", "minecraft:lantern", 15);
registerEmissionTest("light_shroomlight_emits_15", "minecraft:shroomlight", 15);
registerEmissionTest("light_beacon_emits_15", "minecraft:beacon", 15);

// 等级 14 的发光方块
registerEmissionTest("light_torch_emits_14", "minecraft:torch", 14);
registerEmissionTest("light_end_rod_emits_14", "minecraft:end_rod", 14);

// 等级 10 的发光方块
registerEmissionTest("light_soul_torch_emits_10", "minecraft:soul_torch", 10);
registerEmissionTest("light_soul_lantern_emits_10", "minecraft:soul_lantern", 10);
registerEmissionTest("light_crying_obsidian_emits_10", "minecraft:crying_obsidian", 10);

// 等级 7 的发光方块
registerEmissionTest("light_ender_chest_emits_7", "minecraft:ender_chest", 7);
registerEmissionTest("light_enchanting_table_emits_7", "minecraft:enchanting_table", 7);

// 等级 3 的发光方块
registerEmissionTest("light_magma_block_emits_3", "minecraft:magma_block", 3);

// 等级 1 的发光方块
registerEmissionTest("light_brown_mushroom_emits_1", "minecraft:brown_mushroom", 1);
registerEmissionTest("light_dragon_egg_emits_1", "minecraft:dragon_egg", 1);

// 熔岩发光等级 15
registerEmissionTest("light_lava_emits_15", "minecraft:lava", 15);
