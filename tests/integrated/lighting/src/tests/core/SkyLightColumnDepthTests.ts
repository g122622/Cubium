// 天空光垂直列深度衰减测试：验证 opacity=0 的方块整列垂直直达15（不散射），opacity>0 阻断后下方侧传14。
//
// 核心机制（SkyLightEngine::tryPropagateSkylight，SkyLightEngine.cpp:293-386）：逐列从世界顶向下扫，
// 每格取 opacity：
//   - opacity==0（air/glass/water 等透光方块）：该格入队 skyLight=15，**继续向下传播**（垂直直达）。
//   - opacity>0（stone/leaves/ice 等遮挡方块）：break，该格及下方**不再由垂直列得15**。
// 遮挡格下方改由水平露天邻居（skyLight=15）经 performLightIncrease 侧传，衰减 max(1,opacity)：
//   - 上方遮挡 stone(opacity=15)：下方侧传 15-max(1,15)=0... 但下方空气格从水平露天空气(15)侧传衰减1=14。
//     （侧传源是水平邻居的 skyLight，非遮挡格自身的 opacity。下方空气格邻居=水平露天空气15→衰减1=14。）
//
// 关键区分（本组验证点）：
//   1. glass(opacity=0) 列：整列垂直直达15。放一层 glass，下方仍15（不散射，对齐 wiki「不包括 Glass」）。
//   2. 两层 glass 列：整列 opacity 全0，仍垂直直达15（多层透光方块不累积衰减天空光）。
//   3. stone(opacity=15) 遮挡：遮挡格下方侧传得14（非15，垂直列被阻断）。
//   4. 两层 stone 列：与单层 stone 相同，遮挡格下方仍14（下方天空光来自水平侧传，与上方遮挡层数无关——
//      只要下方空气格的水平邻居是露天空气，就得14；多层遮挡不"加深"下方，因下方光源是水平而非垂直）。
//
// 设计：grass_pen + skyAccess(true) + setupTicks(20)。多测试用不同遮挡组合。
//
// 跨服务端：skyLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#天空光照（露天=15，opacity=0 透光方块透传）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#散射（Glass 不散射天空光）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getCanSeeSky } from "../utils/lightAssert.js";

// 单层玻璃列垂直直达：OCCLUDER=(4,4,3) 放玻璃，PROBE=(4,3,3) 正下方。
// glass opacity=0 不 break → 整列垂直直达 skyLight=15。canSeeSky=true（15>=15）。
// 与 SkyLightTests.glassDoesNotBlockSkyLight 互补：此处额外断言 canSeeSky=true，并作为多层测试的基准。
function glassColumnFullSkyLight(test: Test): void {
    test.setBlockType("minecraft:glass", { x: 4, y: 4, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, 4, 4, 3) === 15 &&
                getSkyLight(test, 4, 3, 3) === 15 &&
                getCanSeeSky(test, 4, 3, 3) === true
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glass column: (4,4,3)=${getSkyLight(test, 4, 4, 3)} (4,3,3)=${getSkyLight(
                        test,
                        4,
                        3,
                        3,
                    )} canSeeSky=${getCanSeeSky(test, 4, 3, 3)} expected 15/15/true`,
                );
            },
        },
    );
}

// 两层玻璃列仍垂直直达15：(4,4,3) 和 (4,3,3) 都放玻璃，(4,2,3) 正下方探针。
// 整列 opacity 全0（glass 显式 .opacity(0)），垂直直达 → (4,2,3)=15。验证多层透光方块不累积衰减
// 天空光（glass 不散射，对齐 wiki「不包括 Glass」）。canSeeSky=true。
function twoGlassLayersStillFull(test: Test): void {
    test.setBlockType("minecraft:glass", { x: 4, y: 4, z: 3 });
    test.setBlockType("minecraft:glass", { x: 4, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, 4, 4, 3) === 15 &&
                getSkyLight(test, 4, 3, 3) === 15 &&
                getSkyLight(test, 4, 2, 3) === 15 &&
                getCanSeeSky(test, 4, 2, 3) === true
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `two glass layers: (4,4,3)=${getSkyLight(test, 4, 4, 3)} (4,3,3)=${getSkyLight(
                        test,
                        4,
                        3,
                        3,
                    )} (4,2,3)=${getSkyLight(test, 4, 2, 3)} canSeeSky=${getCanSeeSky(
                        test,
                        4,
                        2,
                        3,
                    )} expected 15/15/15/true`,
                );
            },
        },
    );
}

// 单层石头遮挡列下方侧传14：(4,4,3) 放石头，(4,3,3) 正下方。
// stone opacity=15 break → (4,3,3) 不再垂直直达，改由水平露天邻居(15)侧传衰减1=14。canSeeSky=false。
// 作为多层石头测试的基准（与 SkyLightTests.stoneBlocksSkyLight 互补，此处明确"单层遮挡"语义）。
function singleStoneOccluderSidePropagates(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 4, y: 4, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, 4, 4, 3) === 0 &&
                getSkyLight(test, 4, 3, 3) === 14 &&
                getCanSeeSky(test, 4, 3, 3) === false
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `single stone: (4,4,3)=${getSkyLight(test, 4, 4, 3)} (4,3,3)=${getSkyLight(
                        test,
                        4,
                        3,
                        3,
                    )} canSeeSky=${getCanSeeSky(test, 4, 3, 3)} expected 0/14/false`,
                );
            },
        },
    );
}

// 两层石头遮挡列下方仍14（不加深）：(4,4,3) 和 (4,3,3) 都放石头，(4,2,3) 正下方。
// (4,2,3) 的天空光来自水平邻居（y=2 露天空气15）侧传衰减1=14，与上方遮挡层数无关。
// 验证"下方天空光来自水平侧传而非垂直"——多层遮挡不使下方更暗（下方光源是水平露天邻居）。
// canSeeSky=false（14<15）。
function twoStoneLayersDoNotDeepen(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 4, y: 4, z: 3 });
    test.setBlockType("minecraft:stone", { x: 4, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, 4, 3, 3) === 0 &&
                getSkyLight(test, 4, 2, 3) === 14 &&
                getCanSeeSky(test, 4, 2, 3) === false
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `two stone layers: (4,3,3)=${getSkyLight(test, 4, 3, 3)} (4,2,3)=${getSkyLight(
                        test,
                        4,
                        2,
                        3,
                    )} canSeeSky=${getCanSeeSky(test, 4, 2, 3)} expected 0/14/false (side-propagated, not deepened)`,
                );
            },
        },
    );
}

export function registerSkyLightColumnDepthTests(): void {
    GameTest.register("LightingTests", "light_sky_light_glass_column_full", glassColumnFullSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_two_glass_layers_full", twoGlassLayersStillFull)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_single_stone_side_propagates", singleStoneOccluderSidePropagates)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_two_stone_layers_not_deepened", twoStoneLayersDoNotDeepen)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
}
