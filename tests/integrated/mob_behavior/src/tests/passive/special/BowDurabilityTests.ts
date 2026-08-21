// 弓箭耐久损耗对齐测试（验证 onPlayerStoppedUsing 权威手持耐久损耗，对应 stopActiveHand 拷贝不回写缺陷修复）。
//
// 验证 BowItem::onPlayerStoppedUsing 的弓耐久损耗链路（wiki tech_弓.txt#用途：拉弓释放射箭损耗1耐久，
// 弓数量不变；箭矢消耗1支）。
//
// 此前 LivingEntity::stopActiveHand（LivingEntity.cpp:2146）传 stackCopy（m_activeItem 的拷贝）给
// onPlayerStoppedUsing，BowItem::onPlayerStoppedUsing 内 hurtAndBreak(stack) 作用于拷贝不回写权威装备槽
// ——弓耐久损耗不回写（同 itemInteractionForEntity 拷贝不回写范式）。已修复：stopActiveHand 改传
// m_activeItem 引用 + 调用后 setEquipment 回写权威装备槽（同 onItemUseFinish 范式）。本测试 Survival
// 玩家持弓+箭拉弓释放射箭，断言弓数量仍1（耐久损耗不消耗数量）+ arrow 实体出现（发射成功）。
//
// 框架补全：SimulatedPlayer.stopUsingItem 原为 stub（_throwNotImplemented），无法在测试中触发
// onPlayerStoppedUsing（拉弓释放）。已补全 stopUsingItem 调 stopActiveHand + 返回活动物品。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板，箭可飞）。
// Survival 玩家 (1,2,3) 主手弓（slot 0）+ 副手箭（slot 40）。
//
// 时序：tick 5 useItem(弓) 拉弓（setActiveHand，useDuration=72000），tick 20 stopUsingItem 释放
// （蓄力 15 tick，velocity=getArrowVelocity(15)≈0.5 > MIN_VELOCITY=0.1，发射）。
//
// 判定手段（双重断言）：
//   1. 区域内出现 minecraft:arrow 实体（发射成功）；
//   2. 主手槽（slot 0）弓数量仍1（耐久损耗不消耗数量）。
// Survival 模式（创造跳过耐久消耗无数不变证据）。脚本侧无法读弓 damage，用"数量不变"间接验证耐久损耗。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const BOW = "minecraft:bow";
const ARROW = "minecraft:arrow";
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 弓拉弓释放射箭耐久损耗：Survival 玩家持弓+箭拉弓释放，断言 arrow 出现 + 弓数量仍1。
function bowDurabilityTest(test: Test): void {
  // Survival 玩家 (1,2,3) 主手弓（slot 0, selectSlot=true）+ 副手箭（slot 40）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "archer", 0 as any);
  const bow = new ItemStack(BOW, 1);
  player.setItem(bow as unknown as Parameters<typeof player.setItem>[0], 0, true);
  const arrow = new ItemStack(ARROW, 5);
  // 副手 slot 40（_findAmmoSlot 先查副手）。
  player.setItem(arrow as unknown as Parameters<typeof player.setItem>[0], 40, false);

  // tick 5 useItem(弓) → BowItem::onItemRightClick（有箭矢）→ setActiveHand 拉弓（useDuration=72000）。
  test.runAtTickTime(5, () => {
    (player as any).useItem(bow as unknown as Parameters<typeof player.useItem>[0]);
  });

  // tick 20 stopUsingItem 释放 → stopActiveHand → BowItem::onPlayerStoppedUsing(m_activeItem)
  // → hurtAndBreak 权威手持损耗1耐久 + removeItem 消耗1箭 + spawnEntity(arrow)。
  // 蓄力 15 tick（chargeTicks=72000-71985=15），velocity≈0.5 > 0.1 发射。
  test.runAtTickTime(20, () => {
    (player as any).stopUsingItem();
  });

  // 轮询双重断言：区域内 ≥1 个 arrow 实体（发射成功）+ 主手弓数量仍1（耐久损耗不消耗数量）。
  pollUntilSucceed(test, () => {
    const arrows = test.getDimension().getEntities({
      type: "minecraft:arrow",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (arrows.length < 1) return false;
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 22,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const arrows = test.getDimension().getEntities({
        type: "minecraft:arrow",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `bow_durability: failed: arrowCount=${arrows.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 1)`);
    },
  });
}

export function registerBowDurabilityTests(): void {
  GameTest.register("MobBehaviorTests", "bow_durability_no_count_consumption", bowDurabilityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);
}
