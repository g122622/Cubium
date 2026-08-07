// 入门类 GameTest：simpleMobTest（狐狸追鸡）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 简单生物测试：生成狐狸与鸡，验证狐狸捕食鸡后鸡消失。
function simpleMobTest(test: Test): void {
  const foxId = "fox";
  const chickenId = "chicken";

  test.spawn(foxId, { x: 5, y: 2, z: 5 });
  test.spawn(chickenId, { x: 2, y: 2, z: 2 });

  test.assertEntityPresentInArea(chickenId, true);

  test.succeedWhen(() => {
    test.assertEntityPresentInArea(chickenId, false);
  });
}

GameTest.register("StarterTests", "simpleMobTest", simpleMobTest)
  .maxTicks(410)
  .structureName("startertests:mediumglass"); /* use the mediumglass.mcstructure file */
