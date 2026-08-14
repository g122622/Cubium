// 冰雪类方块行为 GameTest（雪层等）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作，需特定支撑时显式 setBlockType 覆盖玻璃底。

// 雪层随其下方方块被破坏而破坏（wiki tech_雪.txt#用途：雪会随着其下方方块的破坏而破坏）。
//
// C++ 链路：SnowBlock::updatePostPlacement（SnowBlock.cpp:161-180）当 facing==Down 且
// !_canSurvive(world, currentPos) 时返回 air 自毁。_canSurvive（:182-206）检查下方方块：若下方
// 在 SNOW_LAYER_CANNOT_SURVIVE_ON 标签（冰/浮冰/屏障）则 false；在 SNOW_LAYER_CAN_SURVIVE_ON 标签
// （蜂蜜块/灵魂沙/泥巴）则 true；下方为满层(8)雪层则 true；否则看下方碰撞形状上面是否完全覆盖
// （Block::isFaceFull(getCollisionShape, Up)）。ServerWorld 邻居更新（ServerWorld.cpp:884-889）对
// 雪层格调 updatePostPlacement(opposite(Up)=Down, facingState=移除后的下方方块)。反应同 tick 同步
// （updatePostPlacement 直接返回 air，ServerWorld 立即 setBlockState）。
//
// 关键约束：
// 1. 放雪层自身不立即自毁——SnowBlock 无 onBlockAdded 重写（基类空操作），且放置只对 6 向邻居派发
//    updatePostPlacement/neighborChanged，不向自身派发。故即使在 air 上方强放雪层也能存活，需"第二步
//    移除下方支撑"触发雪层 Down 方向 updatePostPlacement 才自毁。
// 2. 移除支撑必须是"非 no-op 写入"——若下方原本就是 air，setBlockType("minecraft:air") 是 no-op，
//    不派发邻居更新，雪层收不到 Down 更新不自毁。故先显式铺 stone 支撑再放雪层，再设 air 移除支撑，
//    保证 stone→air 是真实状态变化（oldState!=newState，派发更新）。
// 3. 雪层支撑判定用下方方块的 isFaceFull(Up)：stone 是完整方块，isFaceFull=true 满足支撑；air 无碰撞
//    形状，isFaceFull=false，移除支撑后雪层自毁。
//
// 判定手段：先 (3,1,1) 铺 stone 支撑，(3,2,1) 放雪层（下方 stone 满足支撑，放置不自毁），再
// (3,1,1) 设 air 移除支撑。air 放置向 Up 邻居雪层派发 updatePostPlacement(Down) → _canSurvive 失败
// （下方 air 不满足 isFaceFull）→ 返回 air，雪层自毁。succeedWhenBlockPresent 断言雪层格 (3,2,1)
// 雪层消失（同 tick 同步成立）。
// 注意：雪层自毁掉落雪球依赖 onBlockRemoved/掉落系统，本测试仅断言方块变 air（核心行为），掉落物
// 未断言。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_雪.txt#用途（雪随下方方块破坏而破坏）
function snowBreaksWhenSupportBelowRemoved(test: Test): void {
  // (3,1,1) 铺 stone 作雪层下方支撑（完整方块，isFaceFull(Up)=true 满足雪层支撑）。
  test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

  // (3,2,1) 放雪层（下方 stone 满足支撑）。放置自身不立即自毁（无 onBlockAdded 重写，放置不向
  // 自身派发 updatePostPlacement）。
  test.setBlockType("minecraft:snow", { x: 3, y: 2, z: 1 });

  // (3,1,1) 设 air 移除支撑（stone→air 真实状态变化，非 no-op，派发邻居更新）。air 放置向 Up 邻居
  // 雪层派发 updatePostPlacement(Down) → 下方 air 不满足 isFaceFull → 返回 air，雪层自毁。
  test.setBlockType("minecraft:air", { x: 3, y: 1, z: 1 });

  // 断言雪层格 (3,2,1) 雪层已自毁消失（同 tick 同步）。
  test.succeedWhenBlockPresent("minecraft:snow", { x: 3, y: 2, z: 1 }, false);
}

export function registerSnowTests(): void {
  GameTest.register("BlockBehaviorTests", "snow_breaks_when_support_below_removed", snowBreaksWhenSupportBelowRemoved)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
