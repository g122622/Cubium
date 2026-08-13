// 铁傀儡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

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

export function registerIronGolemTests(): void {
  GameTest.register("MobBehaviorTests", "iron_golem_arena", ironGolemArena)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(810);
}
