// 鸡行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 鸡下蛋：成年鸡每隔 6000-12000 tick（5-10 分钟）下 1 个鸡蛋，鸡蛋以掉落物实体（minecraft:item，
// 持 EGG 物品）形式 spawn 在鸡身旁。ChickenEntity::tick 内 eggTimer 每 tick 递减，到 0 时
// spawn ItemEntity(EGG) + 播放音效 + 重置计时器（仅成年、非鸡骑士）。
// 本测试 spawn 成年鸡，断言结构内出现 item 掉落物实体即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#下蛋
function chickenLayEgg(test: Test): void {
  const chickenType = "chicken";

  // 结构 grass_pen（9×5×9 开放玻璃围栏 + 满铺草地，y=0 草地 helper-y=1，y=1 空气腔 helper-y=2）。
  // spawn 2 只成年鸡分散站位，任一只下蛋即通过（提高触发概率，缩短期望等待时间）。
  test.spawn(chickenType, { x: 3, y: 2, z: 3 });
  test.spawn(chickenType, { x: 5, y: 2, z: 5 });

  // eggTimer 初值 6000-12000 随机。2 只鸡取最小值期望约 3000-6000 tick 首次下蛋，最坏 12000 tick。
  // maxTicks=13000 留余量。下蛋 spawn 的 item 掉落物实体类型="item"（minecraft:item）。
  // 用 assertEntityInVolume（基于 getEntities，指定 worldLocation 体积）覆盖整个 grass_pen 内腔，
  // 断言 item 实体出现。item 掉落在草地（helper y=1）上，鸡在 y=2，体积覆盖 y=1..4 全内腔。
  test.succeedWhen(() => {
    assertEntityInVolume(test, "item", 1, 1, 1, 7, 4, 7);
  });
}

// 两只鸡各喂小麦种子后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小鸡
// （wiki tech_鸡.txt#繁殖：手持任意种子右键两只成年鸡使其进入"爱心模式"，两只鸡靠近后繁殖出小鸡，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// C++ 链路（对齐 MC Java 1.21.11 Chicken + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持小麦种子 + interactWithEntity(chicken) → Player::interactOn → chicken.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      ChickenEntity::isBreedingItem(小麦种子) 命中（ChickenEntity.cpp:107-114 item==Items::WHEAT_SEEDS
//      ||PUMPKIN_SEEDS||MELON_SEEDS||BEETROOT_SEEDS）→ 成体 canBreed() → setInLove(player.playerId())。
//      创造模式喂食不消耗种子，同一根种子喂两只鸡。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) ChickenEntity::spawnBaby（ChickenEntity.cpp:121-139）：构造 ChickenEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(CHICKEN) 保证 getEntities 可查。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两只鸡放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。鸡 MOVEMENT_SPEED=0.25，BreedGoal speed=1.0。
//
// 判定手段：繁殖完成后区域内 chicken 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_鸡.txt#繁殖（喂种子→爱心→繁殖小鸡+冷却+经验球）
function chickenBreedsWhenFedSeeds(test: Test): void {
  const chickenType = "chicken";

  // 两只成年鸡放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  const chicken1 = test.spawn(chickenType, { x: 4, y: 2, z: 4 });
  const chicken2 = test.spawn(chickenType, { x: 4, y: 2, z: 6 });

  // 创造玩家持小麦种子：创造模式喂食不消耗种子（同一根喂两只鸡）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "chickenBreeder");
  const seeds = new ItemStack("minecraft:wheat_seeds", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(seeds as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只鸡：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(chicken1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(chicken2);
  });

  // 轮询：繁殖完成后区域内 chicken 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const chickens = test.getDimension().getEntities({
      type: chickenType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return chickens.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const chickens = test.getDimension().getEntities({
        type: chickenType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `chicken did not breed: chickenCount=${chickens.length} (expected >=3 after feeding seeds)`);
    },
  });
}

export function registerChickenTests(): void {
  GameTest.register("MobBehaviorTests", "chicken_lay_egg", chickenLayEgg)
    .structureName("gametests:grass_pen")
    .maxTicks(13000);

  GameTest.register("MobBehaviorTests", "chicken_breeds_when_fed_seeds", chickenBreedsWhenFedSeeds)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}
