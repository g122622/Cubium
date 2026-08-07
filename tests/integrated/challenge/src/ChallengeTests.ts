// 挑战类 GameTest：minibiomes（矿车载猪滑行）、collapsing（实心空间坍缩）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertEntityInVolume } from "./utils/entity/assert.js";

// 矿车载猪：猪骑乘矿车沿铁轨滑行至终点，验证骑乘实体位置随载具同步。
function minibiomes(test: Test): void {
  const minecartEntityType = "minecraft:minecart";
  const pigEntityType = "minecraft:pig";

  const minecart = test.spawn(minecartEntityType, { x: 9, y: 7, z: 7 });
  const pig = test.spawn(pigEntityType, { x: 9, y: 7, z: 7 });

  test.setBlockType("minecraft:cobblestone", { x: 10, y: 7, z: 7 });

  const minecartRideableComp = minecart.getComponent("minecraft:rideable");
  if (!minecartRideableComp) {
    test.assert(false, "minecart has no rideable component");
    return;
  }

  minecartRideableComp.addRider(pig);

  test.succeedWhenEntityPresent(pigEntityType, { x: 5, y: 3, z: 1 }, true);
}

// 实心空间坍缩：按下按钮触发结构变化，验证区域内 zoglin 实体被清除。
function collapsing(test: Test): void {
  const zoglinEntityType = "minecraft:zoglin";
  const shulkerEntityType = "minecraft:shulker";

  for (let i = 0; i < 3; i++) {
    test.spawn(zoglinEntityType, { x: i + 2, y: 2, z: 3 });
    test.spawn(shulkerEntityType, { x: 4, y: 2, z: i + 2 });
  }

  test.pressButton({ x: 6, y: 8, z: 5 });

  test.succeedWhen(() => {
    assertEntityInVolume(test, zoglinEntityType, 0, 8, 0, 12, 12, 12);
  });
}

export function registerChallengeTests(): void {
  GameTest.register("ChallengeTests", "minibiomes", minibiomes).structureName("gametests:minibiomes").maxTicks(260);
  GameTest.register("ChallengeTests", "collapsing", collapsing)
    .structureName("gametests:collapsing_space")
    .maxTicks(260);
}
