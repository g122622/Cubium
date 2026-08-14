// 掠夺者行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit / grass_pen 结构尺寸（7×5×7 / 9×5×9），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 掠夺者用弩远程射击玩家致掉血（wiki tech_掠夺者.txt#行为：掠夺者使用弩进行远程攻击）。
//
// C++ 链路：PillagerEntity : AbstractIllagerEntity，实现 ICrossbowUser。registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标
//   goalSelector 优先级3：RangedCrossbowAttackGoal(this, 1.0, 8.0)——弩远程攻击，攻击半径 8 格。
// RangedCrossbowAttackGoal 状态机：Uncharged→Charging(装填25tick)→Charged→ReadyToAttack→发射。
//   shouldExecute 守卫 _isHoldingCrossbow()（主手 getUseAction==Crossbow）+ attackTarget 存活。
//   _handleChargingState 用 m_chargeTime 计数达 getCrossbowChargeTime(25) 后 setCharged(true)。
//   _handleReadyToAttackState canSee 时调 shootCrossbow 发射弩箭。装填/发射均每 tick tick（tick()
//   不受 GoalSelector 半 tick 节流，m_chargeTime 每 tick 递增，装填时间准确 25 tick）。
// PillagerEntity::shootCrossbow 凭空 make_unique<ArrowEntity>（不消耗弹药），setDamage(5.0)，
//   setShotFromCrossbow(true)，shoot 朝目标。箭矢命中玩家造成 5 点伤害。
//
// 构造期补弩（关键）：GameTest 的 test.spawn 不走 finalizeSpawn/populateDefaultEquipmentSlots，
// 故 PillagerEntity 构造函数补主手弩（ItemStack(*Items::CROSSBOW,1)，isEmpty 守卫避免自然生成重复）。
// 不补弩则 _isHoldingCrossbow()=false，RangedCrossbowAttackGoal::shouldExecute 永不通过，掠夺者徒手
// 不射击。补弩后整条弩攻击链路启动。新弩默认未装填，goal 状态机自驱完成 25 tick 装填再发射。
//
// 环境选择：creeper_pit（7×5×7 开放坑）无围墙，NearestAttackableTarget checkSight 射线不被阻挡 +
// 寻路通畅。掠夺者是灾厄村民（非亡灵），构造期 setBurnsInDaylight(false) 关闭日光燃烧，且无
// FleeSun/RestrictSun goal，白天即可主动射击（与骷髅/流浪者/沼骸必须 batch("night") 不同）。
// 掠夺者(1,2,1) + Survival 玩家(5,2,5)，水平距 √(4²+4²)≈5.66 格 < 8 攻击半径，在弩射程内。
//
// 判定手段：断言玩家 HP 下降（<20）。弩箭伤害 5.0（shootCrossbow setDamage），玩家满血 20 → 15。
// 首次命中即 HP<20。确定型用"玩家掉血"判定（见 guardian-laser-deterministic-hit-test-strategy
// 确定型攻击判定策略）。不直接断言箭矢实体出现（箭矢飞行命中后消失，getEntities 轮询撞窗口不稳）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验）：创造/旁观被 NearestAttackableTarget 滤掉。
// maxTicks=400：装填 25 tick + CHARGED_WAIT(20-40) + canSee 判定 + 箭矢飞行 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_掠夺者.txt#行为（使用弩远程攻击）
function pillagerShootsArrowAtPlayer(test: Test): void {
  // 掠夺者 (1,2,1)（一角），Survival 玩家 (5,2,5)（对角，距 ~5.66 格 < 8 弩攻击半径）。
  // 掠夺者在弩射程内锁定玩家后装填+发射。玩家被弩箭命中掉血（伤害 5.0）。
  // 灾厄村民不燃、无 FleeSun goal，白天默认环境即可射击（不 batch night）。
  test.spawn("pillager", { x: 1, y: 2, z: 1 });
  test.spawnSimulatedPlayer({ x: 5, y: 2, z: 5 }, "bait", 0 as any);

  // 断言玩家掉血：succeedWhen 每 tick 持续检查玩家 HP<20。
  // 时序：NearestAttackableTarget 选目标 + seenTime>=5（约 tick 5）+ 装填 25 tick（约 tick 30）
  //   + CHARGED_WAIT 20-40（约 tick 50-70）+ canSee 发射 + 箭矢飞行几 tick命中，约 tick 60-80 玩家首伤。
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
      `pillager did not shoot player, hp=${(health as any).currentValue}`);
  });
}

// 掠夺者不在阳光下燃烧（wiki tech_掠夺者.txt 通篇未提掠夺者阳光下燃烧；掠夺者是灾厄村民非亡灵）。
//
// C++ 链路：MonsterEntity::tick→handleDaylightBurning→if(m_burnsInDaylight) burnUndead()。
// 掠夺者构造时 setBurnsInDaylight(false) 关闭日光燃烧（本次补齐，灾厄村民非亡灵不燃）。
// 与 zombie_burns_in_daylight（僵尸燃）+ ravager_does_not_burn_in_daylight（劫掠兽不燃）对照：
// 同为 MonsterEntity 子类，僵尸燃 / 劫掠兽不燃 / 掠夺者不燃，交叉验证 m_burnsInDaylight 门控。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_掠夺者.txt#行为（无阳光燃烧描述，掠夺者不燃）
function pillagerDoesNotBurnInDaylight(test: Test): void {
  // 结构 grass_pen（9×5×9）：y=0 满铺 grass_block，y=1..3 玻璃墙+内空气，y=4 全 air 露天。
  // 掠夺者 spawn 于 (4,2,4)（结构内 y=1 空气腔，头顶 y=2/y=3 air、y=4 露天 → canSeeSky=true）。
  // skyAccess(true) 清空结构上方 worldgen 制造露天列使 canSeeSky=true（见 SkeletonTests 同款注释）。
  // setupTicks(20) 让光照重算稳定（清空上方方块后 skyLight 入队需 tick 重算）。
  // 不显式标注 Entity 类型：test.spawn 返回类型来自内嵌 @minecraft/server，与顶层包 Entity
  // 因 Dimension 属性差异不兼容，显式标注触发 TS2322（见 SkeletonTests 同款注释）。
  const pillager = test.spawn("pillager", { x: 4, y: 2, z: 4 });

  // 白天露天掠夺者不着火：轮询 onfire 组件，应恒 undefined（setBurnsInDaylight=false）。
  // maxTicks=400：白天燃烧判定每 tick 概率触发，掠夺者本就不燃，但留余量确保断言稳定。
  // 此为负向断言（assert 不着火），有 zombie_burns_in_daylight 正向断言对照互补验证。
  test.succeedWhen(() => {
    const fire = pillager.getComponent("minecraft:onfire");
    if (fire !== undefined) {
      throw new Error("pillager should not burn in daylight");
    }
  });
}

export function registerPillagerTests(): void {
  GameTest.register("MobBehaviorTests", "pillager_shoots_arrow_at_player", pillagerShootsArrowAtPlayer)
    .structureName("gametests:creeper_pit")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "pillager_does_not_burn_in_daylight", pillagerDoesNotBurnInDaylight)
    .structureName("gametests:grass_pen")
    .skyAccess(true)
    .setupTicks(20)
    .maxTicks(400);
}
