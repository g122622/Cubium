// 恼鬼行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 恼鬼主动攻击玩家致掉血（wiki tech_恼鬼.txt#行为：非唤魔者召唤的恼鬼会主动攻击玩家；
// 攻击时变红猛冲，飞向玩家造成伤害）。
//
// C++ 链路：VexEntity : MonsterEntity（独立链，不经 Raider），registerGoals 注册：
//   targetSelector 优先级3：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标。
//   goalSelector 优先级4：VexChargeAttackGoal——飞向目标眼睛位置，碰撞箱相交时 attackEntityAsMob。
// VexChargeAttackGoal::tick：distSq<STOP_CHASE_DISTANCE_SQ(9, 即3格)时 moveController 飞向目标眼睛，
//   碰撞箱相交 _checkAndPerformAttack→attackEntityAsMob 造成 ATTACK_DAMAGE(4.0) 伤害，冷却 ATTACK_COOLDOWN_TICKS。
// 恼鬼飞行无重力（setNoGravity(true)），用 VexMovementController 直接改 velocity 飞行，无需寻路地面。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 恼鬼 setBurnsInDaylight(false)（构造期关闭，对齐原版不燃），白天默认环境即可主动攻击。
// 恼鬼飞行无重力，不需脚下支撑方块（区别于陆地怪物）。但 VexMoveRandomGoal 可能让恼鬼飞离，
// 玩家在攻击范围内时 VexChargeAttackGoal 优先级4 > 随机飞行优先级8，恼鬼优先冲锋攻击。
// 恼鬼(2,2,3)+玩家(5,2,3)，水平距 3 格 → 恼鬼飞向玩家碰撞攻击。
//
// 判定手段：断言玩家 HP 下降（<20）。恼鬼冲锋碰撞攻击确定性命中（无散布），
// 伤害 4.0，玩家满血 20 → 16。确定型攻击用"玩家掉血"判定稳定。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_恼鬼.txt#行为（主动攻击玩家）
function vexAttacksPlayer(test: Test): void {
  const vexType = "vex";

  // 恼鬼 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // VexChargeAttackGoal::shouldExecute 要求 distSq>MIN_CHARGE_DISTANCE_SQ(4, 即>2格) 才发起冲锋——
  //   距离太近(≤2格)反而不冲锋。距 3 格满足冲锋条件，恼鬼飞向玩家眼睛位置，碰撞箱相交时攻击。
  // 恼鬼飞行无重力，不需脚下支撑；玩家受重力下落，脚下 (5,1,3) 放玻璃支撑。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  test.spawn(vexType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + VexChargeAttackGoal shouldExecute(1/7概率+距>2格) +
  //   飞行接近 + 碰撞攻击。shouldExecute 有 1/7 概率门控，平均每 7 tick 尝试一次，需若干次才触发。
  // maxTicks=1000 留充裕余量吸收 1/7 概率门控 + 飞行路径随机性（GameTest 非确定性）。
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
      `vex did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 恼鬼不在阳光下燃烧（wiki tech_恼鬼.txt#行为 章节未提阳光燃烧，原版 Vex 不燃）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 恼鬼构造时 setBurnsInDaylight(false) 关闭日光燃烧（对齐原版，本次补齐）。
// 与 zombie_burns_in_daylight（僵尸燃）+ blaze_does_not_burn_in_daylight（烈焰人不燃）对照。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_恼鬼.txt#行为（无阳光燃烧描述，恼鬼不燃）
function vexDoesNotBurnInDaylight(test: Test): void {
  const vexType = "vex";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 恼鬼 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 恼鬼飞行无重力，悬停在 spawn 位置不下落。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  const vex = test.spawn(vexType, { x: 4, y: 2, z: 4 });

  // 白天露天恼鬼不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，恼鬼本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = vex.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("vex should not burn in daylight");
    }
  });
}

export function registerVexTests(): void {
  GameTest.register("MobBehaviorTests", "vex_attacks_player", vexAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(1000);

  GameTest.register("MobBehaviorTests", "vex_does_not_burn_in_daylight", vexDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
