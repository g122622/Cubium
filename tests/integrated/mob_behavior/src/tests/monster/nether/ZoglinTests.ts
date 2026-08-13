// 疣猪兽（Zoglin）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

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

export function registerZoglinTests(): void {
  GameTest.register("MobBehaviorTests", "zoglin_float", zoglinFloat)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(210);
}
