// 珊瑚方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 grass_block 地板，y=1..4 为 air。方块测试在 y=1 空气层操作。

// 珊瑚扇脱水死亡变为死珊瑚扇（wiki tech_珊瑚.txt#脱水：珊瑚离开水后会死亡变为死珊瑚）。
//
// C++ 链路：CoralFanBlock::updatePostPlacement（coral/CoralBlock.cpp:201-229）当 hasNearbyWater（:45-60，
// 检查 6 向邻居流体）为 false 时返回 m_deadBlock 对应的死珊瑚扇状态，ServerWorld 随即将该格替换为死珊瑚扇。
// 珊瑚扇无 onBlockAdded 重写（基类 Block::onBlockAdded 空操作），放置自身不触发死亡。且 ServerWorld
// 放方块只对 6 向邻居派发 updatePostPlacement，新方块自身只触发 onBlockAdded，故珊瑚扇死亡必须靠
// "第二步在相邻格放方块"触发其 updatePostPlacement。
//
// 关键约束：CoralFanBlock::updatePostPlacement 的 Down 分支（:212-216）先于死亡检查——若下方无固体支撑
// （canAttachTo(Down) 为 false）则返回 air 自毁而非死珊瑚。故珊瑚扇必须放在固体上方。glass_pit y=1 下方
// y=0 是 grass_block，满足支撑。
//
// 判定手段：先在 (3,1,1) 放 tube_coral_fan（下方 grass_block 支撑，周围无水但放置自身不死），再在相邻
// (4,1,1) 放 stone。stone 放置向珊瑚扇格派发 updatePostPlacement → 无水 → 返回 dead_tube_coral_fan。
// succeedWhenBlockPresent 断言珊瑚扇格出现 dead_tube_coral_fan。反应同 tick 同步，maxTicks 留调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_珊瑚.txt#脱水（离水死亡变死珊瑚）
function coralFanDiesWhenNoWaterNearby(test: Test): void {
  // 放管状珊瑚扇 (3,1,1)，下方 (3,0,1) 为 grass_block 支撑（CoralFanBlock 要求下方固体，否则自毁）。
  // 周围无水，但放置自身不触发死亡（无 onBlockAdded 重写，且放置不向自身派发 updatePostPlacement）。
  test.setBlockType("minecraft:tube_coral_fan", { x: 3, y: 1, z: 1 });

  // 相邻 (4,1,1) 放 stone。stone 放置向珊瑚扇格派发 updatePostPlacement → hasNearbyWater=false
  // → 返回 dead_tube_coral_fan，珊瑚扇格被替换为死珊瑚扇。
  test.setBlockType("minecraft:stone", { x: 4, y: 1, z: 1 });

  // 断言珊瑚扇格已脱水死亡为死珊瑚扇。
  test.succeedWhenBlockPresent("minecraft:dead_tube_coral_fan", { x: 3, y: 1, z: 1 }, true);
}

export function registerCoralTests(): void {
  GameTest.register("BlockBehaviorTests", "coral_fan_dies_when_no_water_nearby", coralFanDiesWhenNoWaterNearby)
    .structureName("gametests:glass_pit")
    .maxTicks(100);
}
