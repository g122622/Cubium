// 容器食物食用消耗对齐测试（验证 FoodItem 食用完成链路 + 持多个容器食物剩余保留 + 容器物品返还）。
//
// 验证两条修复：
// 1. FoodItem::onItemRightClick 缺 setActiveHand 致食用完成链路断裂：FoodItem 重写 onItemRightClick
//    仅返回 Consume 未调 setActiveHand（区别于基础 Item::onItemRightClick 的 isFood 分支与
//    BowItem/PotionItem/MilkBucketItem 范式）。setActiveHand 缺失 → m_activeItem/m_activeItemUseCount
//    未设置 → LivingEntity::tick → updateActiveItem 不递减 → onItemUseFinish 永不触发。已补
//    player.setActiveHand(hand)。
// 2. FoodItem::onItemUseFinish 容器物品处理：持多个容器食物（如2个蘑菇煲）食用1个，shrink 后 stack
//    非空（剩1），应返回 shrink 后的 stack（剩余食物留主手）+ 容器物品（碗）放背包；仅持1个（stack 空）
//    才返回容器物品替换主手。此前无条件返回容器物品致持多个容器食物食用时剩余食物丢失（主手被碗覆盖）。
//
// 环境：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家 (1,2,3) setFoodLevel(5)（foodLevel<20 可进食）持2个蘑菇煲（slot 0 主手）。
//
// 时序：tick 5 useItem(蘑菇煲) → FoodItem::onItemRightClick（canEat）→ setActiveHand
// （useDuration=32）。tick 5+32+余量 ≈ 50 食用完成 → onItemUseFinish → shrink(1) 剩1蘑菇煲返回主手
// + 碗 inventory.add 入背包（合并顺序：主手→副手→快捷栏→主背包，主手是蘑菇煲不合并碗，副手空→碗入副手
// 或快捷栏空槽）。
//
// 判定手段（双重断言）：
//   1. 主手槽（slot 0）蘑菇煲数量变1（消耗1个，剩1个，区别于"持1个食用后主手变碗"）；
//   2. 背包（slot 0-40）出现 minecraft:bowl（容器物品返还）。
// Survival 模式（创造跳过消耗无证据）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const MUSHROOM_STEW = "minecraft:mushroom_stew";
const BOWL = "minecraft:bowl";

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

function getMainHandTypeId(player: any): string {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.typeId ?? "";
}

// 遍历背包槽位（0-40：0-8 快捷栏含主手、9-35 主背包、36-39 护甲、40 副手）查指定 typeId 的总数。
function countItemInInventory(player: any, typeId: string): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const container = inv?.container;
  if (container == null) return 0;
  let total = 0;
  // container.size 覆盖主背包+快捷栏+护甲+副手（PlayerInventory 共 41 槽）。
  for (let i = 0; i < container.size; ++i) {
    const item = container.getItem(i) as any;
    if (item != null && item.typeId === typeId) {
      total += item.amount;
    }
  }
  return total;
}

// 持多个容器食物食用：Survival 玩家持2蘑菇煲食用1个，断言主手剩1蘑菇煲 + 背包出现碗。
function mushroomStewContainerConsumptionTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持2个蘑菇煲（slot 0 主手）。setFoodLevel(5) 使 foodLevel<20 可进食。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "eater", 0 as any);
  const stews = new ItemStack(MUSHROOM_STEW, 2);
  player.setItem(stews as unknown as Parameters<typeof player.setItem>[0], 0, true);
  (player as any).setFoodLevel(5);

  // tick 5 useItem(蘑菇煲) → FoodItem::onItemRightClick（canEat true）→ setActiveHand
  // （m_activeItemUseCount=getUseDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(stews as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询双重断言：主手是蘑菇煲且数量=1（消耗1个剩1个）+ 背包出现碗（容器物品返还）。
  // 食用完成约 tick 5+32=37，留余量到 tick 80。
  pollUntilSucceed(test, () => {
    if (getMainHandTypeId(player) !== MUSHROOM_STEW) return false;
    if (getMainHandAmount(player) !== 1) return false;
    return countItemInInventory(player, BOWL) >= 1;
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 90,
    onTimeout: () => {
      const mainType = getMainHandTypeId(player);
      const mainAmt = getMainHandAmount(player);
      const bowlCount = countItemInInventory(player, BOWL);
      test.assert(false,
        `mushroom_stew_container_consumption: failed: mainHand=${mainType}:${mainAmt} `
        + `(expected ${MUSHROOM_STEW}:1) bowlCount=${bowlCount} (expected >=1)`);
    },
  });
}

// 持1个容器食物食用：Survival 玩家持1蘑菇煲食用，断言主手变碗（数量归零返回容器物品替换主手）。
function mushroomStewSingleReturnsBowlTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持1个蘑菇煲（slot 0 主手）。setFoodLevel(5) 可进食。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "eater1", 0 as any);
  const stew = new ItemStack(MUSHROOM_STEW, 1);
  player.setItem(stew as unknown as Parameters<typeof player.setItem>[0], 0, true);
  (player as any).setFoodLevel(5);

  // tick 5 useItem(蘑菇煲) → setActiveHand（useDuration=32）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(stew as unknown as Parameters<typeof player.useItem>[0]);
  });

  // 轮询断言：主手变碗（数量1）——持1个食用 shrink 后 stack 空，返回容器物品（碗）替换主手。
  pollUntilSucceed(test, () => {
    return getMainHandTypeId(player) === BOWL && getMainHandAmount(player) === 1;
  }, {
    startTick: 40,
    interval: 2,
    maxTick: 90,
    onTimeout: () => {
      const mainType = getMainHandTypeId(player);
      const mainAmt = getMainHandAmount(player);
      test.assert(false,
        `mushroom_stew_single_returns_bowl: failed: mainHand=${mainType}:${mainAmt} `
        + `(expected ${BOWL}:1)`);
    },
  });
}

export function registerFoodContainerConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "mushroom_stew_container_consumed_remaining_kept", mushroomStewContainerConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
  GameTest.register("MobBehaviorTests", "mushroom_stew_single_returns_bowl", mushroomStewSingleReturnsBowlTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(120);
}
