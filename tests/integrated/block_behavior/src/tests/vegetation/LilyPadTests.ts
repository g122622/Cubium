// 睡莲水支撑自毁行为 GameTest（移除下方水时睡莲被破坏）。
//
// wiki tech_睡莲.txt#破坏（:55）："睡莲会在其附着的方块被替换为除水、冰、霜冰和含水方块以外的
// 方块的情况下被破坏并掉落自身。" 即睡莲下方支撑（水）被替换为非水方块（如 air）时睡莲自毁。
// :53 "睡莲能够放在水、冰、霜冰或含水方块的上方。"
//
// C++ 链路：LilyPadBlock 继承 BushBlock，BushBlock::updatePostPlacement（BushBlock.cpp:67-93）当
// facing==Down 时检查下方 canSustain。LilyPadBlock 重写 canSustain（LilyPadBlock.cpp:115-123）返回
// 下方方块 material.isLiquid()。移除下方水（→air）后，canSustain(air) isLiquid false → 返回 air
// 自毁。反应同 tick 同步。
//
// 关键约束（同 SnowTests）：
// 1. setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使下方无水也能强放
//    睡莲。放置自身不立即自毁，需"第二步移除下方水"触发睡莲 Down 方向 updatePostPlacement 才自毁。
// 2. 移除下方水必须是非 no-op 写入——先放水再放睡莲，再设 air 移除水，保证 water→air 真实状态
//    变化派发更新。air 放置向 Up 邻居睡莲派发 updatePostPlacement(Down) → 下方 air isLiquid false
//    → 返回 air，睡莲自毁。
// 3. 水流动：setBlockType("minecraft:water") 放水源后水会向相邻 air 流动（LiquidBlock 流动，tick
//    延迟）。但不影响测试：睡莲在 y=2，支撑判定只看下方 y=1；移除 y=1 水（setBlockType air 直接
//    清除该格水源）瞬间 y=1=air，睡莲同步收到 Down 更新自毁。即便相邻水 tick 后回填 y=1，睡莲已
//    自毁（自毁先于流动回填，因自毁是同步、流动是 tick 延迟）。
//
// 不测「水流到睡莲上破坏」（wiki :48）：水流水平流入睡莲格破坏睡莲，是 LiquidBlock 与睡莲的水平
// 交互，与下方支撑自毁不同路径，且水流动非确定时序，按「随机性/非确定不强测」准则跳过。
// 不测「船撞睡莲破坏」（:41）：依赖船实体移动碰撞，复杂且非确定，跳过。
//
// 跨服务端：睡莲无 state（仅 defaultState），下方水移除自毁行为与 vanilla 一致（同步），可跨
// 服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_睡莲.txt#破坏（附着方块被替换为非水时破坏）
// Ref: BushBlock.cpp（updatePostPlacement Down canSustain）+ LilyPadBlock.cpp（canSustain isLiquid）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除睡莲下方水时睡莲自毁变 air。
//
// 布局：(3,1,1) 放水（水源，作睡莲下方支撑 isLiquid true），(3,2,1) 放睡莲（在水上，强放绕过
// isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除水。air 放置向 Up 邻居睡莲派发
// updatePostPlacement(Down) → 下方 air isLiquid false → 返回 air，睡莲自毁。
//
// 判定：succeedWhenBlockPresent 断言睡莲格 (3,2,1) 睡莲消失（同 tick 同步，先于水流动回填）。
function lilyPadBreaksWhenWaterBelowRemoved(test: Test): void {
    // (3,1,1) 放水（水源，作睡莲下方支撑，isLiquid true）。setBlockType 放 minecraft:water 为水源。
    test.setBlockType("minecraft:water", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放睡莲（在水上方，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:lily_pad", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除水（water→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 睡莲派发 updatePostPlacement(Down) → 下方 air isLiquid false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言睡莲格 (3,2,1) 睡莲已自毁消失（同 tick 同步，先于相邻水流动回填）。
    test.succeedWhenBlockPresent("minecraft:lily_pad", { x: 3, y: 2, z: 1 }, false);
}

export function registerLilyPadTests(): void {
    GameTest.register("BlockBehaviorTests", "lily_pad_breaks_when_water_below_removed", lilyPadBreaksWhenWaterBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
