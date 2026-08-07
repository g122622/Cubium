// 生物行为类 GameTest：僵尸追村民、铁傀儡竞技场、Zoglin 浮空、幻翼避猫。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { addFourNotchedWalls } from "./utils/block/build.js";
import { assertEntityInVolume } from "./utils/entity/assert.js";

// 僵尸在有缺口的砖墙间追逐村民，验证僵尸寻路 AI。
function zombieVillagerChase(test: Test): void {
  const villagerType = "villager_v2";
  const zombieType = "zombie";

  addFourNotchedWalls(test, "minecraft:brick_block", 2, 1, 2, 4, 6, 4);

  test.spawn(villagerType, { x: 1, y: 3, z: 1 });
  test.spawn(zombieType, { x: 5, y: 3, z: 5 });

  test.runAtTickTime(180, () => {
    test.assertEntityPresentInArea(villagerType, true);
    test.succeed();
  });
}

// 铁傀儡的韧性测试：验证铁傀儡能击败骷髅和僵尸。
function ironGolemArena(test: Test): void {
  const ironGolemType = "iron_golem";
  const skeletonType = "skeleton";
  const zombieType = "zombie";

  test.spawn(ironGolemType, { x: 4, y: 3, z: 3 });
  test.spawn(skeletonType, { x: 5, y: 3, z: 5 });
  test.spawn(skeletonType, { x: 4, y: 3, z: 4 });
  test.spawn(skeletonType, { x: 3, y: 3, z: 3 });
  test.spawn(zombieType, { x: 4, y: 3, z: 6 });
  test.spawn(zombieType, { x: 3, y: 3, z: 5 });
  test.spawn(zombieType, { x: 2, y: 3, z: 4 });
  test.spawn(zombieType, { x: 5, y: 3, z: 2 });

  test.succeedWhen(() => {
    test.assertEntityPresentInArea(zombieType, false);
    test.assertEntityPresentInArea(skeletonType, false);
    test.assertEntityPresentInArea(ironGolemType, true);
  });
}

// Shulker 的攻击会使 Zoglin 浮空至笼顶。
function zoglinFloat(test: Test): void {
  const zoglinType = "zoglin";
  const shulkerType = "shulker";

  test.spawn(zoglinType, { x: 5, y: 2, z: 5 });
  test.spawn(shulkerType, { x: 2, y: 2, z: 2 });

  test.succeedWhen(() => {
    // zoglin 是否已浮至笼顶？
    assertEntityInVolume(test, zoglinType, 1, 7, 1, 10, 10, 10);
  });
}

// 幻翼应从猫身边飞走，但会被猫"粘住"而不飞走（此为已知异常行为，标记 broken）。
function phantomsShouldFlyFromCats(test: Test): void {
  const catEntityType = "cat";
  const phantomEntityType = "phantom";

  test.spawn(catEntityType, { x: 4, y: 3, z: 3 });
  test.spawn(phantomEntityType, { x: 4, y: 3, z: 3 });

  test.succeedWhenEntityPresent(phantomEntityType, { x: 4, y: 6, z: 3 }, true); // 幻翼是否已飞至其所在列上方？
}

export function registerMobBehaviorTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_chase", zombieVillagerChase)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(2000);

  GameTest.register("MobBehaviorTests", "iron_golem_arena", ironGolemArena)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(810);

  GameTest.register("MobBehaviorTests", "zoglin_float", zoglinFloat)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(210);

  GameTest.register("MobBehaviorTests", "phantoms_should_fly_from_cats", phantomsShouldFlyFromCats)
    .structureName("gametests:glass_cells")
    .tag("suite:broken");
}
