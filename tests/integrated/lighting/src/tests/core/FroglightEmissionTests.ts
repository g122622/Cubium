// 蛙明灯发光等级测试：验证 ochre/pearlescent/verdant froglight 静态发光 15（对齐 wiki 发光方块表）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块）：蛙明灯（3 种变体）光照等级 15。
// Cubium 实现（WildBlocks.cpp:51-67）：三种蛙明灯注册为 RotatedPillarBlock，注册处 .lightLevel(15) 静态值，
// 无 getLightLevel override，发光等级恒 15，与 axis state 无关。RotatedPillarBlock 无 onBlockAdded/
// neighborChanged/updatePostPlacement/isValidPosition override（继承基类，isValidPosition 恒 true），
// 任意放置存活，无自毁风险。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// 在 PLACE (3,1,3) 放蛙明灯，断言该格 blockLight=15。light_box 封顶保证 skyLight=0，blockLight 是唯一光源，
// 光源格 blockLight 即其发光等级。
//
// 现有 BlockLightEmissionTests 已覆盖 beacon/glowstone/jack_o_lantern/sea_lantern/lantern/shroomlight/lava
// 等 15 级光源，但未覆盖 1.19+ 新增的蛙明灯。本组补全蛙明灯 3 变体（颜色不影响发光，测 3 变体验证注册一致性）。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（蛙明灯 15）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

/**
 * 通用发光等级测试：在 PLACE 放置发光方块，轮询断言该格 blockLight 等于 expected。
 * @param testName 注册测试名
 * @param blockType 方块 typeId
 * @param expected 期望发光等级 (0-15)
 */
function registerFroglightEmissionTest(testName: string, blockType: string, expected: number): void {
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

// 赭黄蛙明灯发光15（lightLevel(15) 静态，axis 默认 y，下方 stone 无关存活）。
registerFroglightEmissionTest("light_ochre_froglight_emits_15", "minecraft:ochre_froglight", 15);
// 珠光蛙明灯发光15。
registerFroglightEmissionTest("light_pearlescent_froglight_emits_15", "minecraft:pearlescent_froglight", 15);
// 翠绿蛙明灯发光15。
registerFroglightEmissionTest("light_verdant_froglight_emits_15", "minecraft:verdant_froglight", 15);
