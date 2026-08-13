// 僵尸行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { addFourNotchedWalls } from "../../../utils/block/build.js";

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

export function registerZombieTests(): void {
  GameTest.register("MobBehaviorTests", "zombie_villager_chase", zombieVillagerChase)
    .batch("night")
    .structureName("gametests:glass_pit")
    .maxTicks(2000);
}
