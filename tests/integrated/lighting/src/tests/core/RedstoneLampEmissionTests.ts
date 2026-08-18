// 红石灯发光等级测试：验证红石灯被红石块充能点亮后发光15（对齐 wiki 发光方块表）。
//
// wiki 发光方块表（tech_亮度.txt#发光方块，行211-212）：点亮的红石灯光照等级 15（未点亮 0）。
// Cubium 实现（RedstoneLampBlock.hpp:70-76）：getLightLevel override 返回 `isLit(state) ? 15 : 0`，
// 发光完全由 LIT state 驱动。LIT 切换由红石信号控制（RedstoneLampBlock.cpp:76-127）：
//   - onBlockAdded/neighborChanged：isPowered(红石块等电源) ≠ isLit → 上升沿立即 lit=true，下降沿调度4tick后灭。
//   - 故 setBlockWithStates lit=true 在无电源时会被 onBlockAdded 调度4tick后熄灭（不稳定，不可直接写 lit=true 测）。
//   红石灯点亮态必须经真实红石电源充能，与 copper_bulb（边沿切换，无信号保持当前 lit）不同。
//
// 点亮方案：先放红石灯(3,1,3)（默认 lit=false 未亮），再放相邻红石块(4,1,3)（水平相邻，全向供电15）。
// 放红石块 flags=3 → 邻居红石灯 neighborChanged → isPowered(红石块 weakPower 15)>0=true ≠ isLit(false)
// → 立即 setBlockState(lit=true) 同步点亮。该链路已由 block_behavior/RedstoneLampTests 验证（lit state 切换），
// 本组补发光等级断言：点亮后 (3,1,3) blockLight=15（getLightLevel `lit?15:0` 返回15）。
//
// 未点亮态：默认 lit=false，红石灯自身格 blockLight=0（getLightLevel 返回0）。但 light_box 内 (3,1,3)
// 放红石灯后不接电源，onBlockAdded 检测 isPowered=false == isLit(false)，不调度熄灭，lit 保持 false，
// blockLight=0 稳定可测。本组同时测未点亮=0 与点亮=15，验证 LIT state 驱动发光切换。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光）。
// (3,1,3) 红石灯 + (4,1,3) 红石块（点亮态）；(3,1,3) 单独红石灯（未点亮态）。
// light_box 封顶保证 skyLight=0，blockLight 是唯一光源，红石灯自身格 blockLight 即其发光等级。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。lit state 名两端一致但发光等级断言仅 Cubium。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#发光方块（点亮的红石灯 15，行211-212）
// Ref: tests/integrated/block_behavior/src/tests/redstone/RedstoneLampTests.ts（lit state 切换链路，本组补发光等级）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

const LAMP = { x: 3, y: 1, z: 3 };
const POWER = { x: 4, y: 1, z: 3 };

// 红石灯被红石块充能点亮后发光15：先放红石灯(3,1,3)（默认未亮），再放相邻红石块(4,1,3) 充能，
// 红石灯 neighborChanged 同步点亮 lit=true，该格 blockLight=15（getLightLevel `lit?15:0`）。
// 验证红石灯点亮态发光等级与 wiki 一致。补 block_behavior RedstoneLampTests 只测 lit state 未测发光等级的缺口。
function litRedstoneLampEmitsFifteen(test: Test): void {
    test.setBlockType("minecraft:redstone_lamp", LAMP);
    test.setBlockType("minecraft:redstone_block", POWER);
    pollUntilSucceed(
        test,
        () => getBlockLight(test, LAMP.x, LAMP.y, LAMP.z) === 15,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, LAMP.x, LAMP.y, LAMP.z);
                test.assert(
                    false,
                    `lit redstone_lamp: expected blockLight=15 at source, got ${actual} ` +
                        `(redstone_block may not have powered the lamp, or lit not toggled?)`,
                );
            },
        },
    );
}

// 未点亮红石灯发光0：单独放红石灯(3,1,3)（默认 lit=false，无电源），onBlockAdded 检测 isPowered=false==isLit(false)
// 不调度熄灭，lit 保持 false，blockLight=0（getLightLevel `lit?15:0` 返回0）。
// 验证红石灯未点亮态不发光，与点亮态测试对照确认 LIT state 驱动发光切换。
function unlitRedstoneLampEmitsZero(test: Test): void {
    test.setBlockType("minecraft:redstone_lamp", LAMP);
    pollUntilSucceed(
        test,
        () => getBlockLight(test, LAMP.x, LAMP.y, LAMP.z) === 0,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                const actual = getBlockLight(test, LAMP.x, LAMP.y, LAMP.z);
                test.assert(
                    false,
                    `unlit redstone_lamp: expected blockLight=0 at source, got ${actual} ` +
                        `(default lit=false should emit 0; got nonzero means LIT default or override wrong?)`,
                );
            },
        },
    );
}

export function registerRedstoneLampEmissionTests(): void {
    GameTest.register("LightingTests", "light_lit_redstone_lamp_emits_15", litRedstoneLampEmitsFifteen)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_unlit_redstone_lamp_emits_0", unlitRedstoneLampEmitsZero)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
