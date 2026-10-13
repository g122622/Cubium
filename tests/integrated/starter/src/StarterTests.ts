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

  test.succeedWhen(() => {
    const health = (chicken.getComponent("minecraft:health") as any).currentValue;
    test.assert(health <= 0, `fox has not killed chicken, hp=${health}`);
  });
  test.runAtTickTime(409, () => {
    test.assert(false, `fox failed to kill chicken: fox=${JSON.stringify(fox.location)}, ` +
      `chicken=${JSON.stringify(chicken.location)}, ` +
      `hp=${(chicken.getComponent("minecraft:health") as any).currentValue}`);
  });
}

GameTest.register("StarterTests", "simpleMobTest", simpleMobTest)
  .batch("night")
  .maxTicks(410)
  .structureName("startertests:mediumglass"); /* use the mediumglass.mcstructure file */
