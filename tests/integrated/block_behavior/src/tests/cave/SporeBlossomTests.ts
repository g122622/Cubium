// 孢子花支撑自毁行为 GameTest（移除上方依附方块时孢子花被破坏）。
//
// wiki block_孢子花.txt#用法（:42）："孢子花能够被放置在任何底面方块支撑形状中心完整的方块底部。"
// :284 "孢子花在依附的方块被破坏之后不再悬浮于空气中。" 孢子花悬挂于天花板下方，依附方块（上方）
// 被破坏时孢子花掉落。
//
// C++ 链路：SporeBlossomBlock::updatePostPlacement（SporeBlossomBlock.cpp:60-76）仅当 facing==Up 且
// !isValidPosition 时返回 air 自毁。isValidPosition（:49-58）：canSupportCenter(world, pos.up(), Down)
// 且 !isWaterAt(pos)。canSupportCenter（Block.cpp:822-836）委托 isFaceSturdy(Center)，对完整碰撞形状
// 方块（stone/glass/cobblestone）返回 true。移除上方支撑方块（stone→air）后，pos.up() 为 air，
// canSupportCenter false → isValidPosition false → 自毁。反应同 tick 同步。
//
// 关键约束（同 SnowTests）：
// 1. setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使上方无支撑也能强放
//    孢子花。放置自身不立即自毁（SporeBlossomBlock 无 onBlockAdded 重写），需"第二步移除上方支撑"
//    触发孢子花 Up 方向 updatePostPlacement 才自毁。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 stone 上方支撑再放孢子花，再设 air 移除上方支撑，保证
//    stone→air 真实状态变化派发更新。air 放置向 Down 邻居孢子花派发 updatePostPlacement(Up) →
//    pos.up()=air canSupportCenter false → isValidPosition false → 返回 air，孢子花自毁。
// 3. 孢子花挂在 y=1，上方支撑在 y=2（glass_pit 内部空腔，y=2 默认 air，需显式铺 stone）。
//
// 不测「水中放置孢子花被破坏」：wiki :285 该行为为历史 bug 修复，1.21.11 已不可在水中放置，且
// isValidPosition 的 isWaterAt 检查在强放场景下不触发（setBlockType 绕过 isValidPosition），测试
// 难以稳定覆盖，跳过。
// 不测 animateTick 粒子：客户端渲染行为，无头跑不可测。
//
// 跨服务端：孢子花无 state（仅 defaultState），自毁行为与 vanilla 一致（依附方块破坏即掉落），
// 可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_孢子花.txt#用法（中心完整方块底部放置）
// Ref: SporeBlossomBlock.cpp（updatePostPlacement/isValidPosition）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除孢子花上方依附方块（stone）时孢子花自毁变 air。
//
// 布局：(3,2,1) 铺 stone 作孢子花上方依附支撑（中心完整，canSupportCenter true），(3,1,1) 放孢子花
// （悬挂于 stone 下方，强放绕过 isValidPosition 不立即自毁），再 (3,2,1) 设 air 移除上方支撑。
// air 放置向 Down 邻居孢子花派发 updatePostPlacement(Up) → pos.up()=air canSupportCenter false →
// isValidPosition false → 返回 air，孢子花自毁。
//
// 判定：succeedWhenBlockPresent 断言孢子花格 (3,1,1) 孢子花消失（同 tick 同步）。
function sporeBlossomBreaksWhenSupportAboveRemoved(test: Test): void {
    // (3,2,1) 铺 stone 作孢子花上方依附支撑（完整方块，canSupportCenter(Down) true）。
    test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });

    // (3,1,1) 放孢子花（悬挂于 stone 下方，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:spore_blossom", { x: 3, y: 1, z: 1 });

    // (3,2,1) 设 air 移除上方支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向
    // Down 邻居孢子花派发 updatePostPlacement(Up) → pos.up()=air canSupportCenter false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 2, z: 1 });

    // 断言孢子花格 (3,1,1) 孢子花已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:spore_blossom", { x: 3, y: 1, z: 1 }, false);
}

export function registerSporeBlossomTests(): void {
    GameTest.register("BlockBehaviorTests", "spore_blossom_breaks_when_support_above_removed", sporeBlossomBreaksWhenSupportAboveRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
