// 海带下方支撑自毁行为 GameTest（移除下方支撑方块时海带自毁）。
//
// wiki tech_海带.txt（:41）："海带可以被放置于上表面方块支撑形状完整的方块上（岩浆块除外）。"
// :45 "破坏海带植株会摧毁其上方所有海带植株和海带。" 即海带下方支撑方块被移除时，海带失去支撑
// 自毁（vanilla 经 neighborChanged 链）。
//
// C++ 链路：KelpBlock::updatePostPlacement（KelpBlock.cpp:117-139）当 facing==Down 时检查
// isValidPosition（:92-115）。isValidPosition 判定下方方块是否为同类海带 / KELP_PLANT / isSolid 固体。
// 移除下方支撑（→air）后，下方 air isSolid=false 且非同类 → isValidPosition 失败 → 返回 air 自毁。
// 反应同 tick 同步（updatePostPlacement 直接返回 air）。
//
// 关键约束（同 SugarCaneTests/TallGrassTests 支撑自毁范式）：
// 1. setBlockType 走 _resolveBlock 取 defaultState（age=0），不经 isValidPosition，故即使下方非水/非
//    固体也能强放海带。海带无 onBlockAdded 重写，放置不向自身派发 updatePostPlacement，强放不立即
//    自毁。需"第二步移除下方支撑"触发海带 Down 方向 updatePostPlacement 才自毁。
// 2. 移除支撑必须是非 no-op 写入——先显式铺 stone 支撑再放海带，再设 air 移除支撑，保证 stone→air
//    真实状态变化派发更新。air 放置向 Up 邻居海带派发 updatePostPlacement(Down) → 下方 air isSolid
//    false → isValidPosition 失败 → 返回 air，海带自毁。
//
// 不测「骨粉海带生长一格」（wiki :63）：调研确认 Cubium KelpBlock 未实现 IGrowable 接口（无
// canGrow/canUseBonemeal/grow），BoneMealItem 经 dynamic_cast<IGrowable*> 检查会跳过海带，骨粉对
// 海带无效。按「不为未实现行为写测试」准则跳过。
//   TODO: 待 Cubium 为 KelpBlock 实现 IGrowable（骨粉生长一格）后补充骨粉测试。
// 不测 randomTick 向上生长（:141-165，14% 概率）：概率性，跳过。
// 不测「海带在水中」语义：Cubium 强放绕过 isValidPosition，且 updatePostPlacement 只检查下方支撑
// （与水无关），符合支撑自毁核心行为点，不为 JE/BE 水语义差异写测试。
//
// 跨服务端：海带 age state 名两端一致（age 0-25），支撑自毁行为与 vanilla 一致（下方支撑失效即
// 破坏，同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_海带.txt（海带放置在支撑形状完整方块上）
// Ref: KelpBlock.cpp（updatePostPlacement Down isValidPosition 支撑自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除海带下方支撑方块（石头）时海带自毁变 air。
//
// 布局：(3,1,1) 铺 stone 作海带下方支撑（isSolid true，isValidPosition 满足），(3,2,1) 放海带
// （在 stone 上，强放绕过 isValidPosition 不立即自毁），再 (3,1,1) 设 air 移除支撑。
// air 放置向 Up 邻居海带派发 updatePostPlacement(Down) → 下方 air isSolid false → isValidPosition
// 失败 → 返回 air。
//
// 判定：succeedWhenBlockPresent 断言海带格 (3,2,1) 海带消失（同 tick 同步）。
function kelpBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 stone 作海带下方支撑（isSolid=true，isValidPosition 下方固体满足）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放海带（在 stone 上，强放绕过 isValidPosition，不立即自毁）。setBlockType 取 defaultState age=0。
    test.setBlockType("minecraft:kelp", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 海带派发 updatePostPlacement(Down) → 下方 air isSolid false → isValidPosition 失败 → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言海带格 (3,2,1) 海带已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:kelp", { x: 3, y: 2, z: 1 }, false);
}

export function registerKelpTests(): void {
    GameTest.register("BlockBehaviorTests", "kelp_breaks_when_support_below_removed", kelpBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
