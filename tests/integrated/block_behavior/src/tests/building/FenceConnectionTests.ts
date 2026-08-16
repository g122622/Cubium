// 栅栏连接状态行为 GameTest（同步 updatePostPlacement 连接判定）。
//
// 栅栏的 north/east/south/west bool state 表示是否与该方向邻居「连接」。连接判定由
// FenceBlock::_canConnect（FenceBlock.cpp:267-294）实现，参考 vanilla FenceBlock#connectsTo：
//   1. 邻居是栅栏门且平行 → 连接
//   2. 邻居在 FENCES 标签且（本方块∈WOODEN_FENCES）==（邻居∈WOODEN_FENCES）→ 连接
//      （木质栅栏连木质栅栏，下界砖栅栏连下界砖栅栏，木质不连下界砖）
//   3. 邻居非 exception（LEAVES/SHULKER_BOXES/barrier/南瓜/雕刻南瓜/西瓜）且 isSolid → 连接
//
// 更新链路：FenceBlock::updatePostPlacement（FenceBlock.cpp:153-187）只处理水平方向，按 facing
// switch 到对应方向 state。邻居方块变化（setBlockState flags=3）时同 tick 同步派发 neighborChanged
// → updatePostPlacement 返回新 state，ServerWorld 同步写入。纯同步，无 tick 调度。
//
// 放置语义：setBlockType 走 GameTestHelper::setBlock → _resolveBlock 取 defaultState（连接全 false），
// 不经 getStateForPlacement。故单放一个栅栏连接全 false；需在邻位放第二个方块触发第一个栅栏的
// updatePostPlacement 更新连接 state。第二个方块放置时也触发双向收敛（A.EAST=true 后 A 变化
// 派发 neighborChanged 给 B → B.WEST=true）。
//
// 测试覆盖（4 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 木质栅栏连木质栅栏（同类）→ east=true
//   2. 栅栏连固体方块（石头 isSolid）→ east=true
//   3. 栅栏不连树叶（LEAVES 在 exception 列表）→ east=false
//   4. 栅栏不连玻璃（玻璃 notSolid，isSolid=false）→ east=false
//
// 已知 Cubium 偏差（不写测试，待修复后补）：
//   - 木质栅栏会连下界砖栅栏（实际 east=true，vanilla 应 false）。根因：FenceBlock::_canConnect
//     第3条用 facingState.isSolid()（FenceBlock.cpp:289），下界砖栅栏 Material::ROCK 无 notSolid
//     故 isSolid=true，导致木质栅栏误连下界砖栅栏。vanilla 用 isSideSolid（碰撞形状是否填满该面），
//     栅栏碰撞形状非完整故 isSideSolid=false 不连。Cubium 的 Block::isSolidSide 默认实现是
//     `m_isSolid && m_hasCollision`（Block.cpp:560），对栅栏类（有碰撞的固体）仍返回 true，不能区分
//     完整方块与栅栏形碰撞，故即改用 isSolidSide 也无法修复——需 doesSideFillSquare 碰撞形状判定。
//     PaneBlock.shouldConnectTo 同样用 isSolidSide（PaneBlock.cpp:284），存在同类潜在偏差。
//   TODO: 待 Cubium 实现基于碰撞形状的 isSideSolid（对齐 vanilla doesSideFillSquare）后，补充
//   「木质栅栏不连下界砖栅栏」「栅栏不连墙」等测试。
//
// 不写「栅栏是否连墙」测试：同上根因，墙 Material::ROCK isSolid=true，栅栏会连墙（实际 east=true），
// 与 vanilla 不一致，待 isSideSolid 修复后补。
//
// 跨服务端：栅栏 north/east/south/west bool state 名两端一致（Java 式），连接判定规则两端一致，
// 可跨服务端对比。getState("east") 用 as any 绕过 BlockStateSuperset 白名单（同 LeavesDistanceTests）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_栅栏.txt（栅栏连接判定）
// Ref: FenceBlock.cpp（_canConnect/updatePostPlacement）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 取栅栏指定方向的连接 state（bool）。返回 null 表示非栅栏或读取失败。
function getFenceConnection(test: Test, x: number, y: number, z: number, dir: string): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(dir as any);
    return typeof value === "boolean" ? value : null;
}

// 通用栅栏连接断言：在 (3,1,1) 放主栅栏，在其 East 邻位 (4,1,1) 放邻居方块，断言主栅栏 east===expected。
//
// @param test GameTest Test 对象
// @param neighborType 邻居方块 typeId
// @param expected 主栅栏 east 连接期望值（true=连接，false=不连接）
// @param label 超时错误标签
function assertFenceConnectsTo(
    test: Test,
    neighborType: string,
    expected: boolean,
    label: string,
): void {
    const fencePos = { x: 3, y: 1, z: 1 };
    const neighborPos = { x: 4, y: 1, z: 1 };

    // 主栅栏 (3,1,1)（defaultState，连接全 false）。setBlockType 直写不经 getStateForPlacement。
    test.setBlockType("minecraft:oak_fence", fencePos);

    // East 邻位 (4,1,1) 放邻居方块。邻居放置触发主栅栏 updatePostPlacement(East) 更新 east 连接。
    test.setBlockType(neighborType, neighborPos);

    // 轮询断言主栅栏 east === expected。updatePostPlacement 同步，但双向收敛（A.east 变化再派发
    // 给 B）可能需 1-2 tick，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getFenceConnection(test, 3, 1, 1, "east") === expected,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `${label}: fence east should be ${expected}, got ${getFenceConnection(test, 3, 1, 1, "east")}`,
                );
            },
        },
    );
}

// 木质栅栏连木质栅栏（同类）→ east=true。
// _canConnect 第2条：邻居 oak_fence ∈ FENCES，且本方块与邻居都在 WOODEN_FENCES → 返回 true。
function fenceConnectsToFence(test: Test): void {
    assertFenceConnectsTo(test, "minecraft:oak_fence", true, "fence-fence");
}

// 栅栏连固体方块（石头）→ east=true。
// _canConnect 第3条：石头 Material::ROCK isSolid=true，非 exception → 返回 true。
function fenceConnectsToSolidBlock(test: Test): void {
    assertFenceConnectsTo(test, "minecraft:stone", true, "fence-stone");
}

// 栅栏不连树叶 → east=false。
// _canConnect 第3条：树叶在 LEAVES 标签 → isExceptionForConnection 返回 true → 第3条 !exception 为 false，
// 不连接。前两条也不命中（树叶非栅栏门非栅栏）→ false。
function fenceDoesNotConnectToLeaves(test: Test): void {
    assertFenceConnectsTo(test, "minecraft:oak_leaves", false, "fence-leaves");
}

// 栅栏不连玻璃 → east=false。
// _canConnect 第3条：玻璃 Material::GLASS notSolid → isSolid=false → 第3条 isSolid 为 false 不连接。
// 前两条不命中（玻璃非栅栏门非栅栏）→ false。
function fenceDoesNotConnectToGlass(test: Test): void {
    assertFenceConnectsTo(test, "minecraft:glass", false, "fence-glass");
}

export function registerFenceConnectionTests(): void {
    GameTest.register("BlockBehaviorTests", "fence_connects_to_fence", fenceConnectsToFence)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "fence_connects_to_solid_block", fenceConnectsToSolidBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "fence_does_not_connect_to_leaves", fenceDoesNotConnectToLeaves)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "fence_does_not_connect_to_glass", fenceDoesNotConnectToGlass)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
