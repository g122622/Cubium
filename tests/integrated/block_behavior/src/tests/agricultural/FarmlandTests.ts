// 农田方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 grass_block 地板，y=1..4 为 air。方块测试在 y=1 空气层操作。

// 农田上方被不透明固体方块遮挡时退化为泥土（wiki tech_农田.txt#退化：农田上方放置固体方块会使其变回泥土）。
//
// C++ 链路：FarmlandBlock::updatePostPlacement（FarmlandBlock.cpp:115-137）当 facing==Up 且上方方块
// hasOpaqueCollisionShape() 为真时，scheduleBlockTick(currentPos, this, 1) 安排 1 tick 后的刻。
// 随后 tick（:141-149）再次确认上方有固体方块 → turnToDirt（:229-239）将自身方块状态替换为 dirt
// （flags=3）。stone 放置（setBlockType flags=3）向下方 farmland 格派发 neighborChanged +
// updatePostPlacement(Up)，farmland 收到 Up 方向更新即安排 1 tick 后退化。退化需 1 tick 延迟
// （scheduledTick），非同 tick 同步。
//
// 判定手段：先放 farmland，再在其正上方放 stone。stone 放置触发 farmland updatePostPlacement(Up)
// → 1 tick 后 turnToDirt 变 dirt。succeedWhen 持续轮询断言 farmland 格变为 dirt（轮询覆盖 1 tick
// 延迟窗口），maxTicks 留足调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_农田.txt#退化（上方固体变泥土）
function farmlandRevertsToDirtWhenSolidAbove(test: Test): void {
  // 放农田 (3,1,1)，下方 (3,0,1) 为 grass_block 支撑。setBlockType 直写 defaultState（moisture=0），
  // 不经 getStateForPlacement，放置本身不立即退化（退化靠上方放方块的 updatePostPlacement）。
  test.setBlockType("minecraft:farmland", { x: 3, y: 1, z: 1 });

  // 正上方 (3,2,1) 放 stone（Material::ROCK，hasOpaqueCollisionShape=true）。stone 放置向下方 farmland
  // 派发 updatePostPlacement(Up)，farmland 安排 1 tick 后 turnToDirt。
  test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });

  // 持续轮询断言农田格变为泥土（1 tick 延迟后成立）。
  test.succeedWhenBlockPresent("minecraft:dirt", { x: 3, y: 1, z: 1 }, true);
}

export function registerFarmlandTests(): void {
  GameTest.register("BlockBehaviorTests", "farmland_reverts_to_dirt_when_solid_above", farmlandRevertsToDirtWhenSolidAbove)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
}
