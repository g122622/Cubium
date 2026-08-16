// 天空光测试：验证露天天空光=15、canSeeSky 语义、透明/实心方块对天空光的遮挡、树叶衰减。
//
// 设计：用 grass_pen（9×5×9 露天围栏，y=4 无封顶）+ skyAccess(true) + setupTicks(20)。
// skyAccess 清空结构上方到世界顶（Y=319）制造露天列，setupTicks 让天空光传播稳定达 15。
// grass_pen 内部空气 x,z∈[1,7], y∈[1,4]，y=4 为露天顶层。
//
// 验证点（Wiki tech_亮度.txt + Cubium 走 Java 散射机制）：
//   1. 露天空气格 skyLight=15、canSeeSky=true（天空光未遮挡列满亮）。
//   2. 玻璃(opacity=0)上方遮挡：透光，下方仍 skyLight=15、canSeeSky=true。
//   3. 石头(opacity=15)上方遮挡：完全挡光，下方 skyLight=0、canSeeSky=false。
//   4. 树叶(opacity=1)上方遮挡：Java 散射每格减1，下方 skyLight=14。
//   5. 阴影列（被实心方块遮挡的列）skyLight<15、canSeeSky=false。
//
// 光照重算异步：放遮挡方块后入队，pollUntilSucceed 轮询等到 skyLight 稳定。
//
// 跨服务端：skyLight/canSeeSky 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#天空光照（露天=15，遮挡衰减）

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

// 玻璃(opacity=0)不阻挡天空光：OCCLUDER 放玻璃，PROBE 仍 skyLight=15、canSeeSky=true。
function glassDoesNotBlockSkyLight(test: Test): void {
    test.setBlockType("minecraft:glass", OCCLUDER);
    pollUntilSucceed(
        test,
        () => getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 15 && getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z) === true,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glass occluder: PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} canSeeSky=${getCanSeeSky(
                        test,
                        PROBE.x,
                        PROBE.y,
                        PROBE.z,
                    )} expected 15/true (glass is transparent)`,
                );
            },
        },
    );
}

// 石头(opacity=15)完全阻挡天空光：OCCLUDER 放石头，PROBE skyLight=0、canSeeSky=false。
function stoneBlocksSkyLight(test: Test): void {
    test.setBlockType("minecraft:stone", OCCLUDER);
    pollUntilSucceed(
        test,
        () => getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 0 && getCanSeeSky(test, PROBE.x, PROBE.y, PROBE.z) === false,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `stone occluder: PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} canSeeSky=${getCanSeeSky(
                        test,
                        PROBE.x,
                        PROBE.y,
                        PROBE.z,
                    )} expected 0/false (stone is opaque)`,
                );
            },
        },
    );
}

// 树叶(opacity=1)按 Java 散射衰减1级：OCCLUDER 放树叶，PROBE skyLight=14、canSeeSky=false。
// canSeeSky 要求 skyLight>=15，树叶衰减后 14<15 故 canSeeSky=false（阴影下不算露天）。
function leavesAttenuateSkyLightByOne(test: Test): void {
    test.setBlockType("minecraft:leaves", OCCLUDER);
    pollUntilSucceed(
        test,
        () => getSkyLight(test, PROBE.x, PROBE.y, PROBE.z) === 14,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaves occluder: PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 14 (leaves opacity=1)`,
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
