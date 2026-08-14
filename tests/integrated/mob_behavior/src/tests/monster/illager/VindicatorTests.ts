// 卫道士行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 卫道士近战攻击玩家致掉血（wiki tech_卫道士.txt#行为：卫道士手持铁斧近战攻击，伤害较高）。
//
// C++ 链路：VindicatorEntity : AbstractIllagerEntity，registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标
//   goalSelector 优先级4：MeleeAttackGoal(this, 1.0, false)——近战攻击。
// MeleeAttackGoal::checkAndPerformAttack 仅判定 distToEnemySqr<=attackReachSq && 冷却到 0 即攻击，
//   不检查主手武器。getAttackReachSqr=(attacker.width*2)² + target.width = (0.6*2)²+0.6 = 2.04，
//   开方约 1.43 格——玩家需在 1.43 格内。命中后 _attackTarget 读 ATTACK_DAMAGE 属性并 hurt 玩家。
//   冷却 ATTACK_COOLDOWN_TICKS=20，经 adjustedTickDelay 减半约 10 tick。
//
// 构造期补铁斧（关键对齐）：GameTest 的 test.spawn 不走 finalizeSpawn/populateDefaultEquipmentSlots，
// 故 VindicatorEntity 构造函数补主手铁斧（ItemStack(*Items::IRON_AXE,1)，isEmpty 守卫避免自然生成重复）。
// LivingEntity::detectEquipmentUpdates 在装备变化时把 ToolItem 的 ATTACK_DAMAGE 修饰符（铁斧 +9）
// 叠到属性上，徒手则只有基础 ATTACK_DAMAGE(5.0)。补铁斧后命中总伤害 5+9=14（普通难度，与原版一致）。
// 注意：MeleeAttackGoal._attackTarget 直接读 ATTACK_DAMAGE 属性（含装备修饰符）并 hurt，不调
//   MobEntity::attackEntityAsMob（后者会再走武器附魔路径），故伤害即属性值 14。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被阻挡 +
// 寻路通畅。卫道士是灾厄村民（非亡灵），构造期 setBurnsInDaylight(false) 关闭日光燃烧，白天即可
// 主动攻击（不 batch night）。卫道士(3,2,3) + Survival 玩家(4,2,3)，水平距 1 格 < 1.43 攻击范围，
//   无需寻路接近即直接命中（MeleeAttackGoal 在攻击范围内停止移动直接 checkAndPerformAttack）。
//
// 判定手段：断言玩家 HP 下降（<20）。卫道士伤害 14，玩家满血 20 → 6。首次命中即 HP<20。
// 确定型近战用"玩家掉血"判定（见 guardian-laser-deterministic-hit-test-strategy 确定型攻击判定策略）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验）：创造/旁观被 NearestAttackableTarget 滤掉。
// maxTicks=400：NearestAttackableTarget 选目标 + MeleeAttackGoal 锁定 + 首次冷却 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_卫道士.txt#行为（手持铁斧近战攻击）
function vindicatorAttacksPlayer(test: Test): void {
  // 卫道士 (3,2,3) + Survival 玩家 (4,2,3)，水平距 1 格 < 1.43 攻击范围。
  // 卫道士在攻击范围内直接 checkAndPerformAttack 命中（无需寻路接近）。
  // 灾厄村民不燃、无 FleeSun goal，白天默认环境即可攻击（不 batch night）。
  test.spawn("vindicator", { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标（约 tick 2-4，半 tick 评估）+ MeleeAttackGoal 锁定 +
  //   首次冷却（adjustedTickDelay(20)≈10 tick）命中，约 tick 15-30 玩家首伤（伤害 14 → HP=6）。
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
      `vindicator did not damage player, hp=${(health as any).currentValue}`);
  });
}

// 卫道士不在阳光下燃烧（wiki tech_卫道士.txt 通篇未提卫道士阳光下燃烧；卫道士是灾厄村民非亡灵）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 卫道士构造时 setBurnsInDaylight(false) 关闭日光燃烧（本次补齐，灾厄村民非亡灵不燃）。
// 与 zombie_burns_in_daylight（僵尸燃）+ ravager_does_not_burn_in_daylight（劫掠兽不燃）
// + pillager_does_not_burn_in_daylight（掠夺者不燃）对照：同为灾厄村民/亡灵的 MonsterEntity 子类，
// 僵尸燃 / 劫掠兽·掠夺者·卫道士不燃，交叉验证 m_burnsInDaylight 门控。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_卫道士.txt#行为（无阳光燃烧描述，卫道士不燃）
function vindicatorDoesNotBurnInDaylight(test: Test): void {
  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 卫道士 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const vindicator = test.spawn("vindicator", { x: 4, y: 2, z: 4 });

  // 白天露天卫道士不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，卫道士本就不燃，但留余量确保断言稳定。
  // 此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = vindicator.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("vindicator should not burn in daylight");
    }
  });
}

export function registerVindicatorTests(): void {
  GameTest.register("MobBehaviorTests", "vindicator_attacks_player", vindicatorAttacksPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "vindicator_does_not_burn_in_daylight", vindicatorDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
