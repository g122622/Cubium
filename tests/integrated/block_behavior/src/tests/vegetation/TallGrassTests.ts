// 草丛支撑自毁行为 GameTest（移除下方植被地面时草丛被破坏）。
//
// wiki block_草灌木.txt / tech_矮草丛.txt：草丛（矮草）只能生长在草方块/泥土/砂土/灰化土/耕地等
// 植被地面上。当下方支撑方块被移除（位置不再合适）时草丛被破坏。
//
// C++ 链路：TallGrassBlock 继承 BushBlock，BushBlock::updatePostPlacement（BushBlock.cpp:67-93）当
// facing==Down 时检查下方 canSustain。TallGrassBlock 重写 canSustain（TallGrassBlock.cpp:63-70）调
// isVegetationGround（:34-41），判定下方是否为 grass_block/dirt/coarse_dirt/podzol/farmland。移除
// 下方 grass_block（→air）后，canSustain(air) isVegetationGround false → 返回 air 自毁。反应同 tick
// 同步（updatePostPlacement 直接返回 air）。
//
// 注意 typeId：1.21+ 矮草丛 typeId 为 minecraft:short_grass（原 tall_grass 在 1.21 改为双高植物
// DoublePlantBlock，矮草丛更名 short_grass）。本测试用 short_grass。
//
// 关键约束（同 SnowTests/SugarCaneTests）：
// 1. setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使下方非植被地面也能
//    强放草丛。放置自身不立即自毁（BushBlock 无 onBlockAdded 重写，放置不向自身派发
//    updatePostPlacement），需"第二步移除下方支撑"触发草丛 Down 方向 updatePostPlacement 才自毁。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 grass_block 支撑再放草丛，再设 air 移除支撑，保证
//    grass_block→air 真实状态变化派发更新。air 放置向 Up 邻居草丛派发 updatePostPlacement(Down) →
//    下方 air isVegetationGround false → 返回 air，草丛自毁。
//
// 不测「草丛放非植被地面（stone）上是否破坏」：setBlockType 绕过 isValidPosition 强放不触发自毁
// （放置不检查 isValidPosition，仅 Down 邻居变化才检查），与 vanilla「放置时拒绝」语义不同，按
// 「不为 JE/BE 不一致行为写测试」准则跳过（Cubium 强放是 GameTestHelper 测试便利，非生产放置路径）。
// 不测骨粉草丛生成/双高植物化：概率性，跳过。
//
// 跨服务端：草丛无 state（仅 defaultState），自毁行为与 vanilla 一致（下方支撑失效即破坏，同步），
// 可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_矮草丛.txt（草丛生长在植被地面）
// Ref: BushBlock.cpp（updatePostPlacement Down canSustain）+ TallGrassBlock.cpp（canSustain/isVegetationGround）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除草丛下方植被地面（grass_block）时草丛自毁变 air。
//
// 布局：(3,1,1) 铺 grass_block 作草丛下方支撑（isVegetationGround true），(3,2,1) 放 short_grass
// （在 grass_block 上，强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居草丛派发 updatePostPlacement(Down) → 下方 air isVegetationGround false → 返回 air。
//
// 判定：succeedWhenBlockPresent 断言草丛格 (3,2,1) 草丛消失（同 tick 同步）。
function shortGrassBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 grass_block 作草丛下方支撑（isVegetationGround 集合内，canSustain true）。
    test.setBlockType("minecraft:grass_block", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放草丛（在 grass_block 上，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:short_grass", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（grass_block→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向
    // Up 邻居草丛派发 updatePostPlacement(Down) → 下方 air isVegetationGround false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言草丛格 (3,2,1) 草丛已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:short_grass", { x: 3, y: 2, z: 1 }, false);
}

export function registerTallGrassTests(): void {
    GameTest.register("BlockBehaviorTests", "short_grass_breaks_when_support_below_removed", shortGrassBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
