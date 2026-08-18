// 方块光传播衰减对称性与累积衰减测试：验证「衰减仅依赖曼哈顿距离、与方向无关」的正八面体扩散语义，
// 以及「连续多个 opacity=0 方块仍每格衰减1」的累积衰减语义。
//
// 核心机制（BaseLightEngine.cpp:1095-1096 / BlockLightEngine.cpp:282）：
//   targetLevel = propagatedLevel - max(1, opacity)
// 空气 opacity=0 → max(1,0)=1，每穿一格衰减1。衰减量只取决于「传播路径上的格数」（曼哈顿距离），
// 与方向无关——这使方块光扩散呈正八面体形状（wiki tech_亮度.txt#方块光照 原文：「上述这种衰减特性
// 会使光源周围光照的扩散近似呈正八面体」）。
//
// 本组验证三个 wiki 一致、当前未覆盖的确定性语义：
//   1. 正八面体对称性：火把(14) 的四个水平对角（曼哈顿距离2）blockLight 都应=12。当前
//      OpacityBlockLightTests.torchDiagonalDecaysByManhattanTwo 只测了 (4,3,4) 一个对角，
//      本组补齐四个方向（东南/西南/东北/西北对角）验证衰减与方向无关。
//   2. 轴向与对角同距离等价：轴向 (5,3,3) 距火把 (3,3,3) 曼哈顿2，对角 (4,3,4) 也曼哈顿2，
//      两者 blockLight 都=12。验证「相同曼哈顿距离→相同 blockLight」（正八面体等距面）。
//   3. 连续多个 opacity=0 方块累积衰减：荧石(15) → 玻璃 → 玻璃 → air，每格衰减1，得 14/13/12。
//      验证 opacity=0 的方块连续穿透时衰减按格数累积（非按方块数），与空气同等衰减。
//      当前 OpacityBlockLightTests.blockLightThroughGlassDecaysOne 只测单层玻璃，本组补双层。
//
// 设计：light_box（7×7×7 封顶实心盒，内部 x,z∈[1,5] y∈[1,5]，skyLight=0 隔绝天空光，方块光是唯一光源）。
// 中心 (3,3,3) 放光源，验证各方向/距离的 blockLight。light_box 封顶保证 skyLight=0，blockLight 不受干扰。
//
// 光照重算异步：setBlock 后入队 m_lightQueue，pollUntilSucceed 轮询等到所有目标格达预期。
//
// 跨服务端：blockLight 是 Cubium 专有，基岩端 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_亮度.txt#方块光照（每格衰减1，曼哈顿距离，正八面体扩散）
// Ref: OpacityBlockLightTests.ts（单层玻璃/树叶衰减1、火把二维/三维对角曼哈顿衰减）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";
import { getBlockLight } from "../utils/lightAssert.js";

// 火把正八面体对称性：火把(14) 放 (3,3,3)，四个水平对角（曼哈顿距离2）blockLight 都应=12。
// 四个对角：(4,3,4) 东南、(2,3,4) 西南、(4,3,2) 东北、(2,3,2) 西北，距 (3,3,3) 曼哈顿均=2（|dx|+|dz|=2）。
// 验证衰减仅依赖曼哈顿距离、与方向无关（正八面体等距面四个方向 blockLight 相等）。
// wiki 原文：「光源周围光照的扩散近似呈正八面体」——同曼哈顿距离的格点等亮。
function torchOctahedralSymmetry(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 四个水平对角，曼哈顿距离均=2，blockLight 都应=12（14-1-1）。
                getBlockLight(test, 4, 3, 4) === 12 &&
                getBlockLight(test, 2, 3, 4) === 12 &&
                getBlockLight(test, 4, 3, 2) === 12 &&
                getBlockLight(test, 2, 3, 2) === 12
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `torch octahedral: SE(4,3,4)=${getBlockLight(test, 4, 3, 4)} ` +
                        `SW(2,3,4)=${getBlockLight(test, 2, 3, 4)} ` +
                        `NE(4,3,2)=${getBlockLight(test, 4, 3, 2)} ` +
                        `NW(2,3,2)=${getBlockLight(test, 2, 3, 2)} expected all 12 (manhattan 2)`,
                );
            },
        },
    );
}

// 轴向与对角同曼哈顿距离等价：火把(14) (3,3,3)，轴向 (5,3,3) 曼哈顿2，对角 (4,3,4) 曼哈顿2。
// 两者 blockLight 都应=12。验证「相同曼哈顿距离→相同 blockLight」，无论路径是直走两格还是斜走两格
// （正八面体等距面：轴向距离2 与对角距离2 在同一等距面上，等亮）。
// 区别于 torchOctahedralSymmetry（只比四个对角互相相等）：此处对比「轴向」与「对角」两类方向等距等亮。
function torchAxialEqualsDiagonalAtSameDistance(test: Test): void {
    test.setBlockType("minecraft:torch", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 轴向 (5,3,3)：距火把东2格，曼哈顿2 → 12。
                getBlockLight(test, 5, 3, 3) === 12 &&
                // 对角 (4,3,4)：距火把东1+南1，曼哈顿2 → 12。
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
                    `axial vs diagonal: axial(5,3,3)=${getBlockLight(test, 5, 3, 3)} ` +
                        `diagonal(4,3,4)=${getBlockLight(test, 4, 3, 4)} expected both 12 (manhattan 2)`,
                );
            },
        },
    );
}

// 方块光穿过连续两个 opacity=0 方块累积衰减：荧石(15) (1,3,3) → 玻璃(2,3,3) → 玻璃(3,3,3) → air(4,3,3) 探针。
// 传播链：荧石15 → 穿玻璃1 max(1,0)=1 衰减得 14 → 穿玻璃2 衰减1 得 13 → 穿 air 衰减1 得 12。
// 验证连续多个 opacity=0 方块穿透时衰减按格数累积（每格-1），与连续空气同等衰减（非按方块数或一次性衰减）。
// 当前 OpacityBlockLightTests.blockLightThroughGlassDecaysOne 只测单层玻璃（15→14→13），本组补双层（15→14→13→12）。
function blockLightThroughTwoGlassLayers(test: Test): void {
    test.setBlockType("minecraft:glowstone", { x: 1, y: 3, z: 3 });
    test.setBlockType("minecraft:glass", { x: 2, y: 3, z: 3 });
    test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });
    pollUntilSucceed(
        test,
        () => {
            return (
                // 玻璃1 (2,3,3)：邻接荧石15，穿玻璃衰减1 得 14。
                getBlockLight(test, 2, 3, 3) === 14 &&
                // 玻璃2 (3,3,3)：穿玻璃1(14)再穿玻璃2衰减1 得 13。
                getBlockLight(test, 3, 3, 3) === 13 &&
                // 探针 air (4,3,3)：穿玻璃2(13)再穿空气衰减1 得 12。
                getBlockLight(test, 4, 3, 3) === 12
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 100,
            onTimeout: () => {
                test.assert(
                    false,
                    `two glass layers: glass1(2,3,3)=${getBlockLight(test, 2, 3, 3)} ` +
                        `glass2(3,3,3)=${getBlockLight(test, 3, 3, 3)} ` +
                        `probe(4,3,3)=${getBlockLight(test, 4, 3, 3)} expected 14/13/12`,
                );
            },
        },
    );
}

export function registerBlockLightPropagationSymmetryTests(): void {
    GameTest.register("LightingTests", "light_block_light_torch_octahedral_symmetry", torchOctahedralSymmetry)
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register(
        "LightingTests",
        "light_block_light_torch_axial_equals_diagonal",
        torchAxialEqualsDiagonalAtSameDistance,
    )
        .structureName("gametests:light_box")
        .maxTicks(120);
    GameTest.register("LightingTests", "light_block_light_through_two_glass_layers", blockLightThroughTwoGlassLayers)
        .structureName("gametests:light_box")
        .maxTicks(120);
}
