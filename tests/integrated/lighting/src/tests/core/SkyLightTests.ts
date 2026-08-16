// 天空光测试：验证露天天空光=15、canSeeSky 语义、透明/实心方块对天空光的遮挡、树叶衰减。
//
// 设计：用 grass_pen（9×5×9 露天围栏，y=4 无封顶）+ skyAccess(true) + setupTicks(20)。
// skyAccess 清空结构上方到世界顶（Y=319）制造露天列，setupTicks 让天空光传播稳定达 15。
// grass_pen 内部空气 x,z∈[1,7], y∈[1,4]，y=4 为露天顶层。
//
// 验证点（Wiki tech_亮度.txt#天空光照：露天=15，之后按 Flood Fill 衰减传播）：
//   1. 露天空气格 skyLight=15、canSeeSky=true（天空光未遮挡列满亮）。
//   2. 玻璃(opacity=0)上方遮挡：透光，遮挡格自身及下方仍 skyLight=15、canSeeSky=true。
//   3. 石头(opacity=15)上方遮挡：阻断该列垂直天空源，遮挡格下方不再满亮。下方格由水平方向
//      露天空气列（skyLight=15）经 Flood Fill 侧传衰减 1 级得 14（非 0，因天空光如方块光般
//      在透明方块间散射传播）。canSeeSky=false（Cubium canSeeSky=skyLight>=15，14<15）。
//   4. 树叶(opacity=1)上方遮挡：阻断垂直列（opacity>0 即断），树叶格自身由水平露天邻居侧传
//      15-1=14（opacity仅衰减1级，非0），下方同样得14。canSeeSky=false（14<15）。
//   5. 阴影列（被实心方块遮挡的列）skyLight<15、canSeeSky=false。
//
// 光照重算异步：放遮挡方块后入队，pollUntilSucceed 轮询等到 skyLight 稳定。
//
// 跨服务端：skyLight/canSeeSky 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#天空光照（露天=15，Flood Fill 衰减）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getCanSeeSky } from "../utils/lightAssert.js";

// grass_pen 内部测试格：(4,3,3) 为墙内空气，正上方 (4,4,3) 为露天顶层 air。
const PROBE = { x: 4, y: 3, z: 3 };
// 遮挡格：PROBE 正上方 (4,4,3)，放方块遮挡 PROBE 的天空光。
const OCCLUDER = { x: 4, y: 4, z: 3 };

// 露天空气格天空光=15 且 canSeeSky=true。grass_pen 顶层 (4,4,3) 露天。
function openSkyIsFull(test: Test): void {
    pollUntilSucceed(
        test,
        () => getSkyLight(test, 4, 4, 3) === 15 && getCanSeeSky(test, 4, 4, 3) === true,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `open sky (4,4,3): skyLight=${getSkyLight(test, 4, 4, 3)} canSeeSky=${getCanSeeSky(
                        test,
                        4,
                        4,
                        3,
                    )} expected 15/true`,
                );
            },
        },
    );
}

// 玻璃(opacity=0)不阻挡天空光：OCCLUDER 放玻璃，玻璃格自身 skyLight=15（透光列直达），
// PROBE（正下方）仍 skyLight=15、canSeeSky=true。这是 tryPropagateSkylight 透光方块修复的
// 核心验证点：opacity==0 不阻断垂直列，遮挡格与下方均满亮（而非被错误清零）。
function glassDoesNotBlockSkyLight(test: Test): void {
    test.setBlockType("minecraft:glass", OCCLUDER);
    pollUntilSucceed(
        test,
        () => {
            return (
                // 玻璃格自身：opacity=0，垂直天空光直达，skyLight=15。
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 15 &&
                // 正下方：透光列满亮。
                getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 15 &&
                getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z) === true
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glass occluder: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} ` +
                        `canSeeSky=${getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z)} expected 15/15/true`,
                );
            },
        },
    );
}

// 石头(opacity=15)阻断该列垂直天空源：OCCLUDER 放石头。
// 遮挡格自身 skyLight=0（实心方块不透光，不入天空光增加队列）；PROBE（正下方 air）不再满亮，
// 由水平方向露天空气列（skyLight=15）经 Flood Fill 侧传衰减 1 级得 14（天空光在透明方块间散射传播，
// 见 Wiki 天空光照 Flood Fill 语义）。canSeeSky=false（Cubium canSeeSky=skyLight>=15，14<15）。
function stoneBlocksSkyLight(test: Test): void {
    test.setBlockType("minecraft:stone", OCCLUDER);
    pollUntilSucceed(
        test,
        () => {
            return (
                // 遮挡格自身被实心方块阻断，无天空光入队，skyLight=0。
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 0 &&
                // 正下方由侧方露天列散射传入，衰减 1 级得 14（非 0）。
                getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 14 &&
                getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z) === false
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `stone occluder: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} ` +
                        `canSeeSky=${getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z)} expected occluder=0 / probe=14 / see=false`,
                );
            },
        },
    );
}

// 树叶(opacity=1)阻断垂直列（opacity>0 即断）：OCCLUDER 放 oak_leaves。
// 用 minecraft:oak_leaves（具体方块 id）；minecraft:leaves 仅为标签名，非有效方块 id，
// setBlockType 解析失败会退化为 air（见 GameTestHelper::_resolveBlock），故必须用具体树种 id。
// 树叶格不入天空光增加队列（break），由 checkBlock+performLightDecrease 从水平露天邻居侧传重算：
// 邻居 15 - max(1, leaves_opacity=1) = 14，故树叶格自身 skyLight=14（非 0，opacity 仅衰减1级）。
// PROBE（正下方）同样由侧传得 14。canSeeSky=false（14<15）。
// 与 stone 测试对比：stone opacity=15 把侧传光全吃掉（15-15=0，遮挡格=0），leaves opacity=1 仅衰减1级
// （遮挡格=14）——验证 opacity 数值差异对遮挡格自身天空光的影响。
function leavesAttenuateSkyLightByOne(test: Test): void {
    test.setBlockType("minecraft:oak_leaves", OCCLUDER);
    pollUntilSucceed(
        test,
        () => {
            return (
                // 树叶格 opacity=1，从水平露天邻居侧传 15-1=14（非0，opacity仅衰减1级）。
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 14 &&
                // 正下方由侧方露天列散射传入，同样衰减 1 级得 14。
                getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 14 &&
                getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z) === false
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaves occluder: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} ` +
                        `canSeeSky=${getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z)} expected occluder=14 / probe=14 / see=false`,
                );
            },
        },
    );
}

export function registerSkyLightTests(): void {
    GameTest.register("LightingTests", "light_sky_light_open_is_15", openSkyIsFull)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_glass_does_not_block", glassDoesNotBlockSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_stone_blocks", stoneBlocksSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_leaves_attenuate_by_one", leavesAttenuateSkyLightByOne)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
}
