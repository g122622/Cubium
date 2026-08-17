// 部分光照透明方块的天空光遮挡测试：验证半砖/楼梯（useShapeForLightOcclusion 方块）阻断垂直天空光。
//
// wiki「部分光照透明」（tech_亮度.txt#部分光照透明）：遮挡形状不完整但存在完整遮挡面的方块（半砖、
// 楼梯、雪层等），光线只从完整遮挡面射入/射出时被阻挡。
//
// Cubium 实际阻断机制（核查 SkyLightEngine.cpp:316-351 列传播，非 wiki 描述的形状遮挡）：
//   列传播每格 current 依次检查：① above 的 Down 面形状遮挡 ② current 的 Up 面形状遮挡 ③ current opacity。
//   - bottom slab（box(0,0,0,1,0.5,1)）：Up 面投影为空（maxY=0.5 不触及 Up 面，getFaceShape 返回空），
//     故形状遮挡分支 faceShapeOccludes(empty,空)=false 不 break（行 339）；随后 opacity=15>0 break（行 348）。
//   - double slab：useShapeForLightOcclusion=false → isConditionallyFullOpaque=false → 不进形状遮挡分支，
//     纯 opacity=15 break（行 348）。
//   - straight 下半楼梯：getShape=SLAB_BOTTOM(box 0,0,0,1,0.5,1) 同 bottom slab，Up 面空，opacity=15 break。
//   故三测试的阻断机制均为 opacity=15 break，形状遮挡分支未独立触发。
//
// Cubium 与 vanilla 语义差异（不为该差异另写测试，此处仅标注）：
//   vanilla bottom slab getLightBlock(isSolidRender)=0，靠形状遮挡（Down 面完整）阻断天空光，opacity 不参与；
//   Cubium Block::getOpacity 用 isOpaque(material)（ROCK/WOOD opaque→15）而非 vanilla isSolidRender，
//   致 slab/stairs opacity=15 抢先 break，形状遮挡分支冗余。此差异是 BuildingBlocks 等 TODO(光照) 待修项。
//
// 数值断言（与 Cubium 实际行为一致）：遮挡格自身 skyLight=0（opacity break 不入队），正下方 PROBE 由水平
// 露天邻居侧传衰减1 得 14，canSeeSky=false（14<15）。断言值正确，仅上方机制注释此前误标为「形状遮挡」。
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

// 单层下半砖阻断天空光：OCCLUDER 放 stone_brick_slab half=bottom（单层下半，box(0,0,0,1,0.5,1)）。
// useShapeForLightOcclusion=true（单层），opacity=15（ROCK 材质 isOpaque）→ isConditionallyFullOpaque=true。
// 列传播：Up 面投影为空（maxY=0.5 不触及 Up 面）→ 形状遮挡分支不 break；opacity=15>0 break（行 348）。
// 遮挡格自身 skyLight=0（break 不入队），正下方侧传得 14。验证单层半砖阻断垂直天空光（机制为 opacity break，
// 非形状遮挡——Cubium 与 vanilla 语义差异，详见文件头）。
function bottomSlabTopFaceBlocksSkyLight(test: Test): void {
    test.setBlockWithStates("minecraft:stone_brick_slab", OCCLUDER, "type=bottom");
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
                    `bottom slab: OCCLUDER skyLight=${getSkyLight(test, OCCLUDER.x, OCCLUDER.y, OCCLUDER.z)} ` +
                        `PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 0/14`,
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

// 直线下半楼梯阻断天空光：OCCLUDER 放 oak_stairs half=bottom shape=straight。
// useShapeForLightOcclusion=true（StairsBlock.hpp:101），opacity=15（WOOD 材质 isOpaque）→ isConditionallyFullOpaque=true。
// straight 下半 getShape=SLAB_BOTTOM（box(0,0,0,1,0.5,1)，无角），Up 面投影为空（maxY=0.5 不触及 Up 面）
// → 形状遮挡分支不 break；opacity=15>0 break（行 348）。遮挡格自身=0，正下方侧传得 14。
// 与 bottom slab 同机制（opacity break，非形状遮挡）。Cubium straight 楼梯形状简化为纯下半台阶（无角）。
function straightBottomStairsBlockSkyLight(test: Test): void {
    test.setBlockWithStates("minecraft:oak_stairs", OCCLUDER, "half=bottom,shape=straight");
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
                    `straight bottom stairs: OCCLUDER skyLight=${getSkyLight(
                        test,
                        OCCLUDER.x,
                        OCCLUDER.y,
                        OCCLUDER.z,
                    )} PROBE skyLight=${getSkyLight(test, PROBE.x, PROBE.y, PROBE.z)} expected 0/14`,
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
