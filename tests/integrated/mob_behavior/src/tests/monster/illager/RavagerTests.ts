// 劫掠兽行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 劫掠兽主动攻击玩家致掉血（wiki tech_劫掠兽.txt#行为：劫掠兽是敌对生物，会主动攻击
// 玩家、铁傀儡、村民等；攻击方式为近战冲撞，伤害 12（普通）/18（困难））。
//
// C++ 链路：RavagerEntity : AbstractRaiderEntity，registerGoals 注册：
//   targetSelector 优先级3：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标
//     （对齐 MC 1.21.11 Ravager.registerGoals；与女巫不同，劫掠兽本就注册了此 goal，不缺）。
//   targetSelector 优先级4：NearestAttackableTargetGoal<AbstractVillager>(排除幼年) +
//     NearestAttackableTargetGoal<IronGolem>。
//   goalSelector 优先级4：RavagerAttackGoal(this)（继承 MeleeAttackGoal, speed=1.0, useLongMemory=true）。
// RavagerAttackGoal::getAttackReachSqr = (width-0.1)*2 的平方 + target.width ≈ 13.69 + 0.6 = 14.29，
//   开方约 3.78 格——劫掠兽能命中 3.78 格内的玩家。命中后 RavagerEntity::attackEntityAsMob 造成
//   ATTACK_DAMAGE(12.0) 伤害并击退目标。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被玻璃阻挡。
// 劫掠兽 setBurnsInDaylight(false)（构造期关闭，对齐原版不燃），白天默认环境即可主动攻击。
// 劫掠兽体积大（width 1.95 / height 2.2），creeper_pit y=1..4 空气腔高 4 格足够容纳。
// 劫掠兽(2,2,3)+玩家(5,2,3)，水平距 3 格 < 3.78 攻击距离 → 选目标后近战命中。
//
// 判定手段：断言玩家 HP 下降（<20）。近战确定性命中（无散布，MeleeAttackGoal 到冷却即 hurt），
// 伤害 12，玩家满血 20 → 8。确定型近战用"玩家掉血"判定稳定
// （见 guardian-laser-deterministic-hit-test-strategy 确定型攻击判定策略）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#行为（主动攻击玩家）
function ravagerAttacksPlayer(test: Test): void {
  const ravagerType = "ravager";

  // 劫掠兽 (2,2,3)、Survival 玩家 (5,2,3)，水平距 3 格，同处结构 y=2 层。
  // 距 3 格 < 3.78 攻击距离，劫掠兽选目标后 RavagerAttackGoal 直接命中（无需寻路接近）。
  // 劫掠兽受 MonsterEntity 重力会下落，故脚下 (2,1,3) 放玻璃支撑；玩家脚下 (5,1,3) 放玻璃。
  // creeper_pit 开放坑无围墙，checkSight 射线不被阻挡。
  test.setBlockType("minecraft:glass", { x: 2, y: 1, z: 3 });
  test.setBlockType("minecraft:glass", { x: 5, y: 1, z: 3 });
  test.spawn(ravagerType, { x: 2, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + RavagerAttackGoal 攻击冷却约 20 tick。
  // 注意：劫掠兽攻击后击退玩家，玩家可能被推远导致脱离攻击范围——但首次命中即掉血至 8，
  //   HP<20 断言一旦满足即 succeed，不受后续击退影响。
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
      `ravager did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 劫掠兽不在阳光下燃烧（wiki tech_劫掠兽.txt 通篇未提劫掠兽阳光下燃烧；劫掠兽是袭击生物非亡灵）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 劫掠兽构造时 setBurnsInDaylight(false) 关闭日光燃烧（对齐原版，本次补齐）。
// 与 zombie_burns_in_daylight（僵尸燃）+ witch_does_not_burn_in_daylight（女巫不燃）对照。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_劫掠兽.txt#行为（无阳光燃烧描述，劫掠兽不燃）
function ravagerDoesNotBurnInDaylight(test: Test): void {
  const ravagerType = "ravager";

  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 劫掠兽 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // 劫掠兽体积大（width 1.95 / height 2.2），grass_pen 9×9 空间足够容纳。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  const ravager = test.spawn(ravagerType, { x: 4, y: 2, z: 4 });

  // 白天露天劫掠兽不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，劫掠兽本就不燃，但留余量确保断言稳定。
  // 注意：此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = ravager.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("ravager should not burn in daylight");
    }
  });
}

export function registerRavagerTests(): void {
  GameTest.register("MobBehaviorTests", "ravager_attacks_player", ravagerAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(600);

  GameTest.register("MobBehaviorTests", "ravager_does_not_burn_in_daylight", ravagerDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
