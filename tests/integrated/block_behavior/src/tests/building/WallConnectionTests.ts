// 墙连接状态行为 GameTest（同步 updatePostPlacement 连接高度判定）。
//
// 墙的 north/east/south/west state 表示该方向连接高度（none/low/tall，WallHeight enum），up 表示
// 墙柱是否升起。连接判定由 WallBlock::_getWallHeight（WallBlock.cpp:369-414）实现，参考 vanilla WallBlock：
//   1. 邻居是墙（WALLS 标签）→ 连接（Low，上方覆盖则 Tall）
//   2. 邻居是栅栏门且平行 → 连接（Low）
//   3. 邻居是铁栏杆（BARS 标签）→ 连接（Low/Tall）
//   4. 邻居非 exception 且 isSolid → 连接（Low/Tall）
//   否则 → None
//
// 更新链路：WallBlock::updatePostPlacement（WallBlock.cpp:167-184）任意邻居变化都调 _calculateState
// （:324-367）重算全部四方向连接高度 + up（与栅栏仅更新 facing 方向不同，墙每次全量重算）。
// 邻居方块变化（setBlockState flags=3）同 tick 同步派发 neighborChanged → updatePostPlacement →
// _calculateState 返回新 state，ServerWorld 同步写入。纯同步，无 tick 调度。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（north/east/south/west 全 none, up=true），
// 不经 getStateForPlacement。故单放一个墙连接全 none；需在邻位放第二个方块触发第一个墙的
// updatePostPlacement 重算连接。
//
// 测试覆盖（4 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 墙连墙（同类）→ east=low
//   2. 墙连固体方块（石头 isSolid）→ east=low
//   3. 墙不连树叶（LEAVES exception）→ east=none
//   4. 墙不连玻璃（玻璃 notSolid，isSolid=false）→ east=none
//
// 已知 Cubium 潜在偏差（不写测试，待修复后补）：
//   - 墙连栅栏/下界砖栅栏：_getWallHeight 第4条用 state.isSolid()（WallBlock.cpp:405），下界砖栅栏
//     Material::ROCK isSolid=true → 墙会连下界砖栅栏（vanilla 用 isSideSolid 碰撞形状判定，栅栏非完整
//     不连）。同 FenceBlock 的 isSolid 偏差根因（见 FenceConnectionTests 注释）。待 doesSideFillSquare
//     形状判定落地后补「墙不连栅栏」测试。
//   TODO: 待 Cubium 实现基于碰撞形状的 isSideSolid 后，补充「墙不连栅栏/下界砖栅栏」测试。
//
// 不测连接高度 Tall（需上方方块碰撞形状覆盖墙臂测试形状）：Tall 判定依赖 isCovered(墙臂测试形状,
// 上方面投影)，需在墙上方放完整方块且配置复杂，本文件聚焦「连接/不连接」核心判定（Low/None），
// Tall 高度分支留待后续。TODO: 可补 wall_tall_when_solid_above 测试覆盖 Tall 分支。
//
// 跨服务端：墙连接高度 state 名两端均为 north/east/south/west（Java 式 WallHeight enum，值
// none/low/tall），连接判定规则两端一致，可跨服务端对比。getState("east") 用 as any 绕过
// BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_圆石墙.txt（墙连接高度 none/low/tall）
// Ref: WallBlock.cpp（_calculateState/_getWallHeight/updatePostPlacement）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 取墙指定方向的连接高度 state（"none"/"low"/"tall"）。返回 null 表示读取失败。
//
// 注意 state 名：Cubium 墙的连接高度 state 名为 north/east/south/west（Properties.hpp:867/877/887/897
// WALL_HEIGHT_NORTH/EAST/SOUTH/WEST 的 create name 均为 "north"/"east"/"south"/"west"，Java 式），与
// 栅栏/玻璃板 bool state 同名但墙是 WallHeight enum。getState 对 enum 返回 toString 字符串
// "none"/"low"/"tall"。
function getWallHeight(test: Test, x: number, y: number, z: number, dir: string): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(dir as any);
    return typeof value === "string" ? value : null;
}

// 通用墙连接断言：在 (3,1,1) 放主墙，在其 East 邻位 (4,1,1) 放邻居方块，断言主墙 east===expected。
//
// @param test GameTest Test 对象
// @param neighborType 邻居方块 typeId
// @param expected 主墙 east 期望值（"low"/"none"）
// @param label 超时错误标签
function assertWallConnectsTo(
    test: Test,
    neighborType: string,
    expected: string,
    label: string,
): void {
    const wallPos = { x: 3, y: 1, z: 1 };
    const neighborPos = { x: 4, y: 1, z: 1 };

    // 主墙 (3,1,1)（defaultState，wall_height 全 none, up=true）。
    test.setBlockType("minecraft:cobblestone_wall", wallPos);

    // East 邻位 (4,1,1) 放邻居方块。邻居放置触发主墙 updatePostPlacement → _calculateState 重算全部
    // 四方向连接高度（含 east）。
    test.setBlockType(neighborType, neighborPos);

    // 轮询断言主墙 east === expected。updatePostPlacement 同步，但 _calculateState 全量重算
    // 可能需 1-2 tick 收敛，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getWallHeight(test, 3, 1, 1, "east") === expected,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `${label}: wall east should be ${expected}, got ${getWallHeight(test, 3, 1, 1, "east")}`,
                );
            },
        },
    );
}

// 墙连墙（同类）→ east=low。
// _getWallHeight 第1条：邻居 cobblestone_wall ∈ WALLS 标签 → 连接。无上方覆盖 → Low。
function wallConnectsToWall(test: Test): void {
    assertWallConnectsTo(test, "minecraft:cobblestone_wall", "low", "wall-wall");
}

// 墙连固体方块（石头）→ east=low。
// _getWallHeight 第4条：石头 Material::ROCK isSolid=true，非 exception → 连接。无上方覆盖 → Low。
function wallConnectsToSolidBlock(test: Test): void {
    assertWallConnectsTo(test, "minecraft:stone", "low", "wall-stone");
}

// 墙不连树叶 → east=none。
// _getWallHeight 第4条：树叶在 LEAVES 标签 → isExceptionForConnection true → 第4条 !exception 为 false，
// 不连接。前几条也不命中（树叶非墙非栅栏门非铁栏杆）→ None。
function wallDoesNotConnectToLeaves(test: Test): void {
    assertWallConnectsTo(test, "minecraft:oak_leaves", "none", "wall-leaves");
}

// 墙不连玻璃 → east=none。
// _getWallHeight 第4条：玻璃 Material::GLASS notSolid → isSolid=false → 第4条 isSolid 为 false 不连接。
// 前几条不命中（玻璃非墙非栅栏门非铁栏杆）→ None。
function wallDoesNotConnectToGlass(test: Test): void {
    assertWallConnectsTo(test, "minecraft:glass", "none", "wall-glass");
}

export function registerWallConnectionTests(): void {
    GameTest.register("BlockBehaviorTests", "wall_connects_to_wall", wallConnectsToWall)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "wall_connects_to_solid_block", wallConnectsToSolidBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "wall_does_not_connect_to_leaves", wallDoesNotConnectToLeaves)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "wall_does_not_connect_to_glass", wallDoesNotConnectToGlass)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
