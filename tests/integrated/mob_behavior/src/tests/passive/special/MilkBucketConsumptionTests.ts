// 牛奶桶饮用清除效果对齐测试（验证 updateActiveItem 修复解锁的 MilkBucketItem onItemUseFinish 链路）。
//
// 验证 LivingEntity::updateActiveItem 修复（移除递减后 isUsingItem() 提前 return 跳过 onItemUseFinish
// 的有害检查）解锁的牛奶桶饮用链路。MilkBucketItem::onItemRightClick 已调 setActiveHand（line 70），
// MilkBucketItem::onItemUseFinish 调 removeAllEffects 清除所有药水效果（line 84）+ 返回空桶。
// 此前 updateActiveItem 递减到 0 后 isUsingItem()=false 提前 return，onItemUseFinish 永不触发——
// 牛奶桶饮用不清除效果、不返还空桶。已修复 updateActiveItem 对齐 Java updateUsingItem。
//
// 环境：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家 (1,2,3) 持1个牛奶桶（slot 0 主手）。牛奶桶不需饥饿（canEat 不查 foodLevel）。
//
// 时序：tick 0 addEffect("poison") 给玩家中毒（验证饮用后清除）。tick 5 useItem(牛奶桶) →
// MilkBucketItem::onItemRightClick → setActiveHand（useDuration=32）。tick 5+32+余量 ≈ 50 饮用完成
// → onItemUseFinish → removeAllEffects（poison 消失）+ shrink(1) stack 空 → 返回空桶替换主手。
//
// 判定手段（双重断言）：
//   1. 玩家 poison 效果消失（removeAllEffects 生效，区别于效果自然过期——poison 1200 tick 远超测试时长）；
//   2. 主手变 minecraft:bucket（容器物品空桶返还，持1个 stack 空返回空桶替换主手）。
// Survival 模式（创造跳过 shrink 无空桶返还证据）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const MILK_BUCKET = "minecraft:milk_bucket";
const BUCKET = "minecraft:bucket";

function getMainHandTypeId(player: any): string {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.typeId ?? "";
}

// 牛奶桶饮用清除中毒效果 + 返还空桶：Survival 玩家中毒后喝牛奶，断言 poison 消失 + 主手变空桶。
function milkBucketClearsEffectsTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持1个牛奶桶（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "drinker", 0 as any);
  const milk = new ItemStack(MILK_BUCKET, 1);
  player.setItem(milk as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 给玩家中毒效果（1200 tick，远超测试时长，排除自然过期）。addEffect 是 Cubium 扩展。
  (player as any).addEffect("poison", 1200, { showParticles: false });

  // tick 5 useItem(牛奶桶) → MilkBucketItem::onItemRightClick → setActiveHand（useDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(milk as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询双重断言：poison 效果消失（removeAllEffects 生效）+ 主手变空桶（容器物品返还）。
  // 饮用完成约 tick 5+32=37，留余量到 tick 90。
  pollUntilSucceed(test, () => {
    const poison = (player as any).getEffect("poison");
    if (poison !== undefined) return false;
    return getMainHandTypeId(player) === BUCKET;
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 100,
    onTimeout: () => {
      const poison = (player as any).getEffect("poison");
      const mainType = getMainHandTypeId(player);
      test.assert(false,
        `milk_bucket_clears_effects: failed: poison=${poison ? "present" : "absent"} `
        + `(expected absent) mainHand=${mainType} (expected ${BUCKET})`);
    },
  });
}

export function registerMilkBucketConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "milk_bucket_clears_effects_when_drunk", milkBucketClearsEffectsTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(130);
}
