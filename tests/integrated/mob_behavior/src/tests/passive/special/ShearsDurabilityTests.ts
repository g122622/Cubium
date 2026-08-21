// 剪刀剪羊耐久损耗对齐测试（验证 itemInteractionForEntity 权威手持耐久损耗，对应同款拷贝缺陷修复）。
//
// 验证 ShearsItem::itemInteractionForEntity 的耐久损耗链路（wiki tech_剪刀.txt#用途：剪刀右键羊剪羊毛
// 损耗 1 耐久，数量不变）。
//
// 此前 ShearsItem::itemInteractionForEntity 用 stack 参数（Player::interactOn 传入的 getHeldItem 值拷贝
// Player.cpp:2858）做 hurtAndBreak，耐久损耗不回写权威物品栏——Survival 玩家持剪刀剪羊耐久没损耗
// （拷贝上损耗未回写）。itemInteractionForEntity 不经 useItemOnBlock 外层消耗逻辑（无外层 shrink
// 补足），故修复仅改物品侧：改用 player.getHeldItem(hand) 权威手持做 hurtAndBreak。本测试 Survival
// 玩家持剪刀（数量1）剪羊，断言剪羊毛成功（区域内出现羊毛掉落物）+ 主手剪刀数量仍1（耐久损耗不消耗数量）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。羊 (3,2,3)，Survival 玩家 (1,2,3) 持剪刀。
// 羊是被动生物不攻击玩家，环境干净。
//
// 判定手段（双重断言）：
//   1. 区域内出现 ≥1 个 minecraft:item 掉落物（羊毛掉落，SheepEntity::shear 掉 1-3 羊毛）；
//   2. 主手槽（slot 0）剪刀数量仍 1（耐久损耗不消耗数量）。
// Survival 模式（创造跳过耐久消耗无数量不变证据）。区别于 BoggedTests 剪沼骸（创造模式只验证蘑菇掉落，
// 不验证耐久损耗）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const SHEARS = "minecraft:shears";
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

function getMainHandAmount(player: any): number {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.amount ?? 0;
}

// 剪刀剪羊：Survival 玩家持剪刀右键羊，断言掉羊毛 + 剪刀数量仍1。
function shearsSheepDurabilityTest(test: Test): void {
  // 羊 (3,2,3)（creeper_pit y=0 grass_block 地板，helper y=2→结构 y=1 空气，脚踩 y=0 grass_block）。
  const sheep = test.spawn("minecraft:sheep", { x: 3, y: 2, z: 3 });

  // Survival 玩家 (1,2,3) 持剪刀（slot 0 主手，数量1）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "shearer", 0 as any);
  const shears = new ItemStack(SHEARS, 1);
  player.setItem(shears as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持剪刀 interactWithEntity(sheep) → ShearsItem::itemInteractionForEntity
  // → IShearable.shear 掉 1-3 羊毛 + hurtAndBreak 权威手持损耗 1 耐久。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(sheep);
  });

  // 轮询双重断言：区域内 ≥1 个 item 掉落物（羊毛）+ 主手剪刀数量仍1。
  // startTick=6 剪羊毛后 1 tick 立即查（羊 MobEntity::tick looting 循环可能拾取掉落物，尽早判定）。
  pollUntilSucceed(test, () => {
    const items = test.getDimension().getEntities({
      type: "minecraft:item",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (items.length < 1) return false;
    return getMainHandAmount(player) === 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const items = test.getDimension().getEntities({
        type: "minecraft:item",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `shears_sheep_durability: failed: itemCount=${items.length} (expected >=1) `
        + `mainHand amount=${getMainHandAmount(player)} (expected 1)`);
    },
  });
}

export function registerShearsDurabilityTests(): void {
  GameTest.register("MobBehaviorTests", "shears_sheep_durability_no_count_consumption", shearsSheepDurabilityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
