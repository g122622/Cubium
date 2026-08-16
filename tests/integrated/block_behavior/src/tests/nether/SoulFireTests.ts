// 灵魂火（soul_fire）支撑自毁行为 GameTest。
//
// wiki tech_火.txt#灵魂火：灵魂火是下界火焰方块，只能生成/存在于灵魂沙或灵魂土上方（soul_fire_base_blocks
//   标签）。当下方支撑不再是灵魂沙/土时（被替换/破坏），灵魂火立即自毁为 air（不蔓延、不掉落）。
//   与普通火焰不同，灵魂火无 age/方向 state（状态数 1），伤害更高（2）。
//   - 灵魂火于 1.16 加入，1.21.11 已包含，属 vanilla 正式特性。
//
// C++ 链路：SoulFireBlock（nether/SoulFireBlock.cpp）继承 FireBlock，构造时用空 StateContainer 覆盖
//   FireBlock 的 6 属性容器，使状态数降为 1（无 age/方向连接，与 vanilla 一致）。
//   - isValidPosition（:51-59）：`belowState = world.getBlockState(pos.down())`，返回
//     `isSoulFireBase(&belowState->getBlock())`——下方方块须在 SOUL_FIRE_BASE_BLOCKS 标签
//     （BlockTags.cpp:1306-1308 含 soul_sand + soul_soil）。
//   - updatePostPlacement（:61-82）：忽略 facing/facingState，调 isValidPosition(state, world, currentPos)
//     失败则返回 BlockRegistry::airState()（自毁为 air），成功返回原 state。
//   - isSoulFireBase（:84-88）：查 BlockTags::SOUL_FIRE_BASE_BLOCKS().contains(block)。
//   - onBlockAdded 继承 FireBlock（FireBlock.cpp:246+）：仅检测下界传送门点燃，不自检 isValidPosition。
//   - GameTestHelper::setBlock（GameTestHelper.cpp:330-347）走 ServerWorld::setBlockState 直接写入，
//     不查 isValidPosition，故可强放灵魂火存活；放置只向 6 向邻居派发 updatePostPlacement/neighborChanged，
//     不向自身派发，故放置时不自检（同 SnowTests 范式）。
//
// 测试覆盖（2 个场景，覆盖 wiki 支撑自毁核心行为，双向闭合）：
//   1. 支撑存活：下方灵魂沙 → 灵魂火存活（isValidPosition 通过，不自毁），断言灵魂火仍在。
//   2. 支撑失效自毁：下方灵魂沙放灵魂火存活 → 替换下方为 stone（非 soul_fire_base_blocks）→
//      灵魂火经 updatePostPlacement(Down) 自毁为 air。
//
// 关键约束：
// 1. 灵魂火放置不查 isValidPosition（setBlockState 直写），故即使在 air 上方强放也能存活；放置不向
//    自身派发 updatePostPlacement，故不会放置即自检。需"第二步替换下方支撑"触发 Down 方向
//    updatePostPlacement 才自毁（同 SnowTests）。
// 2. 替换下方支撑必须是真实状态变化（soul_sand→stone 非 no-op）以派发邻居更新。stone→Up 邻居灵魂火
//    派发 updatePostPlacement(Down, stone) → isValidPosition(下方 stone)→ isSoulFireBase(stone)=false
//    → 返回 air，灵魂火自毁。
// 3. 场景 1 用 soul_sand 支撑（合法），灵魂火放置存活不断言自毁；为体现"存活"，断言灵魂火格仍是
//    soul_fire（succeedWhenBlockPresent true）。
// 4. ServerWorld 邻居更新（ServerWorld.cpp setBlockState flags 含 UPDATE_NEIGHBORS）遍历邻居调
//    updatePostPlacement(facing=opposite(neighbor.direction))。soul_sand→stone 写入向 Up 邻居灵魂火
//    派发 updatePostPlacement(Down) → 自毁。同 tick 同步。
// 5. 灵魂火无 state，自毁即 typeId 变 air，用 succeedWhenBlockPresent("minecraft:soul_fire", ..., false)
//    断言消失。
//
// 不测「灵魂火点燃实体/伤害」：onEntityCollision 走 doBlockCollisions（每 tick 实体碰撞），与 magma/
//   campfire 烫伤同构，本文件聚焦支撑自毁。TODO: 可补 soul_fire_damage_on_entity。
// 不测「灵魂火蔓延/熄灭 age 递减」：SoulFire 状态数 1 无 age，canBurn 返 false 不蔓延（:90-96），
//   无可测 state 变化。
// 不测「灵魂土作为支撑」：soul_soil 与 soul_sand 同在 SOUL_FIRE_BASE_BLOCKS 标签，行为同构（同一个
//   isSoulFireBase 判定），本组用 soul_sand 验证通用链路即可。TODO: 可补 soul_soil_supports_soul_fire。
//
// 跨服务端：soul_fire/soul_sand 方块名两端一致，SOUL_FIRE_BASE_BLOCKS 标签两端一致，支撑自毁行为与
//   vanilla 一致（下方非灵魂沙/土即自毁）。两端均可放 soul_sand+soul_fire，替换支撑行为两端可对比，
//   非 one-sided。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_火.txt#灵魂火（仅存于灵魂沙/土上方）
// Ref: SoulFireBlock.cpp（isValidPosition 下方须 soul_fire_base_blocks；updatePostPlacement 失败返 air）
// Ref: BlockTags.cpp（SOUL_FIRE_BASE_BLOCKS = {soul_sand, soul_soil}）
// Ref: FireBlock.cpp（onBlockAdded 仅传送门检测，不自检 isValidPosition）
// Ref: GameTestHelper.cpp（setBlock 走 setBlockState 直写，不查 isValidPosition）
// Ref: SnowTests.ts（支撑移除自毁范式：放置存活→替换支撑→Down 方向 updatePostPlacement 自毁）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 灵魂火 (3,2,1)，下方支撑 (3,1,1)（soul_sand 合法 / stone 失效）。

// 场景 1：支撑存活——下方灵魂沙 → 灵魂火存活（isValidPosition 通过，不自毁）。
//
// 布局：(3,1,1) soul_sand（合法支撑，在 SOUL_FIRE_BASE_BLOCKS 标签）+ (3,2,1) soul_fire。
// 灵魂火放置不向自身派发 updatePostPlacement（仅向 6 向邻居），故放置时不自检 isValidPosition，
// 存活。下方 soul_sand 合法，即使后续邻居更新触发 updatePostPlacement(Down) 也 isValidPosition 通过
// 不自毁。
//
// 判定：succeedWhenBlockPresent 断言灵魂火 (3,2,1) 仍在（soul_fire 存活，未自毁）。
function soulFireSurvivesOnSoulSand(test: Test): void {
    // (3,1,1) 放灵魂沙（合法支撑，soul_fire_base_blocks 标签成员）。
    test.setBlockType("minecraft:soul_sand", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放灵魂火（下方灵魂沙合法）。放置不向自身派发 updatePostPlacement，不自检，存活。
    test.setBlockType("minecraft:soul_fire", { x: 3, y: 2, z: 1 });

    // 断言灵魂火 (3,2,1) 仍在（支撑合法，未自毁）。
    test.succeedWhenBlockPresent("minecraft:soul_fire", { x: 3, y: 2, z: 1 }, true);
}

// 场景 2：支撑失效自毁——下方灵魂沙放灵魂火存活 → 替换下方为 stone → 灵魂火自毁为 air。
//
// 布局：(3,1,1) soul_sand + (3,2,1) soul_fire（先存活），再 (3,1,1) 设 stone 替换支撑。
// soul_sand→stone 真实状态变化（非 no-op）派发邻居更新 → 向 Up 邻居灵魂火派发
// updatePostPlacement(Down, stone) → SoulFireBlock::updatePostPlacement → isValidPosition(下方 stone)
// → isSoulFireBase(stone)=false（stone 不在 soul_fire_base_blocks 标签）→ 返回 air，灵魂火自毁。
// 同 tick 同步（updatePostPlacement 直接返回 air，ServerWorld 立即 setBlockState）。
//
// 判定：succeedWhenBlockPresent 断言灵魂火 (3,2,1) 已消失（自毁为 air）。
function soulFireBreaksWhenSupportReplacedWithStone(test: Test): void {
    // (3,1,1) 放灵魂沙 + (3,2,1) 放灵魂火（下方灵魂沙合法，存活）。
    test.setBlockType("minecraft:soul_sand", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:soul_fire", { x: 3, y: 2, z: 1 });

    // (3,1,1) 设 stone 替换灵魂沙支撑（soul_sand→stone 真实变化，派发邻居更新）。stone 不在
    // soul_fire_base_blocks 标签 → 灵魂火 updatePostPlacement(Down) → isValidPosition 失败 → 自毁 air。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // 断言灵魂火 (3,2,1) 已自毁消失（下方支撑失效，同 tick 同步自毁）。
    test.succeedWhenBlockPresent("minecraft:soul_fire", { x: 3, y: 2, z: 1 }, false);
}

export function registerSoulFireTests(): void {
    GameTest.register("BlockBehaviorTests", "soul_fire_survives_on_soul_sand", soulFireSurvivesOnSoulSand)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "soul_fire_breaks_when_support_replaced_with_stone", soulFireBreaksWhenSupportReplacedWithStone)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
