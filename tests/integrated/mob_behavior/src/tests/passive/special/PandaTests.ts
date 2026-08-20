// 熊猫行为类 GameTest。
//
// 熊猫(Panda)是 1.21.11 新生物,mob_behavior 包此前零测试。熊猫有复杂的性格基因系统(7 种性格)、
// 打喷嚏(死代码无触发)、打滚等特有行为,多数依赖性格注入或未实现 goal 难以 GameTest 测试。但繁殖
// 链路完整可测——喂竹子使两头成年熊猫进入爱心状态繁殖出小熊猫,对齐 cow_breeds_when_fed_wheat 范式,
// 填补 Panda 零测试覆盖。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 两头熊猫各喂竹子后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小熊猫
// （wiki tech_熊猫.txt#繁殖：手持竹子右键两头成年熊猫使其进入"爱心模式"，两头熊猫靠近后繁殖出
//   小熊猫，小熊猫的性格基因由双亲主基因/隐性基因遗传决定）。
//
// C++ 链路（对齐 MC Java 1.21.11 Panda + BreedGoal）：
//   1) 玩家主手持竹子 + interactWithEntity(panda)（ScriptSimulatedPlayer.cpp 扩展绑定）
//      → Player::interactOn(panda, MainHand) → panda.processInitialInteract →
//      MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      isBreedingItem(竹子) 命中 → 成体 setInLove(player.playerId())（设 m_loveTimer=600，广播爱心）。
//      PandaEntity::isBreedingItem（PandaEntity.cpp:113-125）：item==Items::BAMBOO 返回 true。
//      创造模式喂食不消耗竹子（同一根竹子喂两头熊猫）。
//   2) BreedGoal::shouldExecute：isInLove() && findNearbyMate() 非空（BREED_DETECTION_RANGE=8.0 格内
//      按 canMateWith 谓词搜索同种 isInLove 配偶）。
//   3) BreedGoal::tick：lookController 看向配偶 + navigator.moveTo(配偶) + m_spawnBabyDelay++。
//      m_spawnBabyDelay >= adjustedTickDelay(SPAWN_BABY_DELAY=60)=30 且 distanceSq<BREED_DISTANCE_SQ=9.0
//      时 spawnBaby()。
//   4) PandaEntity::spawnBaby（PandaEntity.cpp:211-237）：构造 PandaEntity 幼体 + setChild(true) +
//      setPosition + setWorld + inheritGenesFromParents(this, parent)（PandaEntity.cpp:165- 遗传基因：
//      主基因/隐性基因随机组合决定子代基因，再 updatePersonalityFromGenes 推导性格）。
//      基因遗传随机但不影响 typeId——子代始终是 panda,getEntities 计数有效。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。两头熊猫放中心 (4,2,4) 与 (4,2,6) 相距 2 格
//   （distSq=4 < BREED_DISTANCE_SQ=9 已在繁殖距离内），spawnBaby 几乎只需等 30 tick spawnBabyDelay。
//   熊猫体积约 1.3×1.25，grass_pen 内部 7×3×7 空气腔容纳两头成年+一头幼体无碍。
//   不需 night batch/skyAccess：熊猫是被动生物不燃不刷怪干扰。
//
// 判定手段：繁殖完成后区域内 panda 数 >=3（原 2 头成年 + 1 头幼体）。子代 typeId=PANDA 可被
//   getEntities 查到。基因遗传随机不影响计数。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30 留喂食+
//   选配偶时间，maxTick=700 留充足余量（对齐 cow_breeds_when_fed_wheat 的 700）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熊猫.txt#繁殖（喂竹子→爱心→繁殖小熊猫+基因遗传）
function pandaBreedsWhenFedBamboo(test: Test): void {
  const pandaType = "panda";

  // 两头成年熊猫放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // 脚下 y=1 grass_block 支撑防下落（grass_pen y=0 grass_block 地板，y=1 air 腔，helper y=2 = 结构 y=1 air）。
  const panda1 = test.spawn(pandaType, { x: 4, y: 2, z: 4 });
  const panda2 = test.spawn(pandaType, { x: 4, y: 2, z: 6 });

  // 创造玩家持竹子：创造模式喂食不消耗竹子（同一根竹子喂两头熊猫）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "breeder");
  const bamboo = new ItemStack("minecraft:bamboo", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(bamboo as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两头熊猫：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  // 间隔 5 tick 确保第一头熊猫 setInLove 写入后再喂第二头。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(panda1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(panda2);
  });

  // 轮询：繁殖完成后区域内 panda 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const pandas = test.getDimension().getEntities({
      type: pandaType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return pandas.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const pandas = test.getDimension().getEntities({
        type: pandaType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `panda did not breed: pandaCount=${pandas.length} (expected >=3 after breeding)`);
    },
  });
}

export function registerPandaTests(): void {
  GameTest.register("MobBehaviorTests", "panda_breeds_when_fed_bamboo", pandaBreedsWhenFedBamboo)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}
