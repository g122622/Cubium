// 蜜蜂行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 蜜蜂受击后反击玩家（wiki tech_蜜蜂.txt#攻击：蜜蜂是中立生物，受击或蜂巢被破坏后才攻击）。
//
// C++ 链路：BeeEntity : AnimalEntity + IAngerable（BeeEntity.cpp:374-437 registerGoals）：
//   targetSelector 优先级1：BeeAngerGoal(extends HurtByTargetGoal, alertAllies=true)
//     （BeeGoals.cpp:1086-1101）受击后 setRevengeTarget(攻击者) + setAngry(true)，
//     设 attackTarget=玩家。alertAllies=true 还会召唤附近蜜蜂群起攻击。
//   targetSelector 优先级2：BeeAttackPlayerGoal(chance=10)（BeeGoals.cpp:1107-1148）
//     愤怒且未螫刺时，10 格内搜索当前攻击目标玩家，setAttackTarget。
//   goalSelector 优先级0：BeeStingGoal(extends MeleeAttackGoal, speed=1.4, longMemory=true)
//     （BeeGoals.cpp:84-130）shouldExecute 检查 isAngry() && !hasStung()，接近后
//     checkAndPerformAttack→_attackTarget→hurt(玩家, ATTACK_DAMAGE=2.0)。
// registerAttributes（BeeEntity.cpp:439-458）：MAX_HEALTH=10, MOVEMENT_SPEED=0.3,
//   FLYING_SPEED=0.6, ATTACK_DAMAGE=2.0, FOLLOW_RANGE=48.0。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，蜜蜂飞行 + BeeStingGoal 寻路通畅。
// 蜜蜂(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。蜜蜂脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
// 玩家 tick 8 后 attackEntity(蜜蜂) 触发 BeeAngerGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 WolfTests/ZombifiedPiglinTests 同款注释）。
// 蜜蜂被攻击后设 attackTarget=玩家 + setAngry，BeeStingGoal 飞行接近 3 格 + 攻击冷却后 hurt(玩家, 2.0)。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 2.0，玩家满血 20 → 18。
// 蜜蜂 FLYING_SPEED=0.6 较快，3 格接近 + 攻击冷却（MeleeAttackGoal ATTACK_COOLDOWN_TICKS=20）
// 约需 30-60 tick。maxTicks=800 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#攻击（中立，受击后敌对攻击玩家）
function beeRetaliatesWhenAttacked(test: Test): void {
  const beeType = "bee";

  // 蜜蜂 (3,2,3)、Survival 玩家 (4,2,3)，水平距 1 格，同处结构 y=2 层。
  // 近距 1 格确保蜜蜂愤怒后 BeeStingGoal 立即进入攻击距离（getAttackReachSqr）。
  // 蜜蜂脚下 (3,1,3) 放玻璃支撑；玩家脚下 (4,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，蜜蜂反击飞行寻路通畅。
  test.setBlockType("minecraft:glass", { x: 3, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });
  const bee = test.spawn(beeType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击蜜蜂：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 BeeAngerGoal → startExecuting → setRevengeTarget(玩家)
  //   设 attackTarget=玩家 + setAngry(true)（BeeGoals.cpp BeeAngerGoal::startExecuting）。
  test.runAtTickTime(8, () => {
    player.attackEntity(bee);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + BeeAngerGoal 设目标 + BeeStingGoal 飞行接近 3 格 + 攻击冷却 + hurt(2.0)。
  // 蜜蜂 0.6 飞行速度接近 3 格约需 20-40 tick，maxTicks=800 留充裕余量。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `bee did not retaliate, hp=${(health as any).currentValue}`);
  });
}

// 蜜蜂蜇人后逐渐死亡（wiki tech_蜜蜂.txt#攻击：蜜蜂蜇人后会死亡，蜇刺留在玩家皮肤上）。
//
// C++ 链路（本次修复前为死代码，已修复）：BeeStingGoal::checkAndPerformAttack override
// （BeeGoals.cpp:115-130）在攻击命中（m_attackCooldown 由基类重置为正）后调
// m_beeEntity->setHasStung(true)。此前 BeeStingGoal::tick 仅转调基类、从不设 hasStung，
// 致 m_hasStung 恒 false、BeeEntity::tick() 螫刺后死亡分支不可达、蜜蜂可无限蜇人（与 vanilla 偏差）。
// 修复后 setHasStung(true) 激活 BeeEntity::tick()（BeeEntity.cpp:342-359）死亡链路：
//   m_hasStung 为 true 后，每 5 tick 概率死亡，概率 = 1/clamp(1200-timeSinceSting,1,1200)，
//   随时间递增，最长存活 1200 tick（60 秒）必死（timeSinceSting 达 1200 时 deathChance=1，必死）。
//
// 环境选择：creeper_pit（7×5×7）。蜜蜂(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。
// 玩家 tick 8 attackEntity(蜜蜂) 触发反击，蜜蜂蜇玩家后 setHasStung(true)，之后逐渐死亡。
//
// 判定手段：断言蜜蜂消失（getEntities type=bee length==0）。蜜蜂蜇人后必死，但死亡是概率性
// （最长 1200 tick=60s 必死）。maxTicks=1500 留余量（>1200 确保概率收敛到必死）。
// 注意：蜜蜂蜇人后 shouldExecute 的 !hasStung() 为 false，BeeStingGoal 停止，蜜蜂不再攻击，
// 但死亡链路由 tick() 驱动独立于 goal，故蜜蜂蜇一次后静待死亡。
// 蜜蜂查询用区域限定排除并行污染；type 用 "minecraft:bee"。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#攻击（蜇人后死亡）
function beeDiesAfterSting(test: Test): void {
  const beeType = "bee";

  // 蜜蜂 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const bee = test.spawn(beeType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 玩家攻击蜜蜂触发反击，蜜蜂蜇玩家后 setHasStung(true)。
  test.runAtTickTime(8, () => {
    player.attackEntity(bee);
  });

  // 断言蜜蜂消失：蜇人后逐渐死亡，maxTicks=1500 > 1200 确保概率收敛到必死。
  // 蜜蜂蜇人需先接近玩家（20-40 tick），之后死亡链路最长 1200 tick，总计约 1240 tick，1500 留余量。
  test.succeedWhen(() => {
    const bees = test.getDimension().getEntities({
      type: "minecraft:bee",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(bees.length === 0,
      `bee did not die after sting, count=${bees.length}`);
  });
}

// 蜜蜂跟随手持花朵的玩家（wiki tech_蜜蜂.txt#行为：蜜蜂会被手持花朵的玩家吸引）。
//
// C++ 链路：BeeEntity registerGoals goalSelector 优先级3：
//   TemptGoal(this, 1.25, lambda{item->isIn(ItemTags::FLOWERS)}, false)
//   （BeeEntity.cpp:393-401）诱惑物品=花朵标签。TemptGoal 经 getEntitiesInRange + dynamic_cast<Player*>
//   识别附近持花玩家（含 SimulatedPlayer），调 navigator()->moveTo(player) 驱动蜜蜂飞向玩家。
//   检测范围 TemptGoal 默认 10 格。蜜蜂飞行（IFlyingAnimal），mediumglass 走廊空间够飞行寻路。
//
// 环境选择：mediumglass（12×9×11，走廊 helper y=2,z=5,x=2..10 共 9 格，同 CowTests）。
// 玩家手持蒲公英（minecraft:dandelion，属 ItemTags::FLOWERS），蜜蜂 spawn 在走廊远端距玩家 8 格 < TemptRange 10。
//
// 判定手段：蜜蜂被诱惑后从 x=10 朝玩家 x=2 方向飞行。断言蜜蜂出现在玩家附近体积（x:2..6）即通过。
// 时序：TemptGoal 每 tick 评估（chance=0 跳过概率门控）+ 飞行寻路。蜜蜂 0.6 飞行速度接近 8 格约需 40-60 tick。
// maxTicks=1000 留充裕余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#行为（被手持花朵的玩家吸引）
function beeFollowsFlower(test: Test): void {
  const beeType = "bee";

  // 结构放置有 +1 抬升：helper-y=N 对应结构内 y=N-1。
  // mediumglass 内部空腔 helper y=2（结构内 y=1 空气），地板 helper y=1（结构内 y=0 圆石）。
  // 走廊 helper y=2, z=5, x=2..10（9 格）。玩家与蜜蜂分置走廊两端，距离 8 格 < TemptRange 10。
  const farmer = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 5 }, "farmer");

  // 主手持蒲公英：setItem 第三参 selectSlot=true 同步选中槽 0（主手），
  // 使 getHeldItem(MainHand) 返回蒲公英，TemptGoal 才能识别诱惑源。
  // 蒲公英属 ItemTags::FLOWERS，BeeEntity TemptGoal lambda 判定通过。
  const flower = new ItemStack("minecraft:dandelion", 1);
  // node_modules 中 @minecraft/server 存在两份（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // ItemStack 类型分裂致 setItem 形参类型不兼容；运行时两者均为同一 Cubium ItemStack opaque，强转绕过编译期。
  farmer.setItem(flower as unknown as Parameters<typeof farmer.setItem>[0], 0, true);

  // 蜜蜂 spawn 在走廊远端，距玩家 8 格，在 TemptRange(10) 内
  test.spawn(beeType, { x: 10, y: 2, z: 5 });

  // 蜜蜂被诱惑后从 x=10 朝玩家 x=2 方向飞行。断言蜜蜂出现在玩家附近体积（x:2..6）即通过。
  // 体积用 helper 坐标：from(x:2,y:2,z:4) to(x:6,y:4,z:6)，覆盖玩家附近 5×3×3 区域（蜜蜂飞行 y 跨度大）。
  test.succeedWhen(() => {
    assertEntityInVolume(test, beeType, 2, 2, 4, 6, 4, 6);
  });
}

export function registerBeeTests(): void {
  GameTest.register("MobBehaviorTests", "bee_retaliates_when_attacked", beeRetaliatesWhenAttacked)
    .structureName("gametests:creeper_pit")
    .maxTicks(800);

  GameTest.register("MobBehaviorTests", "bee_dies_after_sting", beeDiesAfterSting)
    .structureName("gametests:creeper_pit")
    .maxTicks(1500);

  GameTest.register("MobBehaviorTests", "bee_follows_flower", beeFollowsFlower)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);
}
