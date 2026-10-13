// 入门类 GameTest：simpleMobTest（狐狸追鸡）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 简单生物测试：生成狐狸与鸡，验证狐狸捕食鸡后鸡消失。
function simpleMobTest(test: Test): void {
  const foxId = "fox";
  const chickenId = "chicken";

  // 开放坑的边缘不能保留被击退的猎物；围墙仅限制边界，内部仍可正常寻路和攻击。
  for (let y = 1; y <= 3; y++) {
    for (let edge = 0; edge < 7; edge++) {
      for (const pos of [
        { x: edge, y, z: 0 }, { x: edge, y, z: 6 },
        { x: 0, y, z: edge }, { x: 6, y, z: edge },
      ]) {
        test.setBlockType("minecraft:stone", pos);
      }
    }
  }

  // 初始猎物就在近战范围内，避免首次寻路前的随机游走改变捕食前提。
  const fox = test.spawn(foxId, { x: 3, y: 2, z: 2 });
  const chicken = test.spawn(chickenId, { x: 2, y: 2, z: 2 });
  (chicken.getComponent("minecraft:movement") as any).setCurrentValue(0);

  test.assertEntityPresentInArea(chickenId, true);

  const initialHealth = (chicken.getComponent("minecraft:health") as any).currentValue;
  let observedDamage = false;
  // 直接调度检查点，存活时继续等待；仅在最后一刻仍未满足时报告失败。
  for (let tick = 1; tick <= 1200; tick++) {
    test.runAtTickTime(tick, () => {
      const health = (chicken.getComponent("minecraft:health") as any)?.currentValue as number | undefined;
      if (health !== undefined && health < initialHealth) {
        observedDamage = true;
      }
      if ((health !== undefined && health <= 0) || (health === undefined && observedDamage)) {
        test.succeed();
        return;
      }
      if (tick === 1200) {
        test.assert(false, `fox failed to kill chicken: fox=${JSON.stringify(fox.location)}, ` +
          `chicken=${health === undefined ? "removed before observed damage" : JSON.stringify(chicken.location)}, ` +
          `hp=${health}`);
      }
    });
  }
}

GameTest.register("StarterTests", "simpleMobTest", simpleMobTest)
  .batch("night_starter_fox_hunts_chicken")
  .skyAccess(true)
  .maxTicks(1200)
  .structureName("gametests:creeper_pit");
