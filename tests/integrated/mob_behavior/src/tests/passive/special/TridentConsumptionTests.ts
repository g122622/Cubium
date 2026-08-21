// 三叉戟投掷消耗对齐测试（验证 onPlayerStoppedUsing 权威手持数量消耗回写，对应 stopActiveHand 修复）。
//
// 验证 TridentItem::onPlayerStoppedUsing 的三叉戟投掷消耗链路（wiki tech_三叉戟.txt#投掷：拉弓释放投掷
// 三叉戟，非创造模式消耗1个三叉戟数量+生成三叉戟实体）。
//
// 此前 LivingEntity::stopActiveHand 传 stackCopy 拷贝给 onPlayerStoppedUsing，TridentItem:209 stack.shrink(1)
// 作用于拷贝不回写权威装备槽——Survival 玩家投掷三叉戟不消耗（数量不变）。已修复 stopActiveHand 改传
// m_activeItem 引用+setEquipment 回写（同 onItemUseFinish 范式）。本测试 Survival 玩家持三叉戟拉弓释放
// 投掷，断言三叉戟数量-1（投掷消耗）+ trident 实体出现。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板，三叉戟可飞）。
// Survival 玩家 (1,2,3) 主手三叉戟数量2（持2个验证投掷消耗1个）。
//
// 时序：tick 5 useItem(三叉戟) 拉弓（setActiveHand，useDuration=72000），tick 20 stopUsingItem 释放
// （蓄力 15 tick ≥ MIN_CHARGE_TICKS=10，投掷）。
//
// 判定手段（双重断言）：
//   1. 区域内出现 minecraft:trident 实体（投掷成功）；
//   2. 主手槽（slot 0）三叉戟数量变1（投掷消耗1个，2→1）。
// Survival 模式（创造跳过消耗无证据）。区别于弓耐久损耗（数量不变），三叉戟投掷是数量消耗。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const TRIDENT = "minecraft:trident";
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 三叉戟投掷消耗1个：Survival 玩家持三叉戟数量2拉弓释放，断言 trident 出现 + 主手数量变1。
function tridentThrowConsumptionTest(test: Test): void {
  // Survival 玩家 (1,2,3) 主手三叉戟数量2（slot 0, selectSlot=true）。持2个验证投掷消耗1。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "thrower", 0 as any);
  const tridents = new ItemStack(TRIDENT, 2);
  player.setItem(tridents as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 useItem(三叉戟) → TridentItem::onItemRightClick → setActiveHand 拉弓（useDuration=72000）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(tridents as unknown as Parameters<typeof player.useItem>[0]);
  });

  // tick 20 stopUsingItem 释放 → stopActiveHand → TridentItem::onPlayerStoppedUsing(m_activeItem)
  // → spawnEntity(trident) + stack.shrink(1)（数量2→1，回写权威装备槽）。
  // 蓄力 15 tick ≥ MIN_CHARGE_TICKS=10，无激流附魔走正常投掷分支。
  test.runAtTickTime(20, () => {
    (player as any).stopUsingItem();
  });

  // 轮询双重断言：区域内 ≥1 个 trident 实体（投掷成功）+ 主手三叉戟数量变1（消耗1个）。
  pollUntilSucceed(test, () => {
    const tridents2 = test.getDimension().getEntities({
      type: "minecraft:trident",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (tridents2.length < 1) return false;
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 22,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const tridents2 = test.getDimension().getEntities({
        type: "minecraft:trident",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `trident_throw_consumption: failed: tridentCount=${tridents2.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 1, started with 2)`);
    },
  });
}

export function registerTridentConsumptionTests(): void {
  GameTest.register("MobBehaviorTests", "trident_consumed_when_thrown", tridentThrowConsumptionTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);
}
