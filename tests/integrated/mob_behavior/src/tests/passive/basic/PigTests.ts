// 猪行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 猪被闪电击中后转化为僵尸猪灵。
// 闪电首个 tick 即对命中范围内实体造成 5 点伤害并调用 onStruckByLightning；
// 猪 10 血存活，转化在 spawn 当 tick 完成。和平难度下不转化（须非 Peaceful）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪.txt#行为
function pigLightningStrike(test: Test): void {
  const pigType = "pig";
  const lightningType = "lightning_bolt";
  const zombifiedPiglinType = "zombified_piglin";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // glass_pit 的 (x=2,z=1) 列结构内：y=2 为实心玻璃隔板，y=3 为空气（脚下玻璃可站立），y=4 为空气。
  // 故猪站结构内 y=3 → helper-y=4，上方 y=4 空气留出 2 格高空间。
  // 猪与闪电同格 spawn，闪电 ±3 XZ 命中范围必覆盖猪。
  test.spawn(pigType, { x: 2, y: 4, z: 1 });
  test.spawn(lightningType, { x: 2, y: 4, z: 1 });

  // 闪电转化在首 tick 触发：僵尸猪灵出现且原猪消失即通过。
  test.succeedWhen(() => {
    test.assertEntityPresentInArea(zombifiedPiglinType, true);
    test.assertEntityPresentInArea(pigType, false);
  });
}

export function registerPigTests(): void {
  GameTest.register("MobBehaviorTests", "pig_lightning_strike", pigLightningStrike)
    .structureName("gametests:glass_pit")
    .maxTicks(200);
}


