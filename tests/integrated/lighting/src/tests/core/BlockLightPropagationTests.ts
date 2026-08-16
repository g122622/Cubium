// 方块光传播衰减测试：验证方块光按曼哈顿距离每格衰减 1（Java 散射机制）。
//
// Cubium 走 Java 版 StarLight 散射机制（非基岩版半透明固定衰减表）。BlockStarLightEngine
// 从光源 flood-fill 传播，每穿一格减 max(1, opacity)，空气 opacity=0 故最小减 1。光源(15)
// 沿单轴直线：邻格 14、再远 13、…、距离 d 处 15-d，到 0 截止。
//
// 设计：在 light_box 内部中心 (3,3,3) 放荧石(15)，验证沿 x/z/y 轴各距离的 blockLight。
// light_box 内部 5×5×5（x,z∈[1,5], y∈[1,5]），中心到任一墙距离 2 格，足够验证 15→13 衰减。
// 多光源叠加测试验证 max 语义（两光源中点取较亮者）。
//
// 光照重算异步：setBlock 后入队 m_lightQueue，pollUntilSucceed 轮询等到所有目标格达预期。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#方块光照（每格衰减1，曼哈顿距离）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// 荧石(15) 沿 x 轴直线衰减：中心 (3,3,3) 发光，(4,3,3)=14、(5,3,3)=13。
// (5,3,3) 距墙1格仍在空气内，未受墙遮挡影响（墙是实心 opacity=15，但衰减只看传播路径上的方块）。
function glowstoneDecaysAlongX(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getBlockLight(test, 3, 3, 3) === 15 &&
                getBlockLight(test, 4, 3, 3) === 14 &&
                getBlockLight(test, 5, 3, 3) === 13
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glowstone decay x: (3,3,3)=${getBlockLight(test, 3, 3, 3)} (4,3,3)=${getBlockLight(
                        test,
                        4,
                        3,
                        3,
                    )} (5,3,3)=${getBlockLight(test, 5, 3, 3)} expected 15/14/13`,
                );
            },
        },
    );
}

// 荧石(15) 沿 z 轴直线衰减：(3,3,4)=14、(3,3,5)=13。
function glowstoneDecaysAlongZ(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getBlockLight(test, 3, 3, 3) === 15 &&
                getBlockLight(test, 3, 3, 4) === 14 &&
                getBlockLight(test, 3, 3, 5) === 13
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glowstone decay z: (3,3,4)=${getBlockLight(test, 3, 3, 4)} (3,3,5)=${getBlockLight(
                        test,
                        3,
                        3,
                        5,
                    )} expected 14/13`,
                );
            },
        },
    );
}

// 荧石(15) 沿 y 轴直线衰减：(3,4,3)=14、(3,5,3)=13。
function glowstoneDecaysAlongY(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                getBlockLight(test, 3, 3, 3) === 15 &&
                getBlockLight(test, 3, 4, 3) === 14 &&
                getBlockLight(test, 3, 5, 3) === 13
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glowstone decay y: (3,4,3)=${getBlockLight(test, 3, 4, 3)} (3,5,3)=${getBlockLight(
                        test,
                        3,
                        5,
                        3,
                    )} expected 14/13`,
                );
            },
        },
    );
}

// 火把(14) 衰减到 0：火把光 14，距离 14 格外应为 0。light_box 内部仅 5×5×5，最远曼哈顿距离
// 不足 14，故改验证"火把邻格=13"确认 14 级光源也按 -1 衰减（与 15 级荧石同规律）。
//
// 坐标：火把放 helper (3,2,3) = 结构内 (3,1,3) air 层，正下方结构内 (3,0,3)=stone 地板提供
// 支撑（standing torch 的 isValidPosition 要求 pos.down() 能 canSupportCenter(Up)，stone 满足）。
// 断言邻格 (4,2,3) = 结构内 (4,1,3) = air，火把光 14 经 air 减 max(1,0)=1 得 13。
// 注意：不可用 helper y=1（= 结构内 y=0 = stone 地板层）——那样火把嵌在 stone 中、邻格也是 stone
// (opacity=15)，targetLevel=0，光传不出来，是测试坐标误用而非引擎 bug。
function torchDecaysByOne(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 3, y: 2, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return getBlockLight(test, 3, 2, 3) === 14 && getBlockLight(test, 4, 2, 3) === 13;
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `torch: (3,2,3)=${getBlockLight(test, 3, 2, 3)} (4,2,3)=${getBlockLight(
                        test,
                        4,
                        2,
                        3,
                    )} expected 14/13`,
                );
            },
        },
    );
}

// 两光源叠加取 max：在 (1,3,3) 和 (5,3,3) 各放荧石(15)，中点 (3,3,3) 距两光源都为 2，
// 各贡献 15-2=13，取 max 仍为 13（非相加 26）。验证散射是 max 语义而非累加。
function twoSourcesTakeMax(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 1, y: 3, z: 3 });
    test.setBlockType("minecraft:glowstone", { x: 5, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            // 中点 (3,3,3)：距两光源曼哈顿距离均为 2，各贡献 13，max=13（非 26）。
            return getBlockLight(test, 3, 3, 3) === 13;
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `two glowstone max: midpoint (3,3,3)=${getBlockLight(test, 3, 3, 3)} expected 13 (max not sum)`,
                );
            },
        },
    );
}

export function registerBlockLightPropagationTests(): void {
    GameTest.register("LightingTests", "light_block_light_decays_along_x", glowstoneDecaysAlongX)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_decays_along_z", glowstoneDecaysAlongZ)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_decays_along_y", glowstoneDecaysAlongY)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_torch_decays_by_one", torchDecaysByOne)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_two_sources_take_max", twoSourcesTakeMax)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
