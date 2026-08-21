// 金苹果喂僵尸村民消耗测试（验证 itemInteractionForEntity 权威手持消耗，对应同款拷贝缺陷修复）。
//
// 验证 GoldenAppleItem::itemInteractionForEntity 的金苹果消耗链路（wiki tech_金苹果.txt#治愈：
// 虚弱状态下对僵尸村民右键金苹果消耗 1 个并启动治愈）。
//
// 此前 GoldenAppleItem::itemInteractionForEntity 用 stack 参数（Player::interactItemOnEntity 传入的
// getHeldItem 值拷贝 Player.cpp:2856）做 shrink，拷贝 shrink 不回写权威物品栏——Survival 玩家持多个
// 金苹果喂僵尸村民时不消耗（仅持1个时靠 Player 回写空槽侥幸消耗）。已改用 player.getHeldItem(hand)
// 权威手持 shrink 修复（同 BucketItem::itemInteractionForEntity 范式）。本测试持金苹果数量2，断言
// 喂食后主手数量变1（消耗1个），覆盖"持多个不消耗"bug。
//
// 环境选择：creeper_pit（7×5×7 开放坑）。僵尸村民 (3,2,3)，Survival 玩家 (1,2,3) 持金苹果数量2。
// 僵尸村民是怪物会追玩家，但 tick 5 喂食瞬间在原位，喂食后立即断言消耗（不等转化）。
//
// 判定手段：主手槽（slot 0）金苹果数量变1（消耗1个）。Survival 模式（创造跳过消耗无证据）。
// 不等待治愈完成（3600-6000 tick 太长），只验证喂食瞬间的消耗。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const GOLDEN_APPLE = "minecraft:golden_apple";

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 金苹果喂虚弱僵尸村民消耗1个：Survival 玩家持金苹果数量2，断言喂食后主手数量1。
function goldenAppleConsumptionTest(test: Test): void {
  // 僵尸村民 (3,2,3)（creeper_pit y=0 grass_block 地板，脚踩 y=0 grass_block）。
  const zombieVillager = test.spawn("minecraft:zombie_villager", { x: 3, y: 2, z: 3 });

  // Survival 玩家 (1,2,3) 持金苹果数量2（slot 0 主手）。持2个验证"持多个也消耗1"。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "feeder", 0 as any);
  const goldenApples = new ItemStack(GOLDEN_APPLE, 2);
  player.setItem(goldenApples as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 施虚弱（金苹果治愈需虚弱状态；itemInteractionForEntity 检查 hasEffect(Weakness)）。
  (zombieVillager as any).addEffect("weakness", 1200, { showParticles: false });

  // tick 5 玩家持金苹果 interactWithEntity(zombieVillager) → itemInteractionForEntity
  // → startConverting + 权威手持 shrink(1)（数量2→1）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(zombieVillager);
  });

  // 轮询断言：主手金苹果数量变1（消耗1个）。
  pollUntilSucceed(test, () => {
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const amount = getMainHandAmount(player);
      test.assert(false,
        `golden_apple_consumption: failed: mainHand amount=${amount} (expected 1, started with 2)`);
    },
  });
}

export function registerGoldenAppleConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "golden_apple_consumed_when_feeding_zombie_villager", goldenAppleConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
