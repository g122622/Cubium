// 铜灯发光等级测试：验证 copper_bulb 点亮态发光15、未点亮态发光0（对齐 wiki 发光方块表，1.21 新增方块）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块，行235-236）：点亮的铜灯光照等级 15（未点亮 0）。
// Cubium 实现（CopperBulbBlock.cpp/hpp）：注册处 copperBulbProps 未设静态 lightLevel（默认0），
// CopperBulbBlock.hpp:82-88 getLightLevel override 返回 `state.get(LIT) ? 15 : 0`，发光完全由 LIT state 驱动。
// LIT 切换仅在 updatePostPlacement 红石上升沿 toggle（cpp:64-87），本测试不经 setBlockWithStates 直接写 LIT state
// 绕过红石（与 nether_portal 同理：flags=3 不调新方块自身 updatePostPlacement，lit 保持写入值）。
//
// 已知偏差（不测）：氧化变体 exposed/weathered/oxidized_copper_bulb 共用同一 getLightLevel override 恒 `lit?15:0`，
// 不分氧化度（wiki 点亮态 15/12/8/4），偏差，跳过氧化变体 12/8/4。本组只测基础 copper_bulb 点亮=15/未点亮=0（与 wiki 一致）。
//
// 点亮态放置安全性：setBlockWithStates("minecraft:copper_bulb", PLACE, "lit=true") 经 ServerWorld::setBlockState(flags=3)
// 写入 lit=true state。flags=3 不调新方块自身 updatePostPlacement（CopperBulbBlock 的红石边沿逻辑不触发），
// 故 lit=true 保持。light_box 内无红石源，后续不会触发 updatePostPlacement 重算红石。CopperBulbBlock 无 onBlockAdded
// override（基类空操作），放置即发光。与 redstone_lamp 不同（lamp onBlockAdded/neighborChanged 按红石重算 lit 会自灭），
// 铜灯只在红石信号边沿切换 LIT，无信号时保持当前 lit，故 setBlockWithStates lit=true 稳定。
//
// 未点亮态：setBlockType 默认 lit=false（CopperBulbBlock.cpp:60-61 默认 state LIT=false），blockLight=0。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// PLACE (3,1,3) 分别放点亮/未点亮铜灯，断言该格 blockLight=15/0。
// light_box 封顶保证 skyLight=0，blockLight 是唯一光源，铜灯自身格 blockLight 即其发光等级。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（点亮的铜灯 15，行235-236）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const PLACE = { x: 3, y: 1, z: 3 };

// 点亮铜灯发光15：setBlockWithStates 写 lit=true（flags=3 不触发红石边沿逻辑，lit 保持），该格 blockLight=15。
// 验证 copper_bulb 点亮态发光等级与 wiki 一致（getLightLevel override `lit?15:0`）。
function litCopperBulbEmitsFifteen(test: Test): void {
    test.setBlockWithStates("minecraft:copper_bulb", PLACE, "lit=true");
    pollUntilSucceed(
        test,
        () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 15,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
                test.assert(
                    false,
                    `lit copper_bulb: expected blockLight=15 at source, got ${actual} ` +
                        `(lit=true may have been reset by updatePostPlacement redstone re-eval?)`,
                );
            },
        },
    );
}

// 未点亮铜灯发光0：setBlockType 默认 lit=false，该格 blockLight=0（getLightLevel override `lit?15:0` 返回0）。
// 验证 copper_bulb 未点亮态不发光，与 wiki 一致。与点亮态测试对照，确认 LIT state 驱动发光切换。
function unlitCopperBulbEmitsZero(test: Test): void {
    test.setBlockType("minecraft:copper_bulb", PLACE);
    pollUntilSucceed(
        test,
        () => getBlockLight(test, PLACE.x, PLACE.y, PLACE.z) === 0,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, PLACE.x, PLACE.y, PLACE.z);
                test.assert(
                    false,
                    `unlit copper_bulb: expected blockLight=0 at source, got ${actual} ` +
                        `(default lit=false should emit 0; got nonzero means LIT default or override wrong?)`,
                );
            },
        },
    );
}

export function registerCopperBulbEmissionTests(): void {
    GameTest.register("LightingTests", "light_lit_copper_bulb_emits_15", litCopperBulbEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_unlit_copper_bulb_emits_0", unlitCopperBulbEmitsZero)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
