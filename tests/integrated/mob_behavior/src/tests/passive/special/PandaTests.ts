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

// 好斗熊猫被攻击后持续反击攻击者（wiki tech_熊猫.txt#行为:92：熊猫在被生物攻击后会尝试反击该生物一次，
// 类似羊驼。但好斗型熊猫会一直反击，直到目标死亡或消失。#变种:123-127 好斗的熊猫：自己或附近的熊猫
// 被攻击后会对攻击者敌对；被攻击时不会惊慌逃窜；会持续攻击，而不是只攻击一次）。
//
// 本测试验证好斗熊猫反击链路（区别普通熊猫"反击一次"——普通熊猫 didBite 机制 Cubium 暂未实现，
// 见下方 C++ 链路说明）。好斗熊猫被攻击后设攻击者为 attackTarget 并持续追击反击。
//
// C++ 链路（对齐 MC Java 1.21.11 Panda）：
//   1) 玩家 attackEntity(熊猫) → PandaEntity::hurt → AnimalEntity::hurt 造伤害。
//   2) targetSelector 优先级1 PandaHurtByTargetGoal（HurtByTargetGoal 子类, alertOthers=true）
//      （PandaEntity.cpp:368-378）：被攻击时取 getLastHurtBy()=玩家 作 attackTarget。
//      alertOthers 谓词 !isAggressive() 只警醒好斗熊猫（非好斗不警醒附近同类），但被攻击熊猫自己
//      无论性格都设 attackTarget（HurtByTargetGoal 基类 alertSelf 语义）。
//   3) goalSelector 优先级3 PandaAttackGoal（PandaGoals.cpp:188-208，继承 MeleeAttackGoal）：
//      shouldExecute = canPerformAction()（!躺/!惊吓/!吃/!打滚/!坐）&& MeleeAttackGoal::shouldExecute
//      （取 attackTarget=玩家 + 寻路）。startExecuting/tick 寻路接近玩家，attackEntityAsMob→
//      hurt(玩家, ATTACK_DAMAGE)。好斗熊猫 ATTACK_DAMAGE=6.0（PandaEntity.cpp:404-406）。
//
// **关键设计——spawnEvent 确定性构造好斗熊猫**：
//   熊猫性格构造期 randomizePersonality 随机（好斗仅 1.6%），测试需确定性好斗熊猫。GameTestHelper
//   applySpawnEvent 支持 panda<minecraft:aggressive>（GameTestHelper.cpp 熊猫分支）：setMainGene(4=Aggressive)
//   + updatePersonalityFromGenes + refreshAttributesForPersonality（重设 ATTACK_DAMAGE=6.0）。test.spawn
//   ("panda<minecraft:aggressive>", pos) 派发事件生成确定性好斗熊猫。
//
// **C++ 偏差说明（TODO）**：vanilla Panda.doHurtTarget（Panda.java:319-325）非好斗熊猫攻击时设 didBite=true，
//   PandaHurtByTargetGoal.canContinueToUse 检测 didBite 后 setTarget(null) 停止——故普通熊猫反击一次即停。
//   Cubium 暂未实现 didBite/gotBamboo 字段（PandaEntity.cpp:365-367 TODO 注释），普通熊猫会持续反击
//   （与原版偏差）。本测试用**好斗熊猫**（本就持续反击，与原版一致），避开此偏差，验证反击链路本身。
//   普通熊猫"反击一次"的对照测试待 didBite 机制实现后补。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。好斗熊猫 (2,2,3) + Survival 玩家 (5,2,3)，水平距 3 格。
//   grass_pen 自带玻璃围墙把熊猫被玩家击退（-x 方向）后的运动限制在结构内——此前 wolf_retaliates
//   用 creeper_pit（无围墙）狼被击退出结构边界持续下落追不到玩家，测试超时。grass_pen 围栏规避此问题。
//   熊猫 MOVEMENT_SPEED=0.15 较慢，3 格接近 + 攻击冷却约需 60-100 tick，maxTicks=800 留充裕余量。
//   玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被反击）。
//
// 判定手段：succeedWhen 每 tick 持续检查玩家 HP<20。熊猫反击 hurt(玩家, 6.0)，玩家满血 20 → 14。
//   玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熊猫.txt#行为（被攻击反击）+ #变种（好斗熊猫持续攻击）
function pandaAggressiveRetaliates(test: Test): void {
  const pandaType = "panda<minecraft:aggressive>";

  // 好斗熊猫 (2,2,3) + Survival 玩家 (5,2,3)，水平距 3 格。grass_pen 玻璃围墙限制熊猫被击退后运动范围。
  // panda<minecraft:aggressive> spawnEvent 派发 setMainGene(4)+updatePersonalityFromGenes 生成确定性好斗熊猫。
  const panda = test.spawn(pandaType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击熊猫：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定 + spawnEvent 派发。
  // attackEntity 远程命中触发 PandaHurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(8, () => {
    player.attackEntity(panda);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + PandaHurtByTargetGoal 设目标 + PandaAttackGoal 寻路接近 3 格 + 攻击冷却 + hurt(6.0)。
  // 熊猫 0.15 速度较慢，3 格接近约需 60-100 tick，maxTicks=800 留充裕余量。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `aggressive panda did not retaliate, hp=${(health as any).currentValue}`);
  });
}

// 熊猫打喷嚏使附近成年熊猫跳跃（wiki tech_熊猫.txt#打喷嚏：幼年熊猫有 1/700 概率打喷嚏，打喷嚏时
//   会喷出粘液球，附近的成年熊猫会被惊得跳起来）。
//
// 本测试验证打喷嚏链路 _onSneezeComplete 的"成年熊猫跳跃"副作用——这是打喷嚏完成后唯一确定性的、
// 可观测的物理效果（掉粘液球仅 1/700 概率，不可测；音效/粒子无 GameTest 断言手段）。
//
// C++ 链路（对齐 MC Java 1.21.11 Panda.sneeze + Panda._onSneezeComplete）：
//   1) sneeze(true) 设 m_sneezing=true + m_sneezeTimer=SNEEZE_DURATION(20)（PandaEntity.cpp:436-447）。
//   2) PandaEntity::tick 每帧 m_sneezeTimer-- ；timer==19 播 playPreSneezeSound（PandaEntity.cpp:260-266）。
//   3) timer 递减到 0 时 m_sneezing=false 并调 _onSneezeComplete()（PandaEntity.cpp:268-271）。
//   4) _onSneezeComplete 遍历自身 boundingBox.expand(10) 内的实体，对其中"成年 && onGround && !inWater"
//      的 PandaEntity 调 panda->jump()（PandaEntity.cpp:490-501）。jump() 设垂直速度 0.42（JUMP_STRENGTH），
//      受影响熊猫下一 tick 起离地上升。
//
// 确定性触发：自然 PandaSneezeGoal 仅幼年熊猫 1/6000 概率触发（PandaGoals.cpp:142-168），测试不可等待
//   随机概率。故用 GameTestHelper 新增的 panda<minecraft:sneeze> spawnEvent：spawn 时立即调 sneeze(true)，
//   绕过 PandaSneezeGoal 的 isChild/概率门控，确定性启动打喷嚏链路。
//
// 布局：grass_pen（9×5×9 玻璃围栏）。
//   - 打喷嚏主体 panda<minecraft:sneeze> 于 (4,2,4)：spawnEvent 派发时 sneeze(true) 设 m_sneezing=true。
//     isSneezing 时 canPerformAction()=false，所有 goal（含 RandomWalkingGoal）不执行 → 主体静止在地面，
//     y 恒定。_onSneezeComplete 的 jump 循环用 getEntitiesInAABB(box, this) 排除 this → 主体自身不会被 jump。
//   - 观察熊猫（成年普通 panda）于 (4,2,6)：距主体 2 格 << 10 格搜索半径，在 jump 候选范围内。
//     观察熊猫 RandomWalkingGoal 可能让其走动，但不会主动跳跃（普通熊猫无跳跃 goal）→ jump 前 y 在地面。
//
// 判定手段：jump() 给观察熊猫 0.42 垂直初速度，y 上升约 12 tick 后回落。pollUntilSucceed 间隔 2 tick
//   在 sneeze 完成窗口（tick 22-60）持续捕获。断言条件：区域内两只 panda 中 maxY - minY > 0.3
//   （一只跳起一只在地面）。maxY-minY 判定不依赖哪只是哪只，只需"一只跳起、一只在地面"——
//   主体静止不跳（y 恒定），观察熊猫 jump 后 y 上升，差异出现。
//
// 时序：tick 0 spawn + sneeze(true) → tick 1..20 timer 递减 → tick 20 _onSneezeComplete 调 jump →
//   tick 21+ 观察熊猫 y 上升。pollUntilSucceed startTick=22（sneeze 完成后），interval=2（捕获窄跳跃窗口），
//   maxTick=60。maxTicks=100 留余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_熊猫.txt#打喷嚏（幼年打喷嚏+成年熊猫惊跳）
function pandaSneezeMakesNearbyAdultsJump(test: Test): void {
  const pandaType = "panda";

  // 打喷嚏主体：panda<minecraft:sneeze> spawnEvent 立即调 sneeze(true)。
  // 主体于 (4,2,4)，isSneezing 期间静止不跳，作"在地面"参考。
  test.spawn(`${pandaType}<minecraft:sneeze>`, { x: 4, y: 2, z: 4 });

  // 观察熊猫（成年普通 panda）：于 (4,2,6)，距主体 2 格 < 10 格 jump 搜索半径。
  // 普通熊猫无跳跃 goal，jump 前 y 在地面；jump 后 y 上升。
  test.spawn(pandaType, { x: 4, y: 2, z: 6 });

  // 区域限定用 PEN（grass_pen 9×5×9）排除并行测试污染。
  pollUntilSucceed(test, () => {
    const pandas = test.getDimension().getEntities({
      type: pandaType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (pandas.length < 2) {
      return false;
    }
    // maxY - minY > 0.3：观察熊猫 jump 后 y 上升，主体 y 恒定 → 差异出现。
    let minY = pandas[0].location.y;
    let maxY = minY;
    for (let i = 1; i < pandas.length; i++) {
      const y = pandas[i].location.y;
      if (y < minY) minY = y;
      if (y > maxY) maxY = y;
    }
    return maxY - minY > 0.3;
  }, {
    startTick: 22,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const pandas = test.getDimension().getEntities({
        type: pandaType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const ys = pandas.map(p => p.location.y.toFixed(2));
      test.assert(false,
        `panda sneeze did not make nearby adult jump, pandaYs=[${ys.join(",")}]`);
    },
  });
}

export function registerPandaTests(): void {
  GameTest.register("MobBehaviorTests", "panda_breeds_when_fed_bamboo", pandaBreedsWhenFedBamboo)
    .structureName("gametests:grass_pen")
    .maxTicks(700);

  GameTest.register("MobBehaviorTests", "panda_aggressive_retaliates", pandaAggressiveRetaliates)
    .structureName("gametests:grass_pen")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "panda_sneeze_makes_nearby_adults_jump", pandaSneezeMakesNearbyAdultsJump)
    .structureName("gametests:grass_pen")
    .maxTicks(100);
}
