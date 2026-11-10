// 铜傀儡建造行为 GameTest。
//
// wiki tech_铜傀儡.txt#创建：南瓜 + 铜块（垂直 2 格），最后放南瓜触发建造。
// 对应 MC 1.21.11 CarvedPumpkinBlock.trySpawnGolem 的铜傀儡分支：
//   copperGolemFullPattern = aisle("^", "#")，'^' 为雕刻南瓜/南瓜灯，'#' 为 BlockTags.COPPER。
//   匹配后 _spawnCopperGolem：清除图案方块 → 在南瓜位置生成 copper_golem →
//   铜块位置替换为对应氧化等级的铜箱子 → CopperGolem.spawn(weatherState) 设置氧化等级。
//
// 结构与坐标：glass_pit（7×5×7）。铜块 (3,2,3)，南瓜 (3,3,3)。
// 判定：建造后 copper_golem 出现；铜块位置变为对应氧化等级的铜箱子。
// 区域限定，排除并行测试污染。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

function readCopperGolems(test: Test): unknown[] {
  return test.getDimension().getEntities({
    type: "copper_golem",
    location: test.worldLocation(PIT_FROM),
    volume: PIT_VOLUME,
  });
}

// 基础铜块建造：南瓜 + copper_block，最后放南瓜触发。
// 铜块位置应被替换为 copper_chest，傀儡氧化等级为 Unaffected。
function copperGolemBuiltByPlayer(test: Test): void {
  test.setBlockType("minecraft:copper_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 3, z: 3 });

  test.succeedWhen(() => {
    const golems = readCopperGolems(test);
    test.assert(golems.length >= 1, `copper_golem not built, count=${golems.length}`);
    const chest = test.getBlock({ x: 3, y: 2, z: 3 });
    const actualId = (chest as any)?.typeId;
    test.assert(
      actualId === "minecraft:copper_chest",
      `copper_block should be replaced by copper_chest after building copper_golem, actual=${actualId}`,
    );
  });
}

// 氧化铜块建造：exposed_copper → exposed_copper_chest，傀儡继承 Exposed 氧化等级。
function copperGolemBuiltFromExposedCopper(test: Test): void {
  test.setBlockType("minecraft:exposed_copper", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 3, z: 3 });

  test.succeedWhen(() => {
    const golems = readCopperGolems(test);
    test.assert(golems.length >= 1, `exposed copper_golem not built, count=${golems.length}`);
    const chest = test.getBlock({ x: 3, y: 2, z: 3 });
    const actualId = (chest as any)?.typeId;
    test.assert(
      actualId === "minecraft:exposed_copper_chest",
      `exposed_copper should be replaced by exposed_copper_chest, actual=${actualId}`,
    );
  });
}

// 负例：南瓜 + 铁块不满足铜傀儡图案，不应生成任何傀儡。
// 铁块单独不触发铁傀儡（需要 T 形），因此预期区域内无 copper_golem / iron_golem。
function copperGolemNotBuiltWithWrongBlock(test: Test): void {
  test.setBlockType("minecraft:iron_block", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:carved_pumpkin", { x: 3, y: 3, z: 3 });

  // 建造检测是同步的，留 40 tick 覆盖可能的延迟生成路径后断言无傀儡。
  test.runAtTickTime(40, () => {
    const golems = readCopperGolems(test);
    test.assert(golems.length === 0, `copper_golem should not be built with iron_block, count=${golems.length}`);
    test.succeed();
  });
}

export function registerCopperGolemBuildTests(): void {
  GameTest.register("BlockBehaviorTests", "copper_golem_built_by_player", copperGolemBuiltByPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("BlockBehaviorTests", "copper_golem_built_from_exposed_copper", copperGolemBuiltFromExposedCopper)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("BlockBehaviorTests", "copper_golem_not_built_with_wrong_block", copperGolemNotBuiltWithWrongBlock)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}