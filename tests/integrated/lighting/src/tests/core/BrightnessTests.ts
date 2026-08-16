// 综合亮度（brightness）测试：验证 IWorld::getLight 的 max(blockLight, skyLight-skyDarkening) 语义。
//
// Cubium Block.brightness 绑定 IWorld::getLight(pos)（IWorld.hpp:627-630）：
//   getLight = getNeighborAwareLightSubtracted(pos, getSkyDarkening())
//            = getLightSubtracted(pos, skyDarkening)
//            = max(blockLight, skyLight>skyDarkening ? skyLight-skyDarkening : 0)
// GameTest 默认晴天白天，getSkyDarkening()=0（calculateSkyDarkening 白天晴天返回 0），
// 故 brightness = max(blockLight, skyLight)。
//
// 验证点（区分"brightness=blockLight"与"brightness=max"两种实现错误）：
//   1. light_box 内（skyLight=0）放荧石邻格：brightness=blockLight=14（无天空光，取方块光）。
//   2. grass_pen 露天空气格：brightness=15（skyLight=15 主导，方块光 0）。
//   3. grass_pen 露天处放火把(14)：blockLight=14 < skyLight=15，brightness=max=15（验证取 max 而非 blockLight）。
//   4. light_box 内荧石自身格：brightness=15（光源照亮自身，blockLight=15，skyLight=0，max=15）。
//
// 测试 3 是核心：若 brightness 错误实现为"=blockLight"会得 14（FAIL），正确 max 实现得 15（PASS）。
//
// 跨服务端：brightness 是 Cubium 专有，基岩端 one-sided。
//
// Ref: src\common\world\IWorld.hpp:585-596 getLightSubtracted（max 语义）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight, getSkyLight, getBrightness } from "../utils/lightAssert.js";

// light_box 内荧石邻格 brightness=blockLight=14（skyLight=0，max(14,0)=14）。
// 验证无天空光时 brightness 取方块光，不被不存在的天空光干扰。
function brightnessInDarkBoxEqualsBlockLight(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            // 邻格 (4,3,3)：blockLight=14, skyLight=0, brightness=max(14,0)=14。
            return (
                getBlockLight(test, 4, 3, 3) === 14 &&
                getSkyLight(test, 4, 3, 3) === 0 &&
                getBrightness(test, 4, 3, 3) === 14
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `dark box neighbor: blockLight=${getBlockLight(test, 4, 3, 3)} skyLight=${getSkyLight(
                        test,
                        4,
                        3,
                        3,
                    )} brightness=${getBrightness(test, 4, 3, 3)} expected 14/0/14`,
                );
            },
        },
    );
}

// grass_pen 露天空气格 brightness=15（skyLight=15 主导，晴天白天 skyDarkening=0）。
// 验证有天空光且无方块光时 brightness 取天空光。
function brightnessOpenSkyIs15(test: Test): void {
    pollUntilSucceed(
        test,
        () => getBrightness(test, 4, 4, 3) === 15,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `open sky (4,4,3): brightness=${getBrightness(test, 4, 4, 3)} expected 15 (skyLight=15, day clear)`,
                );
            },
        },
    );
}

// 露天火把格 brightness 取 max：grass_pen (4,3,3) 放火把(14)，该格 blockLight=14 < skyLight=15，
// brightness=max(14,15)=15。验证 getLight 取 max 而非直接返回 blockLight（区分两种实现错误）。
function brightnessTakesMaxOfBlockAndSky(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 4, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            // 火把自身格 (4,3,3)：blockLight=14（火把发光等级），skyLight=15（露天列），brightness=max=15。
            return (
                getBlockLight(test, 4, 3, 3) === 14 &&
                getSkyLight(test, 4, 3, 3) === 15 &&
                getBrightness(test, 4, 3, 3) === 15
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `torch open sky: blockLight=${getBlockLight(test, 4, 3, 3)} skyLight=${getSkyLight(
                        test,
                        4,
                        3,
                        3,
                    )} brightness=${getBrightness(test, 4, 3, 3)} expected 14/15/15 (max)`,
                );
            },
        },
    );
}

// light_box 内荧石自身格 brightness=15（光源照亮自身 blockLight=15, skyLight=0, max=15）。
// 验证光源格自身亮度正确（blockLight=发光等级，brightness 同）。
function brightnessSourceCellIs15(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => getBrightness(test, 3, 3, 3) === 15,
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glowstone source cell: brightness=${getBrightness(test, 3, 3, 3)} expected 15`,
                );
            },
        },
    );
}

export function registerBrightnessTests(): void {
    GameTest.register("LightingTests", "light_brightness_dark_box_equals_block_light", brightnessInDarkBoxEqualsBlockLight)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_brightness_open_sky_is_15", brightnessOpenSkyIs15)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_brightness_takes_max_of_block_and_sky", brightnessTakesMaxOfBlockAndSky)
        .structureName("gametests:grass_pen")
        .skyAccess(true)
        .setupTicks(20)
        .maxTicks(150);
    GameTest.register("LightingTests", "light_brightness_source_cell_is_15", brightnessSourceCellIs15)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
