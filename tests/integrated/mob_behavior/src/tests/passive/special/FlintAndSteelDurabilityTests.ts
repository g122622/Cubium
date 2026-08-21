// 打火石耐久损耗对齐测试（验证耐久损耗不消耗数量，对应任务：耐久损耗类物品消耗对齐缺陷修复）。
//
// 验证 FlintAndSteelItem 的耐久损耗链路（wiki tech_打火石.txt#用途：打火石点火损耗 1 耐久，数量不变）。
//
// 此前 FlintAndSteelItem::onItemUse 用 context.getItemStackMut()（调用方局部拷贝）做 hurtAndBreak，
// 耐久损耗不回写权威物品栏，且外层 useItemOnBlock/handleItemUseOn 的 itemId 对比（itemId 不变）走
// 通用 shrink(1)——Survival 模式打火石点一次火，耐久没损耗但数量-1（与 vanilla 损耗 1 耐久数量不变
// 严重不符）。已修复：打火石改用 player->getHeldItem(hand) 权威手持做 hurtAndBreak（耐久回写），
// 外层对比扩展为 itemId+damage（damage 变化跳过 shrink）。本测试验证修复后打火石点火数量不变。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air）。
// Survival 玩家持打火石右键 grass_block (3,1,3) 顶面（face=Up），firePos=(3,2,3)（结构 y=1 air）放火。
// 结构放置 +1 抬升：helper (3,1,3)=grass_block，(3,2,3)=air。
//
// 判定手段（双重断言）：
//   1. firePos (3,2,3) 出现 fire 方块（FlintAndSteelItem 放火成功）；
//   2. 主手槽（slot 0）打火石数量仍为 1（耐久损耗不消耗数量）。
// 创造模式跳过耐久消耗（hurtAndBreak 内 creativeMode 守卫？实际 FlintAndSteelItem 无创造守卫，
// hurtAndBreak 对创造玩家也损耗耐久——但创造模式物品不实际消耗是另一机制）。故用 Survival 模式
// 才能以"数量不变"作为耐久损耗（非数量消耗）的证据。读取主手槽用
// getComponent("minecraft:inventory").container.getItem(0)。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

const FLINT_AND_STEEL = "minecraft:flint_and_steel";
const FIRE_BLOCK = "minecraft:fire";
const FIRE_POS = { x: 3, y: 2, z: 3 };
const GRASS_POS = { x: 3, y: 1, z: 3 };

function getMainHand(player: any): { typeId?: string; amount?: number } | undefined {
  const inv = player.getComponent("minecraft:inventory") as any;
  return inv?.container?.getItem?.(0) as any;
}

// 打火石点火：Survival 玩家持打火石右键 grass_block 顶面放火，断言火出现 + 打火石数量仍 1。
function flintAndSteelDurabilityTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持打火石（slot 0 主手，数量 1）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "igniter", 0 as any);
  const flintAndSteel = new ItemStack(FLINT_AND_STEEL, 1);
  player.setItem(flintAndSteel as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持打火石右键 grass_block (3,1,3) 顶面（face=Up 默认）
  // → FlintAndSteelItem::onItemUse → firePos=(3,2,3) 放火 + hurtAndBreak 权威手持损耗 1 耐久。
  test.runAtTickTime(5, () => {
    player.useItemOnBlock(
      flintAndSteel as unknown as Parameters<typeof player.useItemOnBlock>[0],
      GRASS_POS,
    );
  });

  // 轮询双重断言：firePos 变 fire（放火成功）+ 主手打火石数量仍 1（耐久损耗不消耗数量）。
  pollUntilSucceed(test, () => {
    // 断言 1：firePos (3,2,3) 方块为 fire（FlintAndSteelItem 放置）。
    const block = test.getBlock(FIRE_POS) as unknown as { typeId?: string } | undefined;
    if (block?.typeId !== FIRE_BLOCK) return false;
    // 断言 2：主手打火石数量仍 1（耐久损耗不消耗数量）。
    const mainHand = getMainHand(player);
    if (mainHand?.typeId !== FLINT_AND_STEEL) return false;
    return mainHand.amount === 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const block = test.getBlock(FIRE_POS) as unknown as { typeId?: string } | undefined;
      const mainHand = getMainHand(player);
      test.assert(false,
        `flint_and_steel_durability: failed: firePos={typeId:${block?.typeId}} `
        + `mainHand={typeId:${mainHand?.typeId}, amount:${mainHand?.amount}} `
        + `(expected firePos=${FIRE_BLOCK} and mainHand amount=1)`);
    },
  });
}

export function registerFlintAndSteelDurabilityTests(): void {
  GameTest.register("MobBehaviorTests", "flint_and_steel_durability_no_count_consumption", flintAndSteelDurabilityTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
