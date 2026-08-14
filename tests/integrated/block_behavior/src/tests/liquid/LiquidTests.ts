// 液体方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 grass_block 地板，y=1..4 为 air。方块测试在 y=1 空气层操作。

// 熔岩源遇水凝固为黑曜石（wiki tech_熔岩.txt#流体交互：熔岩源流体接触水时凝固为黑曜石）。
//
// C++ 链路：LiquidBlock::reactWithNeighbors（LiquidBlock.cpp:194-259）由三处调用：
// onBlockAdded（:123）、neighborChanged（:142）、updatePostPlacement（:171）。当自身为熔岩源
// （fluidState->isSource()）且 6 向邻居含水流体时，将自身方块状态替换为 obsidian（:228-234）。
// water 放置（setBlockType flags=3）会向其 6 向邻居派发 neighborChanged，熔岩格作为水邻居
// 收到通知即在同 tick 同步执行 reactWithNeighbors → 变 obsidian。熔岩源（level=0）静止不流动，
// 流动需 scheduledTick 在下一 tick 才发生，而反应在同 tick 内已完成，故源格不会先流出 flowing_lava。
//
// 判定手段：先在 (3,1,3) 放熔岩源，再在相邻 (4,1,3) 放水源。水放置触发熔岩格 neighborChanged
// → reactWithNeighbors 检测到熔岩源 + 邻居水 → 变 obsidian。succeedWhenBlockPresent 断言
// (3,1,3) 处出现 obsidian 即通过。反应同 tick 同步完成，maxTicks 仅留调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熔岩.txt#流体交互（熔岩源遇水变黑曜石）
function lavaSourcePlusWaterMakesObsidian(test: Test): void {
  // 先放熔岩源（level=0 defaultState 即源）。下方 (3,0,3) 为 grass_block 支撑，熔岩源静止。
  test.setBlockType("minecraft:lava", { x: 3, y: 1, z: 3 });

  // 相邻 (4,1,3) 放水源。水放置（flags=3）向熔岩格派发 neighborChanged，触发 reactWithNeighbors
  // 同步将熔岩源格变 obsidian。
  test.setBlockType("minecraft:water", { x: 4, y: 1, z: 3 });

  // 断言熔岩源格已凝固为黑曜石。
  test.succeedWhenBlockPresent("minecraft:obsidian", { x: 3, y: 1, z: 3 }, true);
}

export function registerLiquidTests(): void {
  GameTest.register("BlockBehaviorTests", "lava_source_plus_water_makes_obsidian", lavaSourcePlusWaterMakesObsidian)
    .structureName("gametests:glass_pit")
    .maxTicks(100);
}
