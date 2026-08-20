// 绵羊行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 绵羊吃草：成年羊每 tick 有 1/1000 概率吃掉脚下的草方块，将其变为泥土。
// EatGrassGoal 检查实体脚下方块（entityPos.down()），若是 grass_block 则在 40 tick 动画后
// 调 setBlockState 把它变成 dirt（需 mob_griefing=true，默认开）。本测试在满铺草地的围栏里
// 放多只羊，断言任一草方块变泥土即通过。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_绵羊.txt#吃草
function sheepEatGrass(test: Test): void {
  const sheepType = "sheep";

  // 结构 grass_pen（9×5×9）：y=0 满铺 9×9 grass_block，y=1..3 玻璃墙围栏+内部空气，y=4 开放。
  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // 故草地地板=结构内 y=0 → helper-y=1；羊站立的空气腔=结构内 y=1 → helper-y=2。
  // 4 只羊分散站位在草地上（各占角落附近），最大化覆盖不同草方块，提高触发概率。
  const sheepPositions = [
    { x: 2, y: 2, z: 2 },
    { x: 6, y: 2, z: 2 },
    { x: 2, y: 2, z: 6 },
    { x: 6, y: 2, z: 6 },
  ];
  for (const p of sheepPositions) {
    test.spawn(sheepType, p);
  }

  // 满铺草地：81 格 grass_block（结构内 y=0 对应 helper-y=1，x,z∈[0,8]）。
  // 任一格变 dirt 即通过（OR 语义）。
  const grassPositions: { x: number; y: number; z: number }[] = [];
  for (let x = 1; x <= 7; x++) {
    for (let z = 1; z <= 7; z++) {
      grassPositions.push({ x, y: 1, z });
    }
  }

  // 概率时序：EatGrassGoal 已对齐 vanilla adjustedTickDelay(1000)=500，GoalSelector 每 2 tick
  // 评估一次 → 单羊每 tick 等效概率 ≈ 1/1000（与 vanilla 一致）。4 羊 × 49 格草，
  // 期望约 250 tick 首次触发 + 40 tick 动画（adjustedTickDelay(40)=20）+ 寻路余量。
  // maxTicks=3000 → 期望触发约 12 次，P(0) 极低，近乎必过。
  // succeedWhen 多 assert 是 AND 短路，这里把"任一格变 dirt"的 OR 语义用 try/catch 转换：
  // 任一格 assert 通过（不抛）则 return 视为条件满足；全不满足则主动抛异常让框架继续轮询。
  test.succeedWhen(() => {
    for (const p of grassPositions) {
      try {
        test.assertBlockPresent("minecraft:dirt", p, true);
        return;
      } catch {
        // 该格尚未变泥土，继续检查下一格
      }
    }
    throw new Error("no dirt yet");
  });
}

// 两只羊各喂小麦后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小羊
// （wiki tech_绵羊.txt#繁殖：手持小麦右键两只成年羊使其进入"爱心模式"，两只羊靠近后繁殖出小羊，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球；小羊颜色由双亲颜色混合决定）。
//
// 本测试验证基础繁殖链路（不验证颜色遗传）。颜色遗传（getDyeColorMixFromParents）理论上可测，
// 但 SheepEntity 染色（染料右键 setFleeceColor）尚未实现——setFleeceColor 仅在 spawnBaby 与
// EvokerSkills(wololo) 调用，无法在测试中构造不同色双亲验证混合色，故颜色混合测试不可行（留 TODO）。
//
// C++ 链路（对齐 MC Java 1.21.11 Sheep + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持小麦 + interactWithEntity(sheep) → Player::interactOn → sheep.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      SheepEntity::isBreedingItem(小麦) 命中（SheepEntity.cpp:122-128 item==Items::WHEAT）
//      → 成体 canBreed() → setInLove(player.playerId())。创造模式喂食不消耗小麦，同一根小麦喂两只羊。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) SheepEntity::spawnBaby（SheepEntity.cpp:135-164）：构造 SheepEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(SHEEP) 保证 getEntities 可查；颜色由 getDyeColorMixFromParents
//      决定（双亲同色返同色，本测试双亲均默认白色不验证混合）。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两只羊放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内），spawnBaby 几乎只需等 30 tick spawnBabyDelay。
//   注意：grass_pen 满铺草地，但 EatGrassGoal(优先级5) 优先级低于 BreedGoal(2)，繁殖期被 Move flag
//   mutex 阻塞不干扰；且吃草不影响繁殖链路。羊 MOVEMENT_SPEED=0.23，BreedGoal speed=1.0，moveTo 快。
//
// 判定手段：繁殖完成后区域内 sheep 数 >=3（原 2 只成年 + 1 只幼体）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30 留喂食+
//   选配偶时间，maxTick=700 留充足余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_绵羊.txt#繁殖（喂小麦→爱心→繁殖小羊+冷却+经验球）
function sheepBreedsWhenFedWheat(test: Test): void {
  const sheepType = "sheep";

  // 两只成年羊放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // 脚下 y=1 grass_block 支撑防下落（grass_pen y=0 grass_block 地板，y=1 air 腔，helper y=2 = 结构 y=1 air）。
  const sheep1 = test.spawn(sheepType, { x: 4, y: 2, z: 4 });
  const sheep2 = test.spawn(sheepType, { x: 4, y: 2, z: 6 });

  // 创造玩家持小麦：创造模式喂食不消耗小麦（同一根小麦喂两只羊）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "sheepBreeder");
  const wheat = new ItemStack("minecraft:wheat", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(wheat as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只羊：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  // 间隔 5 tick 确保第一只羊 setInLove 写入后再喂第二只。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(sheep1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(sheep2);
  });

  // 轮询：繁殖完成后区域内 sheep 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const sheeps = test.getDimension().getEntities({
      type: sheepType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return sheeps.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const sheeps = test.getDimension().getEntities({
        type: sheepType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `sheep did not breed: sheepCount=${sheeps.length} (expected >=3 after feeding wheat)`);
    },
  });
}

export function registerSheepTests(): void {
  GameTest.register("MobBehaviorTests", "sheep_eat_grass", sheepEatGrass)
    .structureName("gametests:grass_pen")
    .maxTicks(3000);

  GameTest.register("MobBehaviorTests", "sheep_breeds_when_fed_wheat", sheepBreedsWhenFedWheat)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}
