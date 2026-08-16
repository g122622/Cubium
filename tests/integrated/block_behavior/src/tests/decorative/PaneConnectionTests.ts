// 玻璃板连接状态行为 GameTest（同步 updatePostPlacement 连接判定）。
//
// 玻璃板的 north/east/south/west bool state 表示是否与该方向邻居「连接」。连接判定由
// PaneBlock::shouldConnectTo（PaneBlock.cpp:262-286）实现，参考 vanilla PaneBlock#connectsTo：
//   1. 邻居是同类玻璃板（同一 Block 实例）→ 连接
//   2. 邻居是 BARS 标签（铁栏杆/铜栏杆）→ 连接
//   3. 邻居是墙（WALLS 标签）→ 连接
//   4. 邻居非 exception 且 isSolidSide（碰撞形状填满该面）→ 连接
//   否则 → 不连接
//
// 更新链路：PaneBlock::updatePostPlacement（PaneBlock.cpp:149-177）仅处理水平方向，按 facing
// switch 到对应方向 state 调 shouldConnectTo。邻居方块变化（setBlockState flags=3）同 tick 同步派发
// neighborChanged → updatePostPlacement 返回新 state，ServerWorld 同步写入。纯同步，无 tick 调度。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（连接全 false），不经 getStateForPlacement。
// 故单放一个玻璃板连接全 false；需在邻位放第二个方块触发第一个玻璃板的 updatePostPlacement 更新连接。
//
// 测试覆盖（4 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 玻璃板连玻璃板（同类）→ east=true
//   2. 玻璃板连固体方块（石头 isSolidSide）→ east=true
//   3. 玻璃板不连树叶（LEAVES exception）→ east=false
//   4. 玻璃板不连玻璃（玻璃 notSolid，isSolidSide=false）→ east=false
//
// 已知 Cubium 潜在偏差（不写测试，待修复后补）：
//   - 玻璃板连栅栏/下界砖栅栏：shouldConnectTo 第4条用 neighborBlock.isSolidSide（PaneBlock.cpp:284），
//     Cubium Block::isSolidSide 默认实现为 m_isSolid && m_hasCollision（Block.cpp:560），对栅栏类（有碰撞
//     的固体）仍返回 true，导致玻璃板误连栅栏/下界砖栅栏。vanilla 用 isSideSolid（碰撞形状是否填满
//     该面），栅栏碰撞非完整 → false 不连。同 FenceBlock 偏差根因（见 FenceConnectionTests 注释），
//     需 doesSideFillSquare 形状判定方能修复。
//   TODO: 待 Cubium 实现基于碰撞形状的 isSideSolid 后，补充「玻璃板不连栅栏」测试。
//
// 跨服务端：玻璃板 north/east/south/west bool state 名两端一致（Java 式），连接判定规则两端一致
// （同类/BARS/墙/固体 isSolidSide），可跨服务端对比。getState("east") 用 as any 绕过
// BlockStateSuperset 白名单（同栅栏/树叶范式）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_玻璃板.txt（玻璃板连接判定）
// Ref: PaneBlock.cpp（shouldConnectTo/updatePostPlacement）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 取玻璃板指定方向的连接 state（bool）。返回 null 表示读取失败。
function getPaneConnection(test: Test, x: number, y: number, z: number, dir: string): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState(dir as any);
    return typeof value === "boolean" ? value : null;
}

// 通用玻璃板连接断言：在 (3,1,1) 放主玻璃板，在其 East 邻位 (4,1,1) 放邻居方块，断言主玻璃板 east===expected。
//
// @param test GameTest Test 对象
// @param neighborType 邻居方块 typeId
// @param expected 主玻璃板 east 连接期望值（true=连接，false=不连接）
// @param label 超时错误标签
function assertPaneConnectsTo(
    test: Test,
    neighborType: string,
    expected: boolean,
    label: string,
): void {
    const panePos = { x: 3, y: 1, z: 1 };
    const neighborPos = { x: 4, y: 1, z: 1 };

    // 主玻璃板 (3,1,1)（defaultState，连接全 false）。
    test.setBlockType("minecraft:glass_pane", panePos);

    // East 邻位 (4,1,1) 放邻居方块。邻居放置触发主玻璃板 updatePostPlacement(East) 更新 east 连接。
    test.setBlockType(neighborType, neighborPos);

    // 轮询断言主玻璃板 east === expected。updatePostPlacement 同步，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getPaneConnection(test, 3, 1, 1, "east") === expected,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `${label}: pane east should be ${expected}, got ${getPaneConnection(test, 3, 1, 1, "east")}`,
                );
            },
        },
    );
}

// 玻璃板连玻璃板（同类）→ east=true。
// shouldConnectTo 第1条：邻居 glass_pane 与本方块同一 Block 实例 → 连接。
function paneConnectsToPane(test: Test): void {
    assertPaneConnectsTo(test, "minecraft:glass_pane", true, "pane-pane");
}

// 玻璃板连固体方块（石头）→ east=true。
// shouldConnectTo 第4条：石头 isSolidSide（m_isSolid && m_hasCollision）=true，非 exception → 连接。
function paneConnectsToSolidBlock(test: Test): void {
    assertPaneConnectsTo(test, "minecraft:stone", true, "pane-stone");
}

// 玻璃板不连树叶 → east=false。
// shouldConnectTo 第4条：树叶在 LEAVES 标签 → isExceptionForConnection true → 第4条 !exception false，
// 不连接。前几条不命中（树叶非同类非 BARS 非墙）→ false。
function paneDoesNotConnectToLeaves(test: Test): void {
    assertPaneConnectsTo(test, "minecraft:oak_leaves", false, "pane-leaves");
}

// 玻璃板不连玻璃 → east=false。
// shouldConnectTo 第4条：玻璃 Material::GLASS notSolid → isSolid=false → isSolidSide=false → 第4条
// isSolidSide 为 false 不连接。前几条不命中（玻璃非同类非 BARS 非墙）→ false。
function paneDoesNotConnectToGlass(test: Test): void {
    assertPaneConnectsTo(test, "minecraft:glass", false, "pane-glass");
}

export function registerPaneConnectionTests(): void {
    GameTest.register("BlockBehaviorTests", "pane_connects_to_pane", paneConnectsToPane)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pane_connects_to_solid_block", paneConnectsToSolidBlock)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pane_does_not_connect_to_leaves", paneDoesNotConnectToLeaves)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "pane_does_not_connect_to_glass", paneDoesNotConnectToGlass)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
