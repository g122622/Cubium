// 土径（dirt_path）方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 方块测试在内部 air 层操作，需特定支撑时显式 setBlockType 覆盖玻璃底。

// 土径上方被固体方块覆盖时退化为泥土（wiki tech_土径.txt#转变：土径被固体方块覆盖会变回泥土）。
//
// C++ 链路：DirtPathBlock::updatePostPlacement（DirtPathBlock.cpp）当 facing==Up 且
// isValidPosition（对应 vanilla canSurvive）为假时，scheduleBlockTick(currentPos, this, 1)
// 安排 1 tick 后的计划刻。随后 tick 再次确认不满足存活条件 → FarmlandBlock::turnToDirt
// 将自身方块状态替换为 dirt（flags=3）。stone 放置（setBlockType flags=3）向下方 dirt_path 格
// 派发 neighborChanged + updatePostPlacement(Up)，dirt_path 收到 Up 方向更新即安排 1 tick 后退化。
// 退化需 1 tick 延迟（scheduledTick），非同 tick 同步。
//
// 判定手段：先放 dirt_path，再在其正上方放 stone。stone 放置触发 dirt_path updatePostPlacement(Up)
// → 1 tick 后 turnToDirt 变 dirt。succeedWhen 持续轮询断言 dirt_path 格变为 dirt（轮询覆盖 1 tick
// 延迟窗口），maxTicks 留足调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_土径.txt#转变（上方固体变泥土）
function dirtPathRevertsToDirtWhenSolidAbove(test: Test): void {
  // 放土径 (3,1,1)，下方 (3,0,1) 为 glass_pit 玻璃底（支撑与否不影响退化，退化只看上方固体）。
  // setBlockType 直写 defaultState，不经 getStateForPlacement，放置本身不立即退化
  // （退化靠上方放方块的 updatePostPlacement）。
  test.setBlockType("minecraft:dirt_path", { x: 3, y: 1, z: 1 });

  // 正上方 (3,2,1) 放 stone。stone 放置向下方 dirt_path 派发 updatePostPlacement(Up)，
  // dirt_path 安排 1 tick 后 turnToDirt。
  test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });

  // 持续轮询断言土径格变为泥土（1 tick 延迟后成立）。
  test.succeedWhenBlockPresent("minecraft:dirt", { x: 3, y: 1, z: 1 }, true);
}

// 土径上方为栅栏门时不退化（vanilla DirtPathBlock.canSurvive 显式放行栅栏门）。
//
// C++ 链路：DirtPathBlock::isValidPosition 对上方为 FenceGateBlock 的情形返回 true，
// 故 updatePostPlacement(Up) 不安排计划刻，土径保持原样。
//
// 判定手段：放 dirt_path，再在其正上方放 oak_fence_gate。等待若干 tick 后断言 dirt_path 格仍为
// dirt_path（未被栅栏门触发退化）。用 runAtTickTime 在足够晚的 tick 断言。
// Ref: DirtPathBlock.java:69（!blockstate.isSolid() || blockstate.getBlock() instanceof FenceGateBlock）
function dirtPathSurvivesUnderFenceGate(test: Test): void {
  test.setBlockType("minecraft:dirt_path", { x: 3, y: 1, z: 1 });
  // 上方放栅栏门（非固体，vanilla 例外放行）。
  test.setBlockType("minecraft:oak_fence_gate", { x: 3, y: 2, z: 1 });

  // 等待足够 tick（>1 tick 计划刻窗口）后断言土径未被退化。
  test.runAtTickTime(40, () => {
    const block = test.getBlock({ x: 3, y: 1, z: 1 }) as unknown as { typeId?: string } | undefined;
    test.assert(
      block?.typeId === "minecraft:dirt_path",
      `dirt_path should survive under fence gate, got ${block?.typeId}`,
    );
    test.succeed();
  });
}

export function registerDirtPathTests(): void {
  GameTest.register("BlockBehaviorTests", "dirt_path_reverts_to_dirt_when_solid_above", dirtPathRevertsToDirtWhenSolidAbove)
    .structureName("gametests:glass_pit")
    .maxTicks(60);
  GameTest.register("BlockBehaviorTests", "dirt_path_survives_under_fence_gate", dirtPathSurvivesUnderFenceGate)
    .structureName("gametests:glass_pit")
    .maxTicks(80);
}
