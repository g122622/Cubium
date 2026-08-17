// 地毯（carpet）下方支撑失效自毁行为 GameTest。
//
// wiki tech_羊毛.txt#地毯：地毯是 1/16 格高的薄装饰方块，须放在非空气方块上方。当下方方块被移除
//   （变空气）时，地毯立即自毁为 air（不掉落，因无碰撞箱）。与雪层/灵魂火类似，但地毯支撑判定更
//   宽松：下方只要非空气即支撑（含水、火、草等非固体方块也支撑），不要求 isSolid。
//   - 地毯于 1.6 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：CarpetBlock（decorative/CarpetBlock.cpp）继承 Block，无 state（纯 typeId 方块）。
//   - isValidPosition（:55-63）：`belowState = world.getBlockState(pos.down())`，返回
//     `belowState != nullptr && !belowState->isAir()`——下方非空气即合法（比 isSolid 宽松）。
//   - updatePostPlacement（:65-83）：`facing == Direction::Down` 时取 belowState，若为空或 isAir()
//     返回 `VanillaBlocks::AIR->defaultState()`（自毁为 air），否则返回原 state。
//   - GameTestHelper::setBlock 走 ServerWorld::setBlockState 直写不查 isValidPosition，故可强放地毯
//     存活；放置只向 6 向邻居派发 updatePostPlacement/neighborChanged，不向自身派发，故放置时不自检
//     （同 SnowTests/SoulFire 范式）。需第二步替换/移除下方支撑触发 Down 方向 updatePostPlacement 自毁。
//
// 测试覆盖（2 个场景，覆盖 wiki 支撑自毁核心行为，双向闭合）：
//   1. 支撑存活：下方 stone → 地毯存活（isValidPosition 通过，不自毁），断言地毯仍在。
//   2. 支撑失效自毁：下方 stone 放地毯存活 → 下方设 air 移除支撑 → 地毯经 updatePostPlacement(Down)
//      自毁为 air。
//
// 关键约束：
// 1. 地毯放置不查 isValidPosition（setBlockState 直写），故即使在 air 上方强放也能存活；放置不向
//    自身派发 updatePostPlacement，故不会放置即自检。需"第二步移除下方支撑"触发 Down 方向
//    updatePostPlacement 才自毁（同 SnowTests/SoulFire）。
// 2. 移除下方支撑必须是真实状态变化（stone→air 非 no-op）以派发邻居更新。stone→air 写入向 Up 邻居
//    地毯派发 updatePostPlacement(Down, air) → belowState isAir() → 返回 air，地毯自毁。同 tick 同步。
// 3. 地毯无 state，自毁即 typeId 变 air，用 succeedWhenBlockPresent("minecraft:white_carpet", ..., false)
//    断言消失。
// 4. 场景 1 用 stone 支撑（合法），地毯放置存活不断言自毁；为体现"存活"，断言地毯格仍是 white_carpet。
//
// 不测「地毯下方非固体非空气方块也支撑（如水/火）」：需铺水/火方块，water 流体 tick + fire 随机
//   熄灭增加复杂度，本文件聚焦 stone 支撑的确定路径。TODO: 可补 carpet_survives_on_non_solid_non_air。
// 不测「地毯阻燃特性」：羊毛地毯有阻燃（不被火点燃），涉 fire 蔓延随机，跳过。
//
// 跨服务端：white_carpet 方块名两端一致，下方支撑失效自毁行为与 vanilla 一致（下方变空气即自毁）。
//   两端均可放 stone+carpet，移除支撑行为两端可对比，非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_羊毛.txt#地毯（须放非空气上方，下方移除自毁）
// Ref: CarpetBlock.cpp（isValidPosition 下方非空气即支撑；updatePostPlacement Down+isAir 返 air 自毁）
// Ref: SnowTests.ts / SoulFireTests.ts（支撑移除自毁范式：放置存活→移除支撑→Down 方向 updatePostPlacement 自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 地毯 (3,2,1)，下方支撑 (3,1,1)（stone 合法 / air 失效）。

// 场景 1：支撑存活——下方 stone → 地毯存活（isValidPosition 通过，不自毁）。
//
// 布局：(3,1,1) stone（合法支撑，非空气）+ (3,2,1) white_carpet。
// 地毯放置不向自身派发 updatePostPlacement（仅向 6 向邻居），故放置时不自检 isValidPosition，
// 存活。下方 stone 非空气合法，即使后续邻居更新触发 updatePostPlacement(Down) 也 isValidPosition
// 通过不自毁。
//
// 判定：succeedWhenBlockPresent 断言地毯 (3,2,1) 仍在（white_carpet 存活，未自毁）。
function carpetSurvivesOnStone(test: Test): void {
    // (3,1,1) 放 stone（合法支撑，非空气满足 isValidPosition）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放白地毯（下方 stone 非空气合法）。放置不向自身派发 updatePostPlacement，不自检，存活。
    test.setBlockType("minecraft:white_carpet", { x: 3, y: 2, z: 1 });

    // 断言地毯 (3,2,1) 仍在（支撑合法，未自毁）。
    test.succeedWhenBlockPresent("minecraft:white_carpet", { x: 3, y: 2, z: 1 }, true);
}

// 场景 2：支撑失效自毁——下方 stone 放地毯存活 → 下方设 air 移除支撑 → 地毯自毁为 air。
//
// 布局：(3,1,1) stone + (3,2,1) white_carpet（先存活），再 (3,1,1) 设 air 移除支撑。
// stone→air 真实状态变化（非 no-op）派发邻居更新 → 向 Up 邻接地毯派发
// updatePostPlacement(Down, air) → CarpetBlock::updatePostPlacement → belowState isAir() →
// 返回 air，地毯自毁。同 tick 同步（updatePostPlacement 直接返回 air，ServerWorld 立即 setBlockState）。
//
// 判定：succeedWhenBlockPresent 断言地毯 (3,2,1) 已消失（自毁为 air）。
function carpetBreaksWhenSupportBelowRemoved(test: Test): void {
    // (3,1,1) 放 stone + (3,2,1) 放白地毯（下方 stone 非空气合法，存活）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:white_carpet", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 air 移除支撑（stone→air 真实变化，派发邻居更新）。air 放置向 Up 邻接地毯派发
    // updatePostPlacement(Down) → 下方 air isAir() → 返回 air，地毯自毁。
    test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

    // 断言地毯 (3,2,1) 已自毁消失（下方支撑失效，同 tick 同步自毁）。
    test.succeedWhenBlockPresent("minecraft:white_carpet", { x: 3, y: 2, z: 1 }, false);
}

export function registerCarpetTests(): void {
    GameTest.register("BlockBehaviorTests", "carpet_survives_on_stone", carpetSurvivesOnStone)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "carpet_breaks_when_support_below_removed", carpetBreaksWhenSupportBelowRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
