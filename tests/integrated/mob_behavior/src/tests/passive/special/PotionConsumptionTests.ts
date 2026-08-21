// 药水饮用容器返还对齐测试（验证 updateActiveItem 修复解锁的 PotionItem onItemUseFinish 链路 +
// 容器物品玻璃瓶返还）。
//
// 验证 PotionItem::onItemUseFinish 的容器物品处理（返回玻璃瓶）。PotionItem::onItemRightClick 已调
// setActiveHand（line 120，无 FoodItem 同源缺陷），仅受 updateActiveItem bug 阻塞——修复后解锁。
//
// 药水不需饥饿（PotionItem::onItemRightClick 无条件 setActiveHand，区别食物需 needsFood）。普通
// minecraft:potion 默认无效果（awkward），但容器物品玻璃瓶返还可验证 onItemUseFinish 链路通。
//
// 持多个药水饮用：shrink 后非空（剩1），返回 stack（剩药水留主手）+ 玻璃瓶入背包（PotionItem 容器
// 处理，与 FoodItem 修复后范式一致）。背包满掉落玻璃瓶（spawnItemAtEntity，对齐 MilkBucketItem）。
//
// 环境：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家 (1,2,3) 持2个药水（slot 0 主手）。药水不需 setFoodLevel。
//
// 时序：tick 5 useItem(药水) → PotionItem::onItemRightClick → setActiveHand（useDuration=32）。
// tick 5+32+余量 ≈ 50 饮用完成 → onItemUseFinish → 应用效果（默认无）+ shrink(1) 剩1 +
// 玻璃瓶 inventory.add 入背包 → 返回 stack（剩药水）。
//
// 判定手段（双重断言）：
//   1. 主手仍 minecraft:potion 且数量变1（消耗1个剩1个，容器物品入背包不覆盖主手）；
//   2. 背包出现 minecraft:glass_bottle（容器物品返还）。
// Survival 模式。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const POTION = "minecraft:potion";
const GLASS_BOTTLE = "minecraft:glass_bottle";

function getMainHand(player: any): { typeId: string; amount: number } {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return { typeId: mainHand?.typeId ?? "", amount: mainHand?.amount ?? 0 };
}

function countItemInInventory(player: any, typeId: string): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const container = inv?.container;
  if (container == null) return 0;
  let total = 0;
  for (let i = 0; i < container.size; ++i) {
    const item = container.getItem(i) as any;
    if (item != null && item.typeId === typeId) {
      total += item.amount;
    }
  }
  return total;
}

// 持多个药水饮用1个：Survival 玩家持2药水，断言主手剩1药水 + 背包出现玻璃瓶。
function potionContainerConsumptionTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持2个药水（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "sipper", 0 as any);
  const potions = new ItemStack(POTION, 2);
  player.setItem(potions as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(药水) → PotionItem::onItemRightClick → setActiveHand（useDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(potions as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询双重断言：主手是药水且数量1（消耗1剩1）+ 背包出现玻璃瓶（容器返还）。
  // 饮用完成约 tick 5+32=37，留余量到 tick 90。
  pollUntilSucceed(test, () => {
    const main = getMainHand(player);
    if (main.typeId !== POTION) return false;
    if (main.amount !== 1) return false;
    return countItemInInventory(player, GLASS_BOTTLE) >= 1;
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 100,
    onTimeout: () => {
      const main = getMainHand(player);
      const bottleCount = countItemInInventory(player, GLASS_BOTTLE);
      test.assert(false,
        `potion_container_consumption: failed: mainHand=${main.typeId}:${main.amount} `
        + `(expected ${POTION}:1) glassBottle=${bottleCount} (expected >=1)`);
    },
  });
}

// 持1个药水饮用：Survival 玩家持1药水，断言主手变玻璃瓶（数量归零返回容器替换主手）。
function potionSingleReturnsBottleTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持1个药水（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "sipper1", 0 as any);
  const potion = new ItemStack(POTION, 1);
  player.setItem(potion as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(药水) → setActiveHand（useDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(potion as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：主手变玻璃瓶（数量1）——持1个 shrink 后空返回玻璃瓶替换主手。
  pollUntilSucceed(test, () => {
    const main = getMainHand(player);
    return main.typeId === GLASS_BOTTLE && main.amount === 1;
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 100,
    onTimeout: () => {
      const main = getMainHand(player);
      test.assert(false,
        `potion_single_returns_bottle: failed: mainHand=${main.typeId}:${main.amount} `
        + `(expected ${GLASS_BOTTLE}:1)`);
    },
  });
}

export function registerPotionConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "potion_container_consumed_remaining_kept", potionContainerConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
  GameTest.register("MobBehaviorTests", "potion_single_returns_glass_bottle", potionSingleReturnsBottleTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
}
