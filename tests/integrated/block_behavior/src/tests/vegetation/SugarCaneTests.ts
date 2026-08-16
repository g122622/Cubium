// 甘蔗支撑自毁行为 GameTest（移除下方支撑方块时甘蔗被破坏）。
//
// wiki tech_甘蔗.txt#破坏（:45）："当甘蔗的位置变得不合适时，例如当移除支撑方块时，甘蔗会被
// 破坏并作为物品掉落。" 下方支撑方块列表（:54）含沙子/红沙/草方块/泥土等，且需毗邻水。
//
// C++ 链路：SugarCaneBlock::updatePostPlacement（SugarCaneBlock.cpp:129-151）仅当 facing==Down
// 时检查 isValidPosition（:88-112），失败则返回 air 自毁。isValidPosition 判定：下方是甘蔗 → true；
// 否则下方在 {grass_block, dirt, sand, red_sand} 且 _isNearWater（根部水平四向有水，:114-127）
// → true。移除下方支撑方块（sand→air）后，下方 air 既非甘蔗也非有效地面 → isValidPosition false
// → 自毁。反应同 tick 同步（updatePostPlacement 直接返回 air，ServerWorld 立即 setBlockState），
// 与 SnowBlock/CactusBlock 自毁链路一致。
//
// 关键约束（同 SnowTests）：
// 1. setBlockType 走 _resolveBlock 取 defaultState，不经 isValidPosition，故即使无水也能强放甘蔗。
//    放置自身不立即自毁（SugarCaneBlock 无 onBlockAdded 重写，放置不向自身派发 updatePostPlacement），
//    需"第二步移除下方支撑"触发甘蔗 Down 方向 updatePostPlacement 才自毁。
// 2. 移除支撑必须是"非 no-op 写入"——若下方原本就是 air，setBlockType("minecraft:air") 是 no-op
//    不派发邻居更新。故先显式铺 sand 支撑再放甘蔗，再设 air 移除支撑，保证 sand→air 是真实状态
//    变化（派发更新）。air 放置向 Up 邻居甘蔗派发 updatePostPlacement(Down) → isValidPosition 失败
//    （下方 air 非有效地面）→ 返回 air，甘蔗自毁。
// 3. 本测试不依赖水：强放绕过 isValidPosition，自毁判定只需移除 sand 后 isValidPosition 在 Down
//    检查时返回 false（下方 air 非甘蔗非有效地面，且 _isNearWater 在无水时也 false）。故无需布置水，
//    甘蔗自毁由"下方支撑失效"单一条件触发，与 wiki「移除支撑方块时破坏」直接对应。
//
// 不测「移除相邻水致甘蔗破坏」：wiki :45 明确该场景"不会立刻破坏，而在下次 PP更新或随机刻时破坏"，
// Cubium isValidPosition 仅在 Down 触发检查，水平移除水不触发甘蔗 updatePostPlacement（facing 非Down），
// 行为与 vanilla 的 PP更新/随机刻延迟不一致，按「不为 JE/BE 不一致或未明确行为写测试」准则跳过。
// 不测 randomTick 生长：概率性（random.nextInt(16)==0），非确定，按准则跳过。
// TODO: 待需要时可用高 randomTickSpeed + maxTicks 概率性测生长，但非确定性。
//
// 跨服务端：甘蔗 age state 两端一致（AGE_0_15），但本测试只断言方块变 air（核心自毁行为），不读
// state，可跨服务端对比（vanilla 移除下方支撑同步破坏，与 Cubium 一致）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_甘蔗.txt#破坏（移除支撑方块时甘蔗被破坏）
// Ref: SugarCaneBlock.cpp（updatePostPlacement/isValidPosition）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 移除甘蔗下方支撑方块（sand）时甘蔗自毁变 air。
//
// 布局：(3,1,1) 铺 sand 支撑，(3,2,1) 放甘蔗（在 sand 上，强放绕过 isValidPosition），再
// (3,1,1) 设 air 移除支撑。air 放置向 Up 邻居甘蔗派发 updatePostPlacement(Down) → 下方 air 非有效
// 地面 → isValidPosition false → 返回 air，甘蔗自毁。
//
// 判定：succeedWhenBlockPresent 断言甘蔗格 (3,2,1) 甘蔗消失（同 tick 同步）。
function sugarCaneBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 铺 sand 作甘蔗下方支撑（sand 在 isValidPosition 的有效地面集合内）。
    test.setBlockType("minecraft:sand", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放甘蔗（在 sand 上，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:sugar_cane", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（sand→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
    // 甘蔗派发 updatePostPlacement(Down) → 下方 air 非有效地面 → isValidPosition false → 返回 air。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言甘蔗格 (3,2,1) 甘蔗已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:sugar_cane", { x: 3, y: 2, z: 1 }, false);
}

export function registerSugarCaneTests(): void {
    GameTest.register("BlockBehaviorTests", "sugar_cane_breaks_when_support_below_removed", sugarCaneBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
