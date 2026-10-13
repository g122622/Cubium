// 锄耕地耐久损耗对齐测试（验证耐久损耗不消耗数量，对应耐久损耗类物品消耗对齐缺陷修复）。
//
// 验证 HoeItem 的耐久损耗链路（wiki tech_锄.txt#用途：锄右键草地/泥土耕地损耗 1 耐久，数量不变）。
//
// 此前 HoeItem::onItemUse 用 context.getItemStackMut()（调用方局部拷贝）做 hurtAndBreak，耐久损耗不回写
// 权威物品栏，且外层 useItemOnBlock/handleItemUseOn 的 itemId 对比走通用 shrink(1)——Survival 模式锄耕地
// 一次，耐久没损耗但数量-1（与 vanilla 损耗 1 耐久数量不变严重不符）。已修复：锄改用
// player->getHeldItem(hand) 权威手持做 hurtAndBreak（耐久回写），外层对比扩展为 itemId+damage
// （damage 变化跳过 shrink）。本测试验证修复后锄耕地数量不变。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。
// Survival 玩家持木锄右键 grass_block (3,1,3)（结构 y=0 grass_block），grass_block→farmland。
// 注意：锄耕地 blockPos=被点击方块本身（grass_block），非其上方（与打火石放火 firePos=offset(face) 不同）。
// 结构放置 +1 抬升：helper (3,1,3)=grass_block。
//
// 判定手段（双重断言）：
//   1. (3,1,3) 方块变 farmland（HoeItem 耕地成功）；
//   2. 主手槽（slot 0）木锄数量仍为 1（耐久损耗不消耗数量）。
// Survival 模式（创造跳过耐久消耗无数量不变证据）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

const WOODEN_HOE = "minecraft:wooden_hoe";
const FARMLAND_BLOCK = "minecraft:farmland";
const TILL_POS = { x: 3, y: 1, z: 3 };

function getMainHand(player: any): { typeId?: string; amount?: number } | undefined {
  const inv = player.getComponent("minecraft:inventory") as any;
  return inv?.container?.getItem?.(0) as any;
}

// 锄耕地：Survival 玩家持木锄右键 grass_block，断言变 farmland + 木锄数量仍 1。
function hoeTillDurabilityTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持木锄（slot 0 主手，数量 1）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "tiller", 0 as any);
  const hoe = new ItemStack(WOODEN_HOE, 1);
  player.setItem(hoe as unknown as Parameters<typeof player.setItem>[0], 0, true);

  test.setBlockType("minecraft:grass_block", TILL_POS);
  test.setBlockType("minecraft:air", { x: 3, y: 2, z: 3 });
  test.setBlockType("minecraft:water", { x: 4, y: 1, z: 3 });
  // 同步检查实际耕地与耐久变化，避免后续随机刻改变耕地后污染测量。
  test.runAtTickTime(5, () => {
    const used = player.useItemOnBlock(
      hoe as unknown as Parameters<typeof player.useItemOnBlock>[0], TILL_POS, Direction.Up);
    test.assert(used, "wooden hoe should successfully till the grass block");
    const block = test.getBlock(TILL_POS);
    const mainHand = getMainHand(player) as any;
    test.assert(block?.typeId === FARMLAND_BLOCK, `expected farmland immediately after tilling, got ${block?.typeId}`);
    test.assert(mainHand?.typeId === WOODEN_HOE && mainHand.amount === 1,
      "tilling should preserve the hoe item count");
    const damage = mainHand.getComponent("minecraft:durability")?.damage;
    test.assert(damage === 1, `tilling should consume one durability, actual damage=${damage}`);
    test.succeed();
  });
}

export function registerHoeDurabilityTests(): void {
  GameTest.register("MobBehaviorTests", "hoe_till_durability_no_count_consumption", hoeTillDurabilityTest)
    .batch("hoe_durability_solo")
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
