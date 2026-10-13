// 入门类 GameTest：simpleMobTest（狐狸追鸡）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 简单生物测试：生成狐狸与鸡，验证狐狸捕食鸡后鸡消失。
function simpleMobTest(test: Test): void {
  const foxId = "fox";
  const chickenId = "chicken";

  // 初始猎物就在近战范围内，避免首次寻路前的随机游走改变捕食前提。
  const fox = test.spawn(foxId, { x: 3, y: 2, z: 2 });
  const chicken = test.spawn(chickenId, { x: 2, y: 2, z: 2 });

  test.assertEntityPresentInArea(chickenId, true);

  const initialHealth = (chicken.getComponent("minecraft:health") as any).currentValue;
  let observedDamage = false;
  // 直接调度检查点，存活时继续等待；仅在最后一刻仍未满足时报告失败。
  for (let tick = 1; tick <= 410; tick++) {
    test.runAtTickTime(tick, () => {
      const health = (chicken.getComponent("minecraft:health") as any)?.currentValue as number | undefined;
      if (health !== undefined && health < initialHealth) {
        observedDamage = true;
      }
      if ((health !== undefined && health <= 0) || (health === undefined && observedDamage)) {
        test.succeed();
        return;
      }
      if (tick === 410) {
        test.assert(false, `fox failed to kill chicken: fox=${JSON.stringify(fox.location)}, ` +
          `chicken=${health === undefined ? "removed before observed damage" : JSON.stringify(chicken.location)}, ` +
          `hp=${health}`);
      }
    });
  }
}

GameTest.register("StarterTests", "simpleMobTest", simpleMobTest)
  .batch("night")
  .maxTicks(410)
  .structureName("startertests:mediumglass"); /* use the mediumglass.mcstructure file */
