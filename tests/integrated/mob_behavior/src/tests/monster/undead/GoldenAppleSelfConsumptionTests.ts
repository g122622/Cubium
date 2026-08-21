// 金苹果玩家自己食用效果对齐测试（验证 GoldenAppleItem::onItemRightClick setActiveHand 修复 +
// onItemUseFinish 效果应用链路，区别喂僵尸村民的 itemInteractionForEntity 路径）。
//
// 验证 GoldenAppleItem::onItemRightClick 修复（补 setActiveHand）解锁的金苹果自己食用链路。
// GoldenAppleItem::onItemRightClick 此前 canEat 分支仅返回 Consume 未调 setActiveHand（同 FoodItem
// 同源缺陷），玩家自己吃金苹果 onItemUseFinish 永不触发（抗性/生命恢复效果不生效）。已补 setActiveHand。
// 区别 GoldenAppleConsumptionTests（喂僵尸村民走 itemInteractionForEntity，不经 setActiveHand）。
//
// 金苹果 canAlwaysEat（setAlwaysEdible，不需饥饿），效果确定性 probability=1.0：
//   - Regeneration II 100 tick（生命恢复）
//   - Absorption 2400 tick（吸收）
// 金苹果无容器物品（hasContainerItem=false），shrink 后 stack 空，返回空 → 主手变空。
//
// 环境：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家 (1,2,3) 持1金苹果（slot 0 主手）。canAlwaysEat 不需 setFoodLevel。
//
// 时序：tick 5 useItem(金苹果) → GoldenAppleItem::onItemRightClick（canEat true）→ setActiveHand
// （useDuration=32，继承基础 Item isFood 分支或 FoodItem）。tick 5+32+余量 ≈ 50 食用完成 →
// onItemUseFinish → addStats + addEffect(Regeneration/Absorption) + shrink(1) stack 空返回空。
//
// 判定手段（双重断言）：
//   1. 玩家获得 regeneration 效果（getEffect("regeneration") 存在，概率1.0 确定性）；
//   2. 主手变空（金苹果无容器物品，shrink 后空返回空 stack）。
// Survival 模式（创造跳过 shrink 无消耗证据）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const GOLDEN_APPLE = "minecraft:golden_apple";

function getMainHandTypeId(player: any): string {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.typeId ?? "";
}

// 金苹果自己食用获得再生效果 + 消耗：Survival 玩家持1金苹果食用，断言 regeneration 出现 + 主手空。
function goldenAppleSelfConsumptionTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持1金苹果（slot 0 主手）。canAlwaysEat 不需 setFoodLevel。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "eater", 0 as any);
  const apple = new ItemStack(GOLDEN_APPLE, 1);
  player.setItem(apple as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(金苹果) → GoldenAppleItem::onItemRightClick（canAlwaysEat true）→ setActiveHand
  // （useDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(apple as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询双重断言：regeneration 效果出现（概率1.0 确定性）+ 主手变空（无容器物品 shrink 后空）。
  // 食用完成约 tick 5+32=37，留余量到 tick 90。
  pollUntilSucceed(test, () => {
    const regen = (player as any).getEffect("regeneration");
    if (regen === undefined) return false;
    return getMainHandTypeId(player) === "";
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 100,
    onTimeout: () => {
      const regen = (player as any).getEffect("regeneration");
      const mainType = getMainHandTypeId(player);
      test.assert(false,
        `golden_apple_self_consumption: failed: regeneration=${regen ? "present" : "absent"} `
        + `(expected present) mainHand="${mainType}" (expected empty)`);
    },
  });
}

export function registerGoldenAppleSelfConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "golden_apple_self_eaten_applies_regen", goldenAppleSelfConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
}
