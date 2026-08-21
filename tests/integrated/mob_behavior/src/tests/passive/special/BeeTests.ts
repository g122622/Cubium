// 蜜蜂行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
// 蜜蜂反击/蛰击/死亡三测试均用 grass_pen 封闭围栏（玻璃墙物理限制蜜蜂不飞出查询区，
// 消除 MeleeAttackGoal 20-tick 节流窗口内蜜蜂飞走出区域的 flaky，详见各测试环境选择说明）。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

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
//     checkAndPerformAttack→_attackTarget→attackEntityAsMob(玩家)（本次新增 override，
//     调父类 hurt(ATTACK_DAMAGE=2.0) + 施加中毒 + setHasStung + stopBeingAngry + BEE_STING 音效）。
// registerAttributes（BeeEntity.cpp:439-458）：MAX_HEALTH=10, MOVEMENT_SPEED=0.3,
//   FLYING_SPEED=0.6, ATTACK_DAMAGE=2.0, FOLLOW_RANGE=48.0。
//
// 环境选择：grass_pen（9×5×9 玻璃墙封闭围栏，y=0 grass_block 地板 + y=1..3 外圈玻璃墙+内空气）。
// 蜜蜂(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。**距离 3 格**（非 1 格）：
// bee_dies_after_sting 已验证 3 格距离下蜜蜂能稳定接近蛰击玩家；1 格距离下蜜蜂受击后
// 易触发 PanicGoal/飞行振荡飞出 7×7 区域被卸载（bees=0）且蛰击不稳定。3 格给蜜蜂
// 飞行寻路稳定接近的空间。蜜蜂脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
//
// **必须用 grass_pen 封闭围栏而非 creeper_pit 开放坑**（修正前期 flaky 根因）：
// MeleeAttackGoal::shouldExecute 有 20-tick 节流（TARGET_CHECK_COOLDOWN，MeleeAttackGoal.cpp:57-61），
// m_lastCheckTime 初值0，蜜蜂受击后前 20 tick BeeStingGoal::shouldExecute 恒被节流返回 false。
// 这 20 tick 窗口蜜蜂由 BeeWanderGoal 接管随机飞行（FLYING_SPEED=0.6，20 tick 可飞 12 格）。
// creeper_pit 是 7×5×7 开放坑无围墙，蜜蜂能在节流窗口内飞出 7×7 查询区被卸载（bees=0 假阳性），
// 节流窗口后路径失败 shouldExecute 持续 false，BeeStingGoal 永不启动——约 40% 概率蜜蜂飞走出区域
// 致 bee_sting_applies_poison 失败（poison=undefined playerHp=20 bees=0）。grass_pen 外圈玻璃墙
// 把蜜蜂物理限制在 9×5×9 内腔，蜜蜂撞墙停留墙边，节流窗口后 BeeStingGoal 寻路回玩家必然蛰击，
// 消除飞走出区域的概率性失败。dies_after_sting 宽松判定（bees===0 把飞走当通过）掩盖了同一根因，
// 一并改用 grass_pen 收敛。
// 玩家 tick 8 后 attackEntity(蜜蜂) 触发 BeeAngerGoal 反击（attackEntity 不受距离限制，
// 基岩语义 attack can be performed at any distance，见 WolfTests/ZombifiedPiglinTests 同款注释）。
// 蜜蜂被攻击后设 attackTarget=玩家 + setAngry，BeeStingGoal 飞行接近 3 格 + 攻击冷却后
// attackEntityAsMob(玩家) 造成 2.0 伤害 + 施加中毒（中毒判定见 bee_sting_applies_poison）。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布），伤害 2.0，玩家满血 20 → 18。
// 蜜蜂 FLYING_SPEED=0.6 较快，3 格接近 + 20-tick 节流窗口 + 攻击冷却（ATTACK_COOLDOWN_TICKS=20）
// 约需 40-60 tick。maxTicks=1500 留充裕余量吸收 BeeWanderGoal 振荡非确定性。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击/反击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#攻击（中立，受击后敌对攻击玩家）
function beeRetaliatesWhenAttacked(test: Test): void {
  const beeType = "bee";

  // 蜜蜂 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格（与 bee_dies_after_sting 同范式）。
  // 3 格距离给蜜蜂飞行寻路稳定接近的空间，避免 1 格距离下受击振荡飞出区域。
  // grass_pen 外圈玻璃墙封闭防蜜蜂飞出查询区（见上方环境选择说明）。
  // 蜜蜂脚下 (2,1,3) 与玩家脚下 (5,1,3) 放 glass 支撑（grass_pen y=1 是内空气腔，需脚下方块防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const bee = test.spawn(beeType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 后玩家攻击蜜蜂：留 8 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 BeeAngerGoal → startExecuting → setRevengeTarget(玩家)
  //   设 attackTarget=玩家 + setAngry(true)（BeeGoals.cpp BeeAngerGoal::startExecuting）。
  test.runAtTickTime(8, () => {
    player.attackEntity(bee);
  });

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：玩家攻击(8) + 20-tick BeeStingGoal 节流窗口 + BeeStingGoal 飞行接近 3 格 + 攻击冷却 + hurt(2.0)。
  // 蜜蜂 0.6 飞行速度接近 3 格约需 20-40 tick，maxTicks=1500 留充裕余量吸收 Wander 振荡。
  // 玩家查询用区域限定排除并行测试的玩家污染；type 用 "minecraft:player"（玩家类型带前缀）。
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
// 环境选择：grass_pen（9×5×9 玻璃墙封闭围栏）。蜜蜂(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。
// 玩家 tick 8 attackEntity(蜜蜂) 触发反击，蜜蜂蜇玩家后 setHasStung(true)，之后逐渐死亡。
// 同 bee_retaliates_when_attacked 用 grass_pen 封闭腔（非 creeper_pit 开放坑），消除蜜蜂节流窗口
// 飞走出区域的概率性——dies_after_sting 此前用 creeper_pit + 宽松判定（bees===0 把飞走当通过）
// 掩盖了与 sting_applies_poison 同根因的 flaky，此处一并收敛。
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
  // 脚下放 glass 支撑（grass_pen y=1 是内空气腔，需脚下方块防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const bee = test.spawn(beeType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "attacker", 0 as any);

  // tick 8 玩家攻击蜜蜂触发反击，蜜蜂蜇玩家后 setHasStung(true)。
  test.runAtTickTime(8, () => {
    player.attackEntity(bee);
  });

  // 断言蜜蜂消失：蜇人后逐渐死亡，maxTicks=1500 > 1200 确保概率收敛到必死。
  // 蜜蜂蜇人需先接近玩家（20-tick 节流 + 20-40 tick 飞行接近），之后死亡链路最长 1200 tick，
  // 总计约 1260 tick，1500 留余量。
  test.succeedWhen(() => {
    const bees = test.getDimension().getEntities({
      type: "minecraft:bee",
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    test.assert(bees.length === 0,
      `bee did not die after sting, count=${bees.length}`);
  });
}

// 蜜蜂蛰击玩家后对其施加中毒效果（wiki tech_蜜蜂.txt#攻击：蜜蜂蜇人后，玩家会中毒，
// 蜇刺留在皮肤上持续造成伤害）。
//
// C++ 链路（本次新增，对齐 Java 1.21.11 Bee.doHurtTarget，Bee.java:227-252）：
//   BeeStingGoal(extends MeleeAttackGoal) 接近玩家后 checkAndPerformAttack→_attackTarget
//   → m_creature->attackEntityAsMob(*target)（MeleeAttackGoal.cpp:278，虚派发）。
//   BeeEntity::attackEntityAsMob override（本次新增）：
//     1. 调父类 AnimalEntity::attackEntityAsMob（解析到 MobEntity 基类）执行 sting 伤害(2.0)；
//     2. 命中后 setHasStung(true) + setAngry(false) + setAttackTarget(nullptr)（stopBeingAngry）；
//     3. 按难度施加中毒：Normal 10s / Hard 18s（Easy/Peaceful 不中毒），EffectType::Poison 等级0；
//     4. playSound(BEE_STING)。
//   此前 BeeEntity 无 attackEntityAsMob override，蛰击仅造成 2.0 纯伤害——不施加中毒（对齐缺陷）。
//   注：MeleeAttackGoal 已委托 attackEntityAsMob（记忆 [[meleeattackgoal-delegates-attackentityasmob]]），
//   故 BeeEntity override 自动生效，无需改 BeeStingGoal。BeeStingGoal.checkAndPerformAttack 的
//   setHasStung 兜底与 attackEntityAsMob 内 setHasStung 去重（后者先执行置 true，前者 !hasStung() 为 false 跳过）。
//
// 环境选择：grass_pen（9×5×9 玻璃墙封闭围栏）。蜜蜂(2,2,3)+Survival 玩家(5,2,3)，水平距 3 格。
// **距离 3 格**（非 1 格）：bee_dies_after_sting 已验证 3 格距离下蜜蜂能稳定接近蛰击玩家。
// 1 格距离下蜜蜂受击后易触发 PanicGoal/飞行振荡飞出 7×7 区域被卸载（bees=0）且未进入稳定
// 攻击距离，蛰击不触发。3 格给蜜蜂飞行寻路稳定接近的空间，与 dies_after_sting 同范式。
//
// **必须用 grass_pen 封闭围栏而非 creeper_pit 开放坑**（修正前期 40% flaky 根因）：
// MeleeAttackGoal::shouldExecute 有 20-tick 节流（TARGET_CHECK_COOLDOWN，MeleeAttackGoal.cpp:57-61），
// 蜜蜂受击后前 20 tick BeeStingGoal::shouldExecute 恒被节流返回 false，蜜蜂由 BeeWanderGoal
// 接管随机飞行（FLYING_SPEED=0.6，20 tick 可飞 12 格）。creeper_pit 7×7 开放坑无围墙，蜜蜂能在
// 节流窗口飞出查询区被卸载（bees=0），节流后路径失败 shouldExecute 持续 false，BeeStingGoal 永不
// 启动——约 40% 失败（poison=undefined playerHp=20 bees=0）。grass_pen 外圈玻璃墙物理限制蜜蜂在
// 9×5×9 内腔，节流窗口后 BeeStingGoal 寻路回玩家必然蛰击，消除飞走出区域的概率性失败。
// 蜜蜂脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
// GameTestServer 默认 Normal 难度，故中毒持续 10s=200 tick。
//
// 判定手段：玩家攻击蜜蜂→蜜蜂反击蛰击→玩家获得 Poison 效果。pollUntilSucceed 轮询
// player.getEffect("poison") !== undefined。getEffect 已在 Entity + SimulatedPlayer 两处绑定
// （SimulatedPlayer 不继承 Entity 原型，记忆 [[simulated-player-js-class-no-entity-inheritance]]，
// getEffect 在 ScriptSimulatedPlayer.cpp 重绑）。
// 时序：玩家攻击(8) + 20-tick BeeStingGoal 节流窗口 + BeeStingGoal 飞行接近 3 格 + 蛰击(attackEntityAsMob 施毒)。
//   3 格接近约需 20-40 tick，中毒从蛰击起持续 200 tick（至 ~268 tick）。
//   startTick=10 起轮询，maxTick=1500 留足节流(20)+蛰击(40)+中毒窗口(200)+Wander振荡余量。
//   蜜蜂蛰击后 hasStung=true 停止攻击，但中毒已在玩家身上，判定不依赖蜜蜂后续行为。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#攻击（蜇人后玩家中毒）
function beeStingAppliesPoison(test: Test): void {
  const beeType = "bee";

  // 蜜蜂 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格（与 bee_dies_after_sting 同范式）。
  // 3 格距离给蜜蜂飞行寻路稳定接近的空间，避免 1 格距离下受击振荡飞出区域。
  // grass_pen 外圈玻璃墙封闭防蜜蜂飞出查询区（见上方环境选择说明）。
  // 脚下放 glass 支撑（grass_pen y=1 是内空气腔，需脚下方块防下落）。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  const bee = test.spawn(beeType, { x: 2, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "stingVictim", 0 as any);

  // tick 8 玩家攻击蜜蜂触发 BeeAngerGoal → setRevengeTarget(玩家) + setAngry(true)。
  test.runAtTickTime(8, () => {
    player.attackEntity(bee);
  });

  // 轮询：玩家获得 Poison 效果（蜜蜂蛰击后 attackEntityAsMob 施加）。
  // getEffect("poison") 返回 Effect 对象（{ typeId, amplifier, duration }）或 undefined。
  // Normal 难度中毒 200 tick。蜜蜂受击后反击蛰击受 20-tick BeeStingGoal 节流窗口 + Wander 振荡
  // 影响，是概率性时序，maxTick=1500 给足蜜蜂最终蛰击的窗口（与 bee_dies_after_sting 同范式验证稳定）。
  // 中毒一旦施加持续 200 tick，pollUntilSucceed 立即捕获。
  pollUntilSucceed(test, () => {
    const poison = (player as any).getEffect("poison");
    return poison !== undefined;
  }, {
    startTick: 10,
    interval: 2,
    maxTick: 1500,
    onTimeout: () => {
      const poison = (player as any).getEffect("poison");
      const health = player.getComponent("minecraft:health") as any;
      const hp = health?.currentValue;
      const bees = test.getDimension().getEntities({
        type: "minecraft:bee",
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `bee sting did not apply poison: poison=${JSON.stringify(poison)} playerHp=${hp} bees=${bees.length}`);
    },
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

// 两只蜜蜂各喂花朵后进入爱心状态，BreedGoal 驱动互相靠近并繁殖出小蜜蜂
// （wiki tech_蜜蜂.txt#繁殖：手持花朵右键两只成年蜜蜂使其进入"爱心模式"，两只蜜蜂靠近后繁殖出小蜜蜂，
//   双亲进入 5 分钟繁殖冷却，玩家获得 1-7 经验球）。
//
// C++ 链路（对齐 MC Java 1.21.11 Bee + BreedGoal，与 cow_breeds_when_fed_wheat 同构）：
//   1) 玩家主手持花朵 + interactWithEntity(bee) → Player::interactOn → bee.processInitialInteract
//      → MobEntity::interactMob → AnimalEntity::interactMob override（AnimalEntity.cpp:90-141）：
//      BeeEntity::isBreedingItem(花朵) 命中（BeeEntity.cpp:282-290 item->isIn(ItemTags::FLOWERS)）
//      → 成体 canBreed() → setInLove(player.playerId())。创造模式喂食不消耗花朵，同一朵花喂两只蜜蜂。
//   2) BreedGoal::shouldExecute（BreedGoal.cpp:62-74）：isInLove() && findNearbyMate() 非空。
//   3) BreedGoal::tick：navigator.moveTo(配偶) + m_spawnBabyDelay++，达 adjustedTickDelay(60)=30
//      且 distSq<BREED_DISTANCE_SQ=9 时 spawnBaby()。
//   4) BeeEntity::spawnBaby（BeeEntity.cpp:292-...）：构造 BeeEntity 幼体 + setChild(true)；
//      BreedGoal:153 兜底 setTypeId(BEE) 保证 getEntities 可查。
//
// 优先级分析（BeeEntity.cpp:374-439 registerGoals）：BreedGoal 优先级2，仅次于 BeeStingGoal(0，需
//   isAngry()&&!hasStung() 才触发，本测试未激怒蜜蜂不触发) 与 BeeEnterHiveGoal(1，需有蜂巢 hivePos，
//   本测试无蜂巢不触发)。故无蜂巢未愤怒时 BreedGoal(2) 是最高可执行 goal，isInLove 时独占驱动繁殖。
//   BeeWanderGoal(8) 飞行游荡优先级远低于 BreedGoal(2)，繁殖期被 mutex 阻塞不干扰。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏）。蜜蜂是飞行生物（IFlyingAnimal），grass_pen 玻璃封闭内腔
//   9×5×9 防蜜蜂飞出查询区域。两只蜜蜂放中心 (4,2,4) 与 (4,2,6) 相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，
//   已在繁殖距离内）。蜜蜂 FLYING_SPEED=0.6，BreedGoal speed=1.0，moveTo 配偶快。
//
// 判定手段：繁殖完成后区域内 bee 数 >=3（原 2 + 幼体 1）。pollUntilSucceed 轮询。
// 时序：喂食 2×（tick 5、10）+ BreedGoal 评估 + 30 tick spawnBabyDelay + 余量。startTick=30，maxTick=700。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蜜蜂.txt#繁殖（喂花→爱心→繁殖小蜜蜂+冷却+经验球）
function beeBreedsWhenFedFlower(test: Test): void {
  const beeType = "bee";

  // 两只成年蜜蜂放中心相距 2 格（distSq=4 < BREED_DISTANCE_SQ=9，已在繁殖距离内）。
  // 脚下 y=1 grass_block 支撑（蜜蜂飞行不需支撑，但 grass_pen 地板 grass_block 在 y=0，y=1 air 腔）。
  const bee1 = test.spawn(beeType, { x: 4, y: 2, z: 4 });
  const bee2 = test.spawn(beeType, { x: 4, y: 2, z: 6 });

  // 创造玩家持蒲公英（属 ItemTags::FLOWERS）：创造模式喂食不消耗花朵（同一朵花喂两只蜜蜂）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 4 }, "beeBreeder");
  const flower = new ItemStack("minecraft:dandelion", 1);
  // 两份 @minecraft/server ItemStack 类型分裂（顶层 1.13-beta 与 server-gametest 嵌套 1.19），
  // setItem 形参类型不兼容；运行时同一 Cubium ItemStack opaque，强转绕过编译期。
  player.setItem(flower as unknown as Parameters<typeof player.setItem>[0], 0, true);

  // 依次喂两只蜜蜂：interactWithEntity 转发 interactOn → AnimalEntity::interactMob → setInLove。
  test.runAtTickTime(5, () => {
    (player as any).interactWithEntity(bee1);
  });
  test.runAtTickTime(10, () => {
    (player as any).interactWithEntity(bee2);
  });

  // 轮询：繁殖完成后区域内 bee 数 >=3（原 2 + 幼体 1）。
  pollUntilSucceed(test, () => {
    const bees = test.getDimension().getEntities({
      type: beeType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    return bees.length >= 3;
  }, {
    startTick: 30,
    interval: 10,
    maxTick: 700,
    onTimeout: () => {
      const bees = test.getDimension().getEntities({
        type: beeType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      test.assert(false,
        `bee did not breed: beeCount=${bees.length} (expected >=3 after feeding flower)`);
    },
  });
}

export function registerBeeTests(): void {
  GameTest.register("MobBehaviorTests", "bee_retaliates_when_attacked", beeRetaliatesWhenAttacked)
    .structureName("gametests:grass_pen")
    .maxTicks(1500);

  GameTest.register("MobBehaviorTests", "bee_dies_after_sting", beeDiesAfterSting)
    .structureName("gametests:grass_pen")
    .maxTicks(1500);

  GameTest.register("MobBehaviorTests", "bee_sting_applies_poison", beeStingAppliesPoison)
    .structureName("gametests:grass_pen")
    .maxTicks(1800);

  GameTest.register("MobBehaviorTests", "bee_follows_flower", beeFollowsFlower)
    .structureName("gametests:mediumglass")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "bee_breeds_when_fed_flower", beeBreedsWhenFedFlower)
    .structureName("gametests:grass_pen")
    .maxTicks(700);
}
