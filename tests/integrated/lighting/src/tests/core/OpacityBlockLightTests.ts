// 方块光穿透不同 opacity 方块的衰减测试：验证 targetLevel = sourceLevel - max(1, opacity) 语义。
//
// Cubium BlockStarLightEngine 传播（BaseLightEngine.cpp:1095-1096）：每穿一格 targetLevel =
// propagatedLevel - max(1, opacity)。空气 opacity=0 → max(1,0)=1 衰减1；玻璃 opacity=0 同样衰减1
// （不因 opacity=0 而不衰减）；树叶 opacity=1 衰减1；石头 opacity=15 衰减15（阻断，targetLevel<=0）。
//
// wiki「方块光照」（tech_亮度.txt#方块光照）：光源向毗邻六方向传播衰减1，按曼哈顿距离。火把(14)
// 四周=13，对角=12（14-1-1）。墙上方火把对角地板方块=11（14-1-1-1，三维曼哈顿）。
//
// 设计：light_box（7×7×7 封顶实心盒子，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光，方块光是唯一光源）。
// 中心 (3,3,3) 放光源，验证沿各方向穿透不同方块的衰减。light_box 封顶保证 skyLight=0，blockLight 不受干扰。
//
// 断言值来源（核查结论）：
//   - glass getOpacity=0（显式 .opacity(0)），方块光穿玻璃 max(1,0)=1 衰减1（与空气相同）。
//   - oak_leaves getOpacity=1（显式），穿树叶 max(1,1)=1 衰减1。
//   - stone getOpacity=15（ROCK isOpaque），穿石头 max(1,15)=15 衰减15 → 阻断（targetLevel<=0）。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#方块光照（每格衰减1，曼哈顿距离）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// 方块光穿过玻璃仍衰减1：光源 (1,3,3) 荧石15，(2,3,3) 放玻璃(opacity=0)，(3,3,3) 探针 air。
// 传播链：荧石格15 → 玻璃格 max(1,0)=1 衰减得 14 → 探针 air 衰减1 得 13。
// 验证 opacity=0 的玻璃对方块光仍衰减1（max(1,0)=1，非0不衰减）。玻璃格自身 blockLight=14。
function blockLightThroughGlassDecaysOne(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 1, y: 3, z: 3 });
    test.setBlockType("minecraft:glass", { x: 2, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 玻璃格：邻接荧石15，穿玻璃衰减 max(1,0)=1 得 14。
                getBlockLight(test, 2, 3, 3) === 14 &&
                // 探针 air：穿玻璃(14)再穿空气衰减1 得 13。
                getBlockLight(test, 3, 3, 3) === 13
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `glass decay: glass(2,3,3)=${getBlockLight(test, 2, 3, 3)} probe(3,3,3)=${getBlockLight(
                        test,
                        3,
                        3,
                        3,
                    )} expected 14/13`,
                );
            },
        },
    );
}

// 方块光穿过树叶衰减1：光源 (1,3,3) 荧石15，(2,3,3) 树叶(opacity=1)，(3,3,3) 探针。
// 传播链：荧石15 → 树叶格 max(1,1)=1 衰减得 14 → 探针 air 衰减1 得 13。
// 与玻璃测试对比：玻璃 opacity=0 与树叶 opacity=1 衰减量相同（都是 max(1,opacity)=1），验证
// "opacity<1 的方块统一按1衰减"语义。树叶格自身 blockLight=14。
function blockLightThroughLeavesDecaysOne(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 1, y: 3, z: 3 });
    test.setBlockType("minecraft:oak_leaves", { x: 2, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 树叶格：邻接荧石15，穿树叶衰减 max(1,1)=1 得 14。
                getBlockLight(test, 2, 3, 3) === 14 &&
                // 探针 air：穿树叶(14)再穿空气衰减1 得 13。
                getBlockLight(test, 3, 3, 3) === 13
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaves decay: leaves(2,3,3)=${getBlockLight(test, 2, 3, 3)} probe(3,3,3)=${getBlockLight(
                        test,
                        3,
                        3,
                        3,
                    )} expected 14/13`,
                );
            },
        },
    );
}

// 石头阻断方块光（封闭走廊构造）：光源 (1,3,3) 荧石15，(2,3,3) 石头(opacity=15) 阻断直线路径，
// 探针 (3,3,3) air。若仅在直线上放单格石头，光会从开放空间绕路（曼哈顿4，15-4=11）到达探针，
// 无法验证「完全阻断」。故把探针其余 5 面也封上石头，使探针唯一开口朝向阻断石头 (2,3,3)：
//   探针 (3,3,3) 的 6 邻居：(2,3,3) 石头、(4,3,3) 石头、(3,2,3) 石头、(3,4,3) 石头、
//   (3,3,2) 石头、(3,3,4) 石头——六面皆 opacity=15 的实心方块。
// 探针只能从邻居接收光，而所有邻居 blockLight=0（opacity=15 不被点亮），故探针 blockLight=0。
// 这确定性地验证 opacity=15 实心方块完全阻断方块光传播（光无法穿透实心方块到达被其包围的空腔）。
// 石头格自身 blockLight=0（calculateLightValue 对 opacity>=15 直接返回 level=0）。
function blockLightBlockedByStone(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 1, y: 3, z: 3 });
    // 直线阻断石头。
    test.setBlockType("minecraft:stone", { x: 2, y: 3, z: 3 });
    // 封死探针其余 5 面，阻止光从开放空间绕路（否则绕路曼哈顿4，15-4=11≠0）。
    test.setBlockType("minecraft:stone", { x: 4, y: 3, z: 3 });
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 3 });
    test.setBlockType("minecraft:stone", { x: 3, y: 4, z: 3 });
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 2 });
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 4 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 阻断石头格 opacity=15，不接收邻居光，自身 blockLight=0。
                getBlockLight(test, 2, 3, 3) === 0 &&
                // 探针 air：六面皆 opacity=15 石头（blockLight=0），无法接收任何光，得 0。
                getBlockLight(test, 3, 3, 3) === 0
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `stone block: stone(2,3,3)=${getBlockLight(test, 2, 3, 3)} probe(3,3,3)=${getBlockLight(
                        test,
                        3,
                        3,
                        3,
                    )} expected 0/0`,
                );
            },
        },
    );
}

// 火把(14) 二维对角衰减（曼哈顿距离2）：火把放 (3,3,3)，对角 (4,3,4) 距火把曼哈顿2（东1+南1）→ 12。
// 验证方块光按曼哈顿距离衰减（非欧几里得距离）：对角格=14-1-1=12，与火把邻格(13)区分。
// wiki 原文示例：火把四周=13，对角=12。
function torchDiagonalDecaysByManhattanTwo(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 火把邻格 (4,3,3) 距离1 → 13。
                getBlockLight(test, 4, 3, 3) === 13 &&
                // 对角 (4,3,4) 距离2（东1+南1）→ 12。
                getBlockLight(test, 4, 3, 4) === 12
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `torch diagonal: neighbor(4,3,3)=${getBlockLight(test, 4, 3, 3)} diagonal(4,3,4)=${getBlockLight(
                        test,
                        4,
                        3,
                        4,
                    )} expected 13/12`,
                );
            },
        },
    );
}

// 火把(14) 三维斜对角衰减（曼哈顿距离3）：火把放 (3,3,3)，(4,2,4) 距火把曼哈顿3（东1+下1+南1）→ 11。
// 对齐 wiki 原文示例："墙上方火把对角地板方块=11（14-1-1-1）"。
// 验证三维曼哈顿距离衰减：14-1(东)-1(下)-1(南)=11。
function torchThreeAxisDiagonalDecaysToEleven(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            // (4,2,4) 距火把 (3,3,3) 曼哈顿3（东1+下1+南1）→ 14-3=11。
            return getBlockLight(test, 4, 2, 4) === 11;
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `torch 3-axis diagonal: (4,2,4)=${getBlockLight(test, 4, 2, 4)} expected 11 (14-1-1-1 manhattan)`,
                );
            },
        },
    );
}

// 火把(14) 衰减到0：火把光14，距离14格外应为0。light_box 内部仅 5×5×5，中心到角最远曼哈顿距离
// (3,3,3)→(1,1,1)=6，14-6=8 仍>0，无法在盒内验证衰减到0。改为验证"荧石(15)沿单轴衰减到边墙仍>0，
// 而墙外（实心石头层）=0"：荧石(3,3,3)15 → (5,3,3)=13（距离2）→ 墙 (6,3,3)=stone，墙格 blockLight=0
// （opacity=15 阻断）。验证方块光不会穿透实心墙到墙外。
function blockLightDoesNotPenetrateSolidWall(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 内部边格 (5,3,3) 距荧石2 → 13（仍在空气内传播）。
                getBlockLight(test, 5, 3, 3) === 13 &&
                // 墙格 (6,3,3) 是 light_box 的 stone 墙（opacity=15），blockLight=0（阻断）。
                getBlockLight(test, 6, 3, 3) === 0
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `wall penetration: edge(5,3,3)=${getBlockLight(test, 5, 3, 3)} wall(6,3,3)=${getBlockLight(
                        test,
                        6,
                        3,
                        3,
                    )} expected 13/0`,
                );
            },
        },
    );
}

export function registerOpacityBlockLightTests(): void {
    GameTest.register("LightingTests", "light_block_light_through_glass_decays_one", blockLightThroughGlassDecaysOne)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_through_leaves_decays_one", blockLightThroughLeavesDecaysOne)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_blocked_by_stone", blockLightBlockedByStone)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_torch_diagonal_manhattan", torchDiagonalDecaysByManhattanTwo)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register(
        "LightingTests",
        "light_block_light_torch_three_axis_diagonal",
        torchThreeAxisDiagonalDecaysToEleven,
    )
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register(
        "LightingTests",
        "light_block_light_does_not_penetrate_wall",
        blockLightDoesNotPenetrateSolidWall,
    )
        .structureName("gametests:light_box")
        .maxTicks(120);
}
