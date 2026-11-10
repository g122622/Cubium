// 部分光照透明方块的天空光遮挡测试：验证半砖/楼梯（useShapeForLightOcclusion 方块）阻断垂直天空光。
//
// wiki「部分光照透明」（tech_亮度.txt#部分光照透明）：遮挡形状不完整但存在完整遮挡面的方块（半砖、
// 楼梯、雪层等），光线只从完整遮挡面射入/射出时被阻挡。
//
// Cubium 阻断机制（SkyLightEngine::tryPropagateSkylight 列传播）：
//   列传播每格 current 依次检查：① above 的 Down 面形状遮挡 ② current 的 Up 面形状遮挡 ③ current opacity。
//   - bottom slab / straight 下半楼梯（box(0,0,0,1,0.5,1)）：isSolidRender=false → opacity=0，
//     列传播不在遮挡格 break——遮挡格自身仍由上方垂直列得 15。但走到下一格时 above=slab，
//     其 Down 面完整（faceShapeOccludes(empty, downFace)=true）→ 列在此终止，
//     正下方改由水平露天邻居侧传衰减 1 得 14。
//   - double slab：isSolidRender=true → opacity=15 break，遮挡格自身不入队得 0，正下方侧传得 14。
//
// 数值断言：bottom slab / straight 楼梯 的遮挡格自身 skyLight=15、正下方 PROBE=14、
// canSeeSky(PROBE)=false（Cubium canSeeSky=skyLight>=15）；double slab 遮挡格自身=0、PROBE=14。
//
// 设计：grass_pen + skyAccess(true) + setupTicks(20)。OCCLUDER=(4,4,3) 放部分透明方块，PROBE=(4,3,3) 正下方。
//
// 跨服务端：skyLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#部分光照透明（半砖/楼梯遮挡面）
// Ref: SkyLightEngine.cpp:316-351（列传播：形状遮挡判定在 opacity break 之前，但 Up 面投影空致不触发）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getSkyLight, getCanSeeSky } from "../utils/lightAssert.js";

const OCCLUDER = { x: 4, y: 4, z: 3 };
const PROBE = { x: 4, y: 3, z: 3 };

// 单层下半砖阻断向下天空光：OCCLUDER 放 stone_brick_slab half=bottom（单层下半，box(0,0,0,1,0.5,1)）。
// isSolidRender=false（形状非完整立方体）→ opacity=0，列传播不在遮挡格 break：
// 光照从上方经不完整的 Up 面进入，遮挡格自身 skyLight=15；
// 但遮挡格 Down 面完整，垂直列在遮挡格下方终止，PROBE 由水平露天邻居侧传得 14（canSeeSky=false）。
function bottomSlabTopFaceBlocksSkyLight(test: Test): void {
    test.setBlockWithStates("minecraft:stone_brick_slab", OCCLUDER, "type=bottom");
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 15 &&
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
                    `bottom slab: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 15/14`,
                );
            },
        },
    );
}

// 双层半砖（完整方块）阻断天空光：OCCLUDER 放 stone_brick_slab type=double（双层=完整方块）。
// useShapeForLightOcclusion=false（Double 类型，SlabBlock.hpp:108）→ isConditionallyFullOpaque=false
// → 不进形状遮挡分支，纯 opacity=15 break（行 348）。遮挡格自身=0，正下方侧传得 14。
// 验证双层半砖等同完整实心方块阻断天空光（双层本就是完整方块，opacity=15 阻断符合 vanilla）。
function doubleSlabBlocksSkyLight(test: Test): void {
    test.setBlockWithStates("minecraft:stone_brick_slab", OCCLUDER, "type=double");
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 0 &&
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
                    `double slab: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 0/14`,
                );
            },
        },
    );
}

// 直线下半楼梯阻断向下天空光：OCCLUDER 放 oak_stairs half=bottom shape=straight。
// isSolidRender=false（形状非完整立方体）→ opacity=0，列传播不在遮挡格 break：
// 遮挡格自身 skyLight=15；遮挡格 Down 面完整，列在其下方终止，PROBE 由水平侧传得 14。
// 与 bottom slab 同机制。Cubium straight 楼梯形状简化为纯下半台阶（无角）。
function straightBottomStairsBlockSkyLight(test: Test): void {
    test.setBlockWithStates("minecraft:oak_stairs", OCCLUDER, "half=bottom,shape=straight");
    pollUntilSucceed(
        test,
        () => {
            return (
                getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z) === 15 &&
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
                    `straight bottom stairs: OCCLUDER skyLight=${getSkyLight(
                        test,
                        OCCLUDER.x,
                        OCCLUDER.y,
                        OCCLUDER.z,
                    )} PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 15/14`,
                );
            },
        },
    );
}

export function registerShapeOcclusionSkyLightTests(): void {
    GameTest.register("LightingTests", "light_sky_light_bottom_slab_top_face_blocks", bottomSlabTopFaceBlocksSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_sky_light_double_slab_blocks", doubleSlabBlocksSkyLight)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register(
        "LightingTests",
        "light_sky_light_straight_bottom_stairs_block",
        straightBottomStairsBlockSkyLight,
    )
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
}
