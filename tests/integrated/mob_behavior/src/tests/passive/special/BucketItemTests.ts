// 水桶核心玩法行为类 GameTest（舀水/倒水/挤奶手持替换对齐验证）。
//
// 验证 BucketItem 的三个核心对齐行为（wiki tech_水桶.txt#装水/倒水、tech_水桶.txt#挤奶）：
//   1. 空桶右键水源 → 手持变水桶（舀水）；
//   2. 水桶右键地面 → 放水方块 + 手持变空桶（倒水）；
//   3. 空桶右键牛 → 手持变牛奶桶（挤奶）。
//
// 对齐 Java 1.21.11 BucketItem.use / interactLivingEntity（Cubium 实现为 BucketItem::onItemUse /
// itemInteractionForEntity）。此前 BucketItem 三处（舀水/舀粉雪/倒水/直接放置/挤奶）与 FishBucketItem
// 同源对齐缺陷：用 context.getItemStackMut()（调用方局部拷贝）或 Player 路径传入的 stack 拷贝做
// shrink+替换，赋值不回写权威物品栏——玩家舀水后空桶未变水桶（变空堆+背包多水桶）、倒水后水桶未变
// 空桶、挤奶后空桶未变牛奶桶（牛奶桶进背包+手持被回写清空）。已改为直接操作 player->getHeldItem(hand)
// 权威引用修复（同 FishBucketItem/PowderSnowBucketItem 范式）。本测试覆盖舀水/倒水/挤奶三个核心场景。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air）。
//   - 舀水测试：用 fillBlock 在 (3,2,3) 放水源（结构 y=1 air→水），玩家持空桶右键水源方块。
//   - 倒水测试：玩家持水桶右键 grass_block (3,1,3) 顶面（face=Up），placePos=(3,2,3) 放水。
//   - 挤奶测试：spawn 牛 (3,2,3)，玩家持空桶 interactWithEntity(cow)。
// 结构放置 +1 抬升：helper-y=N → 结构内 y=N-1，故 helper (3,1,3)=grass_block，(3,2,3)=air。
//
// 判定手段：
//   - 舀水/倒水：主手槽（slot 0）typeId 变化（空桶↔水桶）+ 放置/取水后方块 typeId 判定。
//   - 挤奶：主手槽 typeId 变空桶→牛奶桶 + 牛仍存活（挤奶不伤害牛）。
// Survival 模式（消耗/替换是成功证据）。读取主手槽用
// getComponent("minecraft:inventory").container.getItem(0)（slot 0=主手）。
// 方块查询用 test.getBlock(pos).typeId（对齐 EndRodTests/DragonEggTests 范式）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { fillBlock } from "../../../utils/block/build.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 物品 typeId 常量。
const EMPTY_BUCKET = "minecraft:bucket";
const WATER_BUCKET = "minecraft:water_bucket";
const MILK_BUCKET = "minecraft:milk_bucket";
// 方块 typeId 常量。
const WATER_BLOCK = "minecraft:water";
// 操作位置（grass_block (3,1,3) 上方 air / 水源）。
const PLACE_POS = { x: 3, y: 2, z: 3 };
const GRASS_POS = { x: 3, y: 1, z: 3 };

// 读取玩家主手槽（slot 0）typeId，未取到返 undefined。
function getMainHandTypeId(player: any): string | undefined {
  const inv = player.getComponent("minecraft:inventory") as any;
  const mainHand = inv?.container?.getItem?.(0) as any;
  return mainHand?.typeId;
}

// === 测试 1：空桶舀水 → 手持变水桶 ===
// wiki tech_水桶.txt#装水：空桶右键水源方块获得水桶（手持替换）。
function bucketPicksUpWaterTest(test: Test): void {
  // 在 (3,2,3) 放水源方块（creeper_pit y=0 grass_block 地板，helper y=2→结构 y=1 空气格放水）。
  fillBlock(test, WATER_BLOCK, 3, 2, 3, 3, 2, 3);

  // Survival 玩家 (1,2,3) 持空桶（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bucketFiller", 0 as any);
  const emptyBucket = new ItemStack(EMPTY_BUCKET, 1);
  player.setItem(emptyBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持空桶右键水源 (3,2,3) → BucketItem::onItemUse 空桶分支 → pickupFluid
  // → 主手变水桶。注意：右键水源方块本身（blockPos=水源），非其上方。
  test.runAtTickTime(5, () => {
    player.useItemOnBlock(
      emptyBucket as unknown as Parameters<typeof player.useItemOnBlock>[0],
      { x: 3, y: 2, z: 3 },
    );
  });

  // 轮询断言：主手变水桶（舀水后手持替换）。
  pollUntilSucceed(test, () => {
    return getMainHandTypeId(player) === WATER_BUCKET;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const typeId = getMainHandTypeId(player);
      test.assert(false,
        `bucket_picks_up_water: failed: mainHand={typeId:${typeId}} (expected ${WATER_BUCKET})`);
    },
  });
}

// === 测试 2：水桶倒水 → 放水方块 + 手持变空桶 ===
// wiki tech_水桶.txt#倒水：水桶右键地面放出水方块并消耗返回空桶（手持替换）。
function bucketEmptiesWaterTest(test: Test): void {
  // Survival 玩家 (1,2,3) 持水桶（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bucketEmptier", 0 as any);
  const waterBucket = new ItemStack(WATER_BUCKET, 1);
  player.setItem(waterBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持水桶右键 grass_block (3,1,3) 顶面（face=Up 默认）
  // → BucketItem::onItemUse 满桶分支 → tryPlaceContainedLiquid 放水到 placePos=(3,2,3)
  // + 主手变空桶。
  test.runAtTickTime(5, () => {
    player.useItemOnBlock(
      waterBucket as unknown as Parameters<typeof player.useItemOnBlock>[0],
      GRASS_POS,
    );
  });

  // 轮询双重断言：placePos 变水方块（倒水放置）+ 主手变空桶（消耗返回）。
  pollUntilSucceed(test, () => {
    // 断言 1：placePos (3,2,3) 方块为水（tryPlaceContainedLiquid 放置）。
    const block = test.getBlock(PLACE_POS) as unknown as { typeId?: string } | undefined;
    if (block?.typeId !== WATER_BLOCK) return false;
    // 断言 2：主手变空桶。
    return getMainHandTypeId(player) === EMPTY_BUCKET;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const block = test.getBlock(PLACE_POS) as unknown as { typeId?: string } | undefined;
      const typeId = getMainHandTypeId(player);
      test.assert(false,
        `bucket_empties_water: failed: placePos={typeId:${block?.typeId}} `
        + `mainHand={typeId:${typeId}} (expected placePos=${WATER_BLOCK} and mainHand=${EMPTY_BUCKET})`);
    },
  });
}

// === 测试 3：空桶挤奶 → 手持变牛奶桶 ===
// wiki tech_水桶.txt#挤奶：空桶右键牛获得牛奶桶（手持替换，挤奶不伤害牛）。
function bucketMilksCowTest(test: Test): void {
  // 牛 spawn 于 (3,2,3)（creeper_pit y=0 grass_block 地板，脚踩 y=0 grass_block）。
  const cow = test.spawn("cow", { x: 3, y: 2, z: 3 });

  // Survival 玩家 (1,2,3) 持空桶（slot 0 主手）。
  const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bucketMilker", 0 as any);
  const emptyBucket = new ItemStack(EMPTY_BUCKET, 1);
  player.setItem(emptyBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // tick 5 玩家持空桶 interactWithEntity(cow) → Player::interactItemOnEntity
  // → BucketItem::itemInteractionForEntity → 主手变牛奶桶。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(cow);
  });

  // 轮询双重断言：主手变牛奶桶（挤奶手持替换）+ 牛仍存活（挤奶不伤害）。
  pollUntilSucceed(test, () => {
    // 断言 1：主手变牛奶桶。
    if (getMainHandTypeId(player) !== MILK_BUCKET) return false;
    // 断言 2：牛仍存活（挤奶不伤害牛，区域 getEntities 仍有牛）。
    const cows = test.getDimension().getEntities({
      type: "minecraft:cow",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    return cows.length >= 1;
  }, {
    startTick: 6,
    interval: 2,
    maxTick: 40,
    onTimeout: () => {
      const typeId = getMainHandTypeId(player);
      const cows = test.getDimension().getEntities({
        type: "minecraft:cow",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      test.assert(false,
        `bucket_milks_cow: failed: mainHand={typeId:${typeId}} cows=${cows.length} `
        + `(expected mainHand=${MILK_BUCKET} and cows>=1)`);
    },
  });
}

export function registerBucketItemTests(): void {
  GameTest.register("MobBehaviorTests", "bucket_picks_up_water", bucketPicksUpWaterTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "bucket_empties_water", bucketEmptiesWaterTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
  GameTest.register("MobBehaviorTests", "bucket_milks_cow", bucketMilksCowTest)
    .structureName("gametests:creeper_pit")
    .maxTicks(80);
}
