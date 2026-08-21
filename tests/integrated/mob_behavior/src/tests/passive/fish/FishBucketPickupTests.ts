// 鱼类/美西螈水桶装取行为类 GameTest。
//
// 验证玩家手持水桶右键鱼/美西螈将其装入对应鱼桶（wiki tech_鳕鱼.txt#繁殖：鱼可以用空桶
// 或水桶装起，得到对应的鱼桶；tech_美西螈.txt#桶装：美西螈可用水桶装起，得到美西螈桶）。
//
// 对齐 Java 1.21.11 Bucketable.bucketMobPickup 流程（本次新增 IBucketable 接口实现）：
//   1. Survival 玩家主手持 water_bucket + interactWithEntity(fish) → Player::interactOn
//      → fish.processInitialInteract → AbstractFishEntity::interactMob override
//      → entity::bucketMobPickup(player, *this, hand)。
//   2. bucketMobPickup（BucketableUtils.cpp）：dynamic_cast<IBucketable*> 守卫 + 检测
//      heldItem==WATER_BUCKET && isAlive → playSound(getPickupSound()) + getBucketItemStack()
//      拿对应鱼桶 + saveToBucketTag()（暂空）+ 非创造模式 heldItem=bucketStack 替换手持 +
//      target.discard() 实体消失 + 返 Success。
//   3. 各鱼子类 override getBucketItemStack 返回对应鱼桶：
//      Cod→cod_bucket / Salmon→salmon_bucket / Pufferfish→pufferfish_bucket /
//      TropicalFish→tropical_fish_bucket / Axolotl→axolotl_bucket。
//
// 此前 Cubium AbstractFishEntity/AxolotlEntity 无 interactMob override（继承 WaterMobEntity→
// MobEntity 默认返 Pass），玩家持水桶右键鱼/美西螈无任何反应——鱼不消失、主手不变鱼桶
// （对齐缺陷）。本次补全 IBucketable 接口 + bucketMobPickup + 各鱼 getBucketItemStack +
// interactMob override，装取主链路打通。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板 + y=1..4 全 air）。鱼/美西螈陆地
// 会窒息（AbstractFishEntity maxAir=480，约 500 tick 后首伤；美西螈 maxAir=6000 更久），但装取
// 测试窗口（~30 tick）远小于窒息线，实体存活不干扰判定。鱼陆地扑腾（updateFlopping）每 100 tick
// 首次跳跃，tick 5 触发 interactWithEntity 时鱼仍在原位（首次扑腾未到），远程触发无距离门控，
// 鱼漂移不影响装取。结构放置 +1 抬升：helper-y=N → 结构内 y=N-1，鱼 spawn 于 (3,2,3) 脚踩结构
// 内 y=0 grass_block。
//
// 判定手段（双重断言）：
//   1. 区域限定 getEntities({type:fishType}) 返回空（鱼被 discard 消失）；
//   2. Survival 玩家主手槽（slot 0）typeId 变为对应鱼桶（非创造模式水桶被替换为鱼桶）。
// 创造模式不消耗水桶（对齐 Java createFilledResult 创造保留原 stack），故必须 Survival 玩家
// 才能以"主手变鱼桶"作为装取成功证据。读取主手槽用
// getComponent("minecraft:inventory").container.getItem(0)（slot 0=主手，对齐 setItem(...,0,true)）。
// 区域限定用 PIT（creeper_pit 7×5×7）排除并行测试污染。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标 x,z∈[0,6], y∈[0,4]。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

const WATER_BUCKET_ITEM = "minecraft:water_bucket";

interface BucketPickupCase {
  testName: string;          // 注册 testName（全等，不带 className）
  entityType: string;        // 主角实体 typeId（spawn 用，无 minecraft: 前缀）
  entityTypeFull: string;    // 查询用全名（带 minecraft: 前缀）
  expectedBucketItem: string; // 装取后主手应变为此鱼桶 typeId
}

// 5 个装取用例：4 种鱼 + 美西螈。entityType 用 spawn 短名（test.spawn 接受无前缀），
// entityTypeFull 带 minecraft: 前缀供 getEntities 查询。
const BUCKET_PICKUP_CASES: BucketPickupCase[] = [
  { testName: "cod_bucket_pickup_by_water_bucket",
    entityType: "cod", entityTypeFull: "minecraft:cod", expectedBucketItem: "minecraft:cod_bucket" },
  { testName: "salmon_bucket_pickup_by_water_bucket",
    entityType: "salmon", entityTypeFull: "minecraft:salmon", expectedBucketItem: "minecraft:salmon_bucket" },
  { testName: "pufferfish_bucket_pickup_by_water_bucket",
    entityType: "pufferfish", entityTypeFull: "minecraft:pufferfish", expectedBucketItem: "minecraft:pufferfish_bucket" },
  { testName: "tropical_fish_bucket_pickup_by_water_bucket",
    entityType: "tropical_fish", entityTypeFull: "minecraft:tropical_fish", expectedBucketItem: "minecraft:tropical_fish_bucket" },
  { testName: "axolotl_bucket_pickup_by_water_bucket",
    entityType: "axolotl", entityTypeFull: "minecraft:axolotl", expectedBucketItem: "minecraft:axolotl_bucket" },
];

// 通用装取测试：Survival 玩家持水桶右键鱼/美西螈，断言实体消失 + 主手变对应鱼桶。
function makeBucketPickupTest(c: BucketPickupCase): (test: Test) => void {
  return function bucketPickupTest(test: Test): void {
    // 主角鱼/美西螈 spawn 于 (3,2,3)（creeper_pit y=0 grass_block 地板，helper y=2→结构 y=1 空气，
    // 脚踩 y=0 grass_block，无需玻璃支撑）。
    const fish = test.spawn(c.entityType, { x: 3, y: 2, z: 3 });

    // Survival 玩家 (1,2,3) 持水桶（slot 0 主手）。距鱼 2 格（interactWithEntity 远程触发无距离门控）。
    // Survival 模式装取会消耗水桶替换为鱼桶（创造模式保留水桶），消耗/替换是装取成功的判定证据。
    const player = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 3 }, "bucketPicker", 0 as any);
    const waterBucket = new ItemStack(WATER_BUCKET_ITEM, 1);
    player.setItem(waterBucket as unknown as Parameters<typeof player.setItem>[0], 0, true);

    // tick 5 玩家持水桶 interactWithEntity(fish) → AbstractFishEntity::interactMob / AxolotlEntity::interactMob
    // → bucketMobPickup → discard 实体 + 主手替换为鱼桶。
    // tick 5 留足 spawn 注册 + 首 tick 稳定；鱼首次扑腾在 tick 100，tick 5 时鱼仍在原位。
    test.runAtTickTime(5, () => {
      (player as any).interactWithEntity(fish);
    });

    // 轮询双重断言：鱼消失（区域 getEntities 空）+ 主手变对应鱼桶。
    // 装取 tick 5 后下一 tick 即可查（discard + heldItem 赋值同步生效）。
    pollUntilSucceed(test, () => {
      // 断言 1：区域内主角实体已消失（discard）。
      const stillPresent = test.getDimension().getEntities({
        type: c.entityTypeFull,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      if (stillPresent.length > 0) return false;

      // 断言 2：主手槽（slot 0）typeId 为对应鱼桶。
      const inv = player.getComponent("minecraft:inventory") as any;
      if (inv === undefined || inv.container === undefined) return false;
      const mainHand = inv.container.getItem(0);
      if (mainHand === undefined) return false;
      return mainHand.typeId === c.expectedBucketItem;
    }, {
      startTick: 6,
      interval: 2,
      maxTick: 40,
      onTimeout: () => {
        const stillPresent = test.getDimension().getEntities({
          type: c.entityTypeFull,
          location: test.worldLocation(PIT_FROM),
          volume: PIT_VOLUME,
        });
        const inv = player.getComponent("minecraft:inventory") as any;
        const mainHand = (inv?.container?.getItem?.(0)) as any;
        const typeId = mainHand?.typeId;
        const amount = mainHand?.amount;
        test.assert(false,
          `${c.testName}: bucket pickup failed: ${c.entityType} present=${stillPresent.length} `
          + `mainHand={typeId:${typeId}, amount:${amount}} (expected ${c.expectedBucketItem})`);
      },
    });
  };
}

export function registerFishBucketPickupTests(): void {
  for (const c of BUCKET_PICKUP_CASES) {
    GameTest.register("MobBehaviorTests", c.testName, makeBucketPickupTest(c))
      .structureName("gametests:creeper_pit")
      .maxTicks(80);
  }
}
