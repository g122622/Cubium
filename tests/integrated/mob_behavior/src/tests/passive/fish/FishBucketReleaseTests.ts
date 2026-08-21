// 鱼桶放鱼行为类 GameTest（反向链路，对应 FishBucketPickupTests 的装取正向链路）。
//
// 验证玩家手持鱼桶右键地面放出对应鱼+水，并返回空桶（wiki tech_鳕鱼.txt#繁殖：鱼桶可放出鱼；
// tech_美西螈.txt#桶装：美西螈桶可放出美西螈）。
//
// 对齐 Java 1.21.11 MobBucketItem.use 流程（Cubium 实现为 FishBucketItem::onItemUse）：
//   1. Survival 玩家主手持 cod_bucket 等 + useItemOnBlock(grass_block, face=Up)
//      → SimulatedPlayer::useItemOnBlock 先 Block.use（grass_block onBlockActivated 基类返 Pass）
//      → fallback Item.useOn → FishBucketItem::onItemUse。
//   2. onItemUse（FishBucketItem.cpp:57-92）：placePos = blockPos.offset(face=Up)（被点击方块上方 air），
//      setBlockState(placePos, WATER) 放水 + scheduleFluidTick + _spawnFish(placePos) 生成鱼
//      + 非创造模式 shrink(1) + _returnEmptyBucket 返回空桶。
//   3. _spawnFish（FishBucketItem.cpp:117-157）：按 m_fishTypeName 创建鱼实体 + setPosition(placePos 中心)
//      + setFromBucket(true)（防消失）+ world.spawnEntity。
//
// 此前装取正向链路（IBucketable + bucketMobPickup，任务 #112）已测；本测试覆盖反向放鱼链路
// （FishBucketItem.onItemUse + _spawnFish），补全鱼桶往返闭环。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air）。玩家右键 grass_block
// (3,1,3)（结构 y=0 grass_block）顶面（face=Up，useItemOnBlock 默认），placePos=(3,2,3)（结构 y=1 air）
// 放水+生成鱼。鱼在水源里存活不窒息（放鱼即在同一 tick 置于水方块）。结构放置 +1 抬升：helper-y=N
// → 结构内 y=N-1，故 helper (3,1,3)=grass_block，(3,2,3)=air。
//
// 判定手段（双重断言）：
//   1. 区域限定 getEntities({type:fishTypeFull}) 出现对应鱼（>=1，_spawnFish 生成）；
//   2. Survival 玩家主手槽（slot 0）typeId 变为 minecraft:bucket（鱼桶消耗返回空桶）。
// 创造模式不消耗鱼桶（onItemUse 内 isCreative 守卫跳过 shrink+_returnEmptyBucket），故必须 Survival
// 玩家才能以"主手变空桶"作为放鱼成功证据。读取主手槽用
// getComponent("minecraft:inventory").container.getItem(0)（slot 0=主手，对齐 setItem(...,0,true)）。
// 区域限定用 PIT（creeper_pit 7×5×7）排除并行测试污染。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 放鱼后主手应变为此物品（空桶）。
const EMPTY_BUCKET_ITEM = "minecraft:bucket";

interface BucketReleaseCase {
  testName: string;          // 注册 testName（全等，不带 className）
  bucketItem: string;        // 玩家初始手持的鱼桶 typeId
  entityTypeFull: string;    // 放鱼后应出现的实体 typeId（带 minecraft: 前缀，供 getEntities 查询）
}

// 5 个放鱼用例：4 种鱼 + 美西螈。bucketItem 是手持鱼桶 typeId，entityTypeFull 是放出的实体全名。
const BUCKET_RELEASE_CASES: BucketReleaseCase[] = [
  { testName: "cod_bucket_releases_cod",
    bucketItem: "minecraft:cod_bucket", entityTypeFull: "minecraft:cod" },
  { testName: "salmon_bucket_releases_salmon",
    bucketItem: "minecraft:salmon_bucket", entityTypeFull: "minecraft:salmon" },
  { testName: "pufferfish_bucket_releases_pufferfish",
    bucketItem: "minecraft:pufferfish_bucket", entityTypeFull: "minecraft:pufferfish" },
  { testName: "tropical_fish_bucket_releases_tropical_fish",
    bucketItem: "minecraft:tropical_fish_bucket", entityTypeFull: "minecraft:tropical_fish" },
  { testName: "axolotl_bucket_releases_axolotl",
    bucketItem: "minecraft:axolotl_bucket", entityTypeFull: "minecraft:axolotl" },
];

// 通用放鱼测试：Survival 玩家持鱼桶右键 grass_block 顶面，断言出现对应鱼 + 主手变空桶。
function makeBucketReleaseTest(c: BucketReleaseCase): (test: Test) => void {
  return function bucketReleaseTest(test: Test): void {
    // Survival 玩家 (1,2,3) 持鱼桶（slot 0 主手）。距放置点 2 格（useItemOnBlock 远程触发无距离门控）。
    // Survival 模式放鱼会消耗鱼桶返回空桶（创造模式保留鱼桶），消耗/替换是放鱼成功的判定证据。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bucketReleaser", 0 as any);
    const fishBucket = new ItemStack(c.bucketItem, 1);
    player.setItem(fishBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

    // tick 5 玩家持鱼桶右键 grass_block (3,1,3) 顶面（face=Up 默认）
    // → FishBucketItem::onItemUse → placePos=(3,2,3) 放水 + _spawnFish 生成鱼 + 主手变空桶。
    // tick 5 留足 spawn 注册 + 首 tick 稳定。
    test.runAtTickTime(5, () => {
      player.useItemOnBlock(
        fishBucket as unknown as Parameters<typeof player.useItemOnBlock>[0],
        { x: 3, y: 1, z: 3 },
      );
    });

    // 轮询双重断言：出现对应鱼（区域 getEntities >=1）+ 主手变空桶。
    // onItemUse 在 tick 5 后下一 tick 即可查（spawnEntity + heldItem 赋值同步生效）。
    pollUntilSucceed(test, () => {
      // 断言 1：区域内出现对应鱼（_spawnFish 生成）。
      const fish = test.getDimension().getEntities({
        type: c.entityTypeFull,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      if (fish.length < 1) return false;

      // 断言 2：主手槽（slot 0）typeId 为空桶（鱼桶消耗返回空桶）。
      const inv = player.getComponent("minecraft:inventory") as any;
      if (inv === undefined || inv.container === undefined) return false;
      const mainHand = inv.container.getItem(0);
      if (mainHand === undefined) return false;
      return mainHand.typeId === EMPTY_BUCKET_ITEM;
    }, {
      startTick: 6,
      interval: 2,
      maxTick: 40,
      onTimeout: () => {
        const fish = test.getDimension().getEntities({
          type: c.entityTypeFull,
          location: test.worldLocation(PIT_FROM),
          volume: PIT_VOLUME,
        });
        const inv = player.getComponent("minecraft:inventory") as any;
        const mainHand = (inv?.container?.getItem?.(0)) as any;
        const typeId = mainHand?.typeId;
        const amount = mainHand?.amount;
        test.assert(false,
          `${c.testName}: bucket release failed: ${c.entityTypeFull} count=${fish.length} `
          + `mainHand={typeId:${typeId}, amount:${amount}} (expected fish>=1 and ${EMPTY_BUCKET_ITEM})`);
      },
    });
  };
}

export function registerFishBucketReleaseTests(): void {
  for (const c of BUCKET_RELEASE_CASES) {
    GameTest.register("MobBehaviorTests", c.testName, makeBucketReleaseTest(c))
      .structureName("gametests:creeper_pit")
      .maxTicks(80);
  }
}
