// 蜂蜜瓶饮用清除中毒（仅中毒，区别牛奶清全部）对齐测试。
//
// 验证 HoneyBottleItem::onItemUseFinish 的解毒特性（wiki tech_蜂蜜瓶.txt#用途：饮用清除中毒效果，
// 返回玻璃瓶；区别牛奶桶清除所有效果）。HoneyBottleItem 继承 FoodItem（onItemRightClick 继承
// setActiveHand 修复），onItemUseFinish 委托 FoodItem::onItemUseFinish（容器物品处理）+
// removeEffect(Poison)（仅清中毒）。
//
// 关键区分点：蜂蜜瓶只清 Poison，保留其他效果（如 Weakness）；牛奶桶 removeAllEffects 清全部。
// 本测试给玩家上 Poison + Weakness 两种效果，喝蜂蜜瓶后断言 Poison 消失、Weakness 保留——验证
// "仅清中毒"语义（区别牛奶桶测试 milk_bucket_clears_effects 清全部）。
//
// 环境：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家 (1,2,3) setFoodLevel(5)（foodLevel<20 可进食，蜂蜜瓶非 canAlwaysEat）持1蜂蜜瓶。
//
// 时序：tick 0 addEffect("poison")+addEffect("weakness") 双效果。tick 5 useItem(蜂蜜瓶) →
// FoodItem::onItemRightClick（继承）→ setActiveHand（useDuration=40，蜂蜜瓶重写）。tick 5+40+余量 ≈ 55
// 饮用完成 → onItemUseFinish → FoodItem shrink(1) stack 空 + HoneyBottleItem removeEffect(Poison) +
// FoodItem 返回玻璃瓶（containerItem）→ 主手变玻璃瓶。
//
// 判定手段（三重断言）：
//   1. poison 效果消失（removeEffect(Poison) 生效）；
//   2. weakness 效果保留（验证"仅清中毒"非清全部，区别牛奶）；
//   3. 主手变 minecraft:glass_bottle（容器物品返还）。
// Survival 模式。weakness 1200 tick 远超测试时长，排除自然过期。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const HONEY_BOTTLE = "minecraft:honey_bottle";
const GLASS_BOTTLE = "minecraft:glass_bottle";

function getMainHandTypeId(player: any): string {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.typeId ?? "";
}

// 蜂蜜瓶饮用仅清中毒保留其他效果 + 返玻璃瓶：Survival 玩家 poison+weakness 双效果喝蜂蜜瓶，
// 断言 poison 消失 + weakness 保留 + 主手变玻璃瓶。
function honeyBottleClearsPoisonOnlyTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持1蜂蜜瓶（slot 0 主手）。setFoodLevel(5) 可进食。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "sips", 0 as any);
  const honey = new ItemStack(HONEY_BOTTLE, 1);
  player.setItem(honey as unknown as Parameters<typeof player.setItem>[0], 0, true);
  (player as any).setFoodLevel(5);

  // 双效果：poison（蜂蜜瓶应清）+ weakness（蜂蜜瓶应保留）。1200 tick 远超测试时长。
  (player as any).addEffect("poison", 1200, { showParticles: false });
  (player as any).addEffect("weakness", 1200, { showParticles: false });

  // tick 5 useItem(蜂蜜瓶) → FoodItem::onItemRightClick（继承 setActiveHand）→ setActiveHand
  // （useDuration=40，蜂蜜瓶重写 getUseDuration）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(honey as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询三重断言：poison 消失 + weakness 保留 + 主手变玻璃瓶。
  // 饮用完成约 tick 5+40=45，留余量到 tick 110。
  pollUntilSucceed(test, () => {
    const poison = (player as any).getEffect("poison");
    if (poison !== undefined) return false;
    const weakness = (player as any).getEffect("weakness");
    if (weakness === undefined) return false;
    return getMainHandTypeId(player) === GLASS_BOTTLE;
  }, {
    startTick: 48,
    interval: 2,
    maxTick: 120,
    onTimeout: () => {
      const poison = (player as any).getEffect("poison");
      const weakness = (player as any).getEffect("weakness");
      const mainType = getMainHandTypeId(player);
      test.assert(false,
        `honey_bottle_clears_poison_only: failed: poison=${poison ? "present" : "absent"} `
        + `(expected absent) weakness=${weakness ? "present" : "absent"} (expected present) `
        + `mainHand=${mainType} (expected ${GLASS_BOTTLE})`);
    },
  });
}

export function registerHoneyBottleConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "honey_bottle_clears_poison_only_when_drunk", honeyBottleClearsPoisonOnlyTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(150);
}
