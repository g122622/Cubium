// 混凝土粉末方块行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 grass_block 地板，y=1..4 为 air。方块测试在 y=1 空气层操作。

// 混凝土粉末接触水凝固为混凝土（wiki tech_混凝土粉末.txt#固化：混凝土粉末接触水时凝固为对应颜色的混凝土）。
//
// C++ 链路：ConcretePowderBlock::updatePostPlacement（ConcretePowderBlock.cpp:52-64）当 touchesLiquid
// （:90-105，6 向邻居的 BlockState 含水流体 canSolidify）为真时，返回 m_concrete->defaultState()，
// ServerWorld 随即将该格方块状态替换为对应混凝土（ServerWorld.cpp:903-904）。m_concrete 在注册时
// 由同色 concrete Block 指针传入（ColoredBlocks.cpp，white_concrete_powder → WHITE_CONCRETE）。
// 水放置（setBlockType flags=3）向粉末格派发 updatePostPlacement，粉末检测到邻居水即同 tick 同步凝固。
// 混凝土粉末无通用型 typeId，只有 16 色具体方块（white_concrete_powder 等），故测试用白色变种。
//
// 判定手段：先放白色混凝土粉末，再在相邻格放水。水放置触发粉末 updatePostPlacement → touchesLiquid
// 为真 → 凝固为 white_concrete。succeedWhenBlockPresent 断言粉末格出现 white_concrete 即通过。
// 反应同 tick 同步完成，maxTicks 仅留调度余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_混凝土粉末.txt#固化（接触水变混凝土）
function concretePowderSolidifiesOnWaterContact(test: Test): void {
  // 放白色混凝土粉末 (3,1,1)，下方 (3,0,1) 为 grass_block 支撑。setBlockType 直写不经
  // getStateForPlacement，周围无水时不立即凝固。
  test.setBlockType("minecraft:white_concrete_powder", { x: 3, y: 1, z: 1 });

  // 相邻 (4,1,1) 放水源。水放置向粉末格派发 updatePostPlacement，粉末检测到邻居水即凝固为
  // white_concrete。
  test.setBlockType("minecraft:water", { x: 4, y: 1, z: 1 });

  // 断言粉末格已凝固为白色混凝土。
  test.succeedWhenBlockPresent("minecraft:white_concrete", { x: 3, y: 1, z: 1 }, true);
}

export function registerConcretePowderTests(): void {
  GameTest.register("BlockBehaviorTests", "concrete_powder_solidifies_on_water_contact", concretePowderSolidifiesOnWaterContact)
    .structureName("gametests:glass_pit")
    .maxTicks(100);
}
