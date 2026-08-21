// 鞍装猪消耗对齐测试（验证 itemInteractionForEntity 权威手持数量消耗，对应同款拷贝缺陷修复）。
//
// 验证 SaddleItem::itemInteractionForEntity 的鞍消耗链路（wiki tech_鞍.txt：玩家手持鞍右键猪装鞍消耗1个鞍）。
//
// 此前 SaddleItem::itemInteractionForEntity 用 stack 参数（Player::interactOn 传入的 getHeldItem 值拷贝
// Player.cpp:2858）做 shrink，shrink 作用于拷贝不回写权威物品栏——Survival 玩家持多个鞍装猪时不消耗
// （仅持1个时靠 Player 路径拷贝 isEmpty 回写空槽侥幸消耗）。已改用 player.getHeldItem(hand) 权威手持
// shrink 修复（同 GoldenAppleItem 修复范式）。本测试持鞍数量2，断言装鞍后主手数量变1（消耗1个），
// 覆盖"持多个不消耗"bug。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。猪 (3,2,3)，Survival 玩家 (1,2,3) 持鞍数量2。
// 猪是被动生物不攻击玩家，环境干净。
//
// 判定手段：主手槽（slot 0）鞍数量变1（消耗1个）。Survival 模式（创造跳过消耗无证据）。
// 区别于 PigTests 装鞍骑乘测试（创造模式+1鞍测骑乘，不验证消耗）。
// 注意：装鞍成功后猪 hasSaddle=true，第二次 interactWithEntity 会触发骑乘分支而非再装鞍——本测试只
// interactWithEntity 一次，只验证装鞍消耗，不涉及骑乘。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const SADDLE = "minecraft:saddle";

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 鞍装猪消耗1个：Survival 玩家持鞍数量2，断言装鞍后主手数量1。
function saddlePigConsumptionTest(test: Test): void {
  // 猪 (3,2,3)（creeper_pit y=0 grass_block 地板）。
  const pig = test.spawn("minecraft:pig", { x: 3, y: 2, z: 3 });

  // Survival 玩家 (1,2,3) 持鞍数量2（slot 0 主手）。持2个验证"持多个也消耗1"。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "saddler", 0 as any);
  const saddles = new ItemStack(SADDLE, 2);
  player.setItem(saddles as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持鞍 interactWithEntity(pig) → SaddleItem::itemInteractionForEntity
  // → setSaddle(true)+setEquipment(0,鞍) + 权威手持 shrink(1)（数量2→1）。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(pig);
  });

  // 轮询断言：主手鞍数量变1（消耗1个）。
  pollUntilSucceed(test, () => {
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const amount = getMainHandAmount(player);
      test.assert(false,
        `saddle_pig_consumption: failed: mainHand amount=${amount} (expected 1, started with 2)`);
    },
  });
}

export function registerSaddleConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "saddle_consumed_when_equipping_pig", saddlePigConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
