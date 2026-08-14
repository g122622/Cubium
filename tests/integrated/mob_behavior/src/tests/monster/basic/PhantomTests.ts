// 幻翼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 幻翼应从猫身边飞走，但会被猫"粘住"而不飞走（此为已知异常行为，标记 broken）。
function phantomsShouldFlyFromCats(test: Test): void {
  const catEntityType = "cat";
  const phantomEntityType = "phantom";

  test.spawn(catEntityType, { x: 4, y: 3, z: 3 });
  test.spawn(phantomEntityType, { x: 4, y: 3, z: 3 });

  test.succeedWhenEntityPresent(phantomEntityType, { x: 4, y: 6, z: 3 }, true); // 幻翼是否已飞至其所在列上方？
}

export function registerPhantomTests(): void {
  GameTest.register("MobBehaviorTests", "phantoms_should_fly_from_cats", phantomsShouldFlyFromCats)
    .structureName("gametests:glass_cells")
    .tag("suite:broken");
}
