// 猪行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 猪被闪电击中后转化为僵尸猪灵。
// 闪电首个 tick 即对命中范围内实体造成 5 点伤害并调用 onStruckByLightning；
// 猪 10 血存活，转化在 spawn 当 tick 完成。和平难度下不转化（须非 Peaceful）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪.txt#行为
function pigLightningStrike(test: Test): void {
  const pigType = "pig";
  const lightningType = "lightning_bolt";
  const zombifiedPiglinType = "zombified_piglin";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // glass_pit 的 (x=2,z=1) 列结构内：y=2 为实心玻璃隔板，y=3 为空气（脚下玻璃可站立），y=4 为空气。
  // 故猪站结构内 y=3 → helper-y=4，上方 y=4 空气留出 2 格高空间。
  // 猪与闪电同格 spawn，闪电 ±3 XZ 命中范围必覆盖猪。
  test.spawn(pigType, { x: 2, y: 4, z: 1 });
  test.spawn(lightningType, { x: 2, y: 4, z: 1 });

  // 闪电转化在首 tick 触发：僵尸猪灵出现且原猪消失即通过。
  test.succeedWhen(() => {
    test.assertEntityPresentInArea(zombifiedPiglinType, true);
    test.assertEntityPresentInArea(pigType, false);
  });
}

// 两只猪各喂胡萝卜后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小猪
// （wiki tech_猪.txt#繁殖：手持胡萝卜/马铃薯/甜菜根右键两只成年猪使其进入"爱心模式"，
//   两只猪靠近后繁殖出小猪，双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// C++ 链路（对齐 MC Java 1.21.11 Pig + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持胡萝卜 + interactWithEntity(pig) → Player::interactOn → pig.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      PigEntity::isBreedingItem(胡萝卜) 命中（PigEntity.cpp:83-89 item==Items::CARROT||POTATO||BEETROOT）
//      → 成体 canBreed() → setInLove(player.playerId())。创造模式喂食不消耗胡萝卜，同一根喂两只猪。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) PigEntity::spawnBaby（PigEntity.cpp:97-115）：构造 PigEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(PIG) 保证 getEntities 可查。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两只猪放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。猪 MOVEMENT_SPEED=0.25，BreedGoal speed=1.0。
//
// 判定手段：繁殖完成后区域内 pig 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪.txt#繁殖（喂胡萝卜→爱心→繁殖小猪+冷却+经验球）
function pigBreedsWhenFedCarrot(test: Test): void {
  const pigType = "pig";

  // 两只成年猪放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  const pig1 = test.spawn(pigType, { x: 4, y: 2, z: 4 });
  const pig2 = test.spawn(pigType, { x: 4, y: 2, z: 6 });

  // 创造玩家持胡萝卜：创造模式喂食不消耗胡萝卜（同一根喂两只猪）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "pigBreeder");
  const carrot = new ItemStack("minecraft:carrot", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(carrot as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只猪：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(pig1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(pig2);
  });

  // 轮询：繁殖完成后区域内 pig 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return pigs.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const pigs = test.getDimension().getEntities({
        type: pigType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `pig did not breed: pigCount=${pigs.length} (expected >=3 after feeding carrot)`);
    },
  });
}

export function registerPigTests(): void {
  GameTest.register("MobBehaviorTests", "pig_lightning_strike", pigLightningStrike)
    .structureName("gametests:glass_pit")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "pig_breeds_when_fed_carrot", pigBreedsWhenFedCarrot)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}


