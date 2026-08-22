// 生物死亡经验掉落 GameTest。
//
// 覆盖 LivingEntity.dropExperience 经验掉落守卫（对齐 MC Java 1.21.11
// LivingEntity.java:1498-1506）。修复前 Cubium MobEntity::dropExperience 不查守卫直接掉落经验球，
// 致两种偏差：(1) 非玩家击杀（test.kill 虚空伤害无玩家来源）的普通生物仍掉经验球；(2) doMobLoot=false
// 时仍掉经验球。vanilla 守卫：!wasExperienceConsumed && (isAlwaysExperienceDropper ||
// (lastHurtByPlayerMemoryTime>0 && shouldDropExperience && doMobLoot))——普通生物需被玩家伤害过
// （100 tick 记忆窗口内死亡）且 doMobLoot=true 才掉经验。
//
// 本次补齐：LivingEntity 加 m_lastHurtByPlayerMemoryTime 字段（受玩家伤害设 100，每 tick 递减）+
// shouldDropExperience/isAlwaysExperienceDropper/wasExperienceConsumed/skipDropExperience 虚函数 +
// shouldDropExperienceOnDeath 综合判定；actuallyHurt 受玩家伤害时设记忆时间；MobEntity::dropExperience
// 加守卫。Player/EnderDragon 各自 override dropExperience 直接掉落（等价 isAlwaysExperienceDropper=true）。
//
// 设计要点：
//   1. 玩家击杀掉经验（正向）：Survival 玩家 attackEntity zombie 1 次（设 lastHurtByPlayerMemoryTime=100，
//      zombie 受 1 伤害 HP=19）→ 紧接 test.kill（虚空伤害致死）。die 时记忆窗口（100tick）内，
//      shouldDropExperienceOnDeath 通过 → 掉经验球。经验球生成后被紧邻玩家吸收（onCollideWithPlayer →
//      _giveExperienceToPlayer）→ 玩家 getTotalXp() 增加。**断言 getTotalXp 增加而非数经验球实体**——
//      经验球设计为被玩家吸收，紧邻玩家的经验球生成后几 tick 内即被吸收消失，数实体不稳定。
//      getTotalXp 验证完整链路（掉落 + 吸收 + 玩家获经验）。
//   2. 非玩家击杀不掉经验（负向）：直接 test.kill zombie（无前置玩家伤害，lastHurtByPlayerMemoryTime=0），
//      守卫拒绝 → 不掉经验球。**断言区域 experience_orb==0**——无玩家吸收，经验球若生成会留存区域，
//      计数稳定 0 验证守卫拦截（修复前会掉，回归测试）。
//   3. doMobLoot=false 不掉经验：玩家伤害 + test.kill，但 doMobLoot=false 守卫拦截 → 不掉经验球。
//      断言 getTotalXp 不增加（玩家无经验球可吸收）。doMobLoot 世界级单例状态，独占 batch + runOnFinish 恢复。
//   4. 经验球实体类型 "minecraft:experience_orb"（区别于 item 实体 "minecraft:item"）。
//   5. zombie 经验值 5（m_experienceValue=5），玩家击杀时掉 5 点经验球，玩家吸收后 getTotalXp 增 5。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: LivingEntity.cpp shouldDropExperienceOnDeath/actuallyHurt（玩家伤害记忆）/tick（递减）
// Ref: MobEntity.cpp dropExperience（守卫）；LivingEntity.java:1498-1506（vanilla dropExperience 守卫）
// Ref: ExperienceOrbEntity.cpp onCollideWithPlayer/_giveExperienceToPlayer（经验球吸收链路）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
const XP_ORB = "minecraft:experience_orb";

/** 统计 glass_pit 区域内 experience_orb 实体数量（区域限定避免批内并行污染）。 */
function countXpOrbs(test: Test): number {
  return test.getDimension().getEntities({
    type: XP_ORB,
    location: test.worldLocation(PIT_FROM),
    volume: PIT_VOLUME,
  }).length;
}

// 玩家击杀的普通生物掉落经验球并被玩家吸收（lastHurtByPlayerMemoryTime 守卫正向）。
//
// Survival 玩家 attackEntity zombie 1 次（Player::attack → DamageSources::playerAttack(this) →
// zombie.hurt → actuallyHurt：source.getTrueSource()=玩家 → setLastHurtByPlayerMemoryTime(100)，
// zombie 受 1 伤害 HP=19）→ 紧接 test.kill（虚空伤害致死）。die 时 lastHurtByPlayerMemoryTime>0，
// shouldDropExperienceOnDeath 通过 → MobEntity::dropExperience 生成 5 点经验球。经验球生成于 zombie
// 位置（紧邻玩家），_followNearestPlayer 追踪玩家 → onCollideWithPlayer → _giveExperienceToPlayer
// → 玩家 totalExperience 增加 5。
//
// 用"玩家伤害 1 次 + 立即 test.kill"而非循环攻击致死：zombie 在 Survival 模式被攻击后会反击玩家
// （HurtByTargetGoal + MeleeAttackGoal），循环攻击期间玩家可能被 zombie 反击致死。玩家仅攻击 1 次
// 后立即 test.kill，zombie 在反击 goal 评估前（下一 tick）已死亡，玩家无伤。
//
// 断言 getTotalXp() 增加（而非数经验球实体）：经验球设计为被玩家吸收，紧邻玩家的经验球几 tick 内
// 被吸收消失，数实体不稳定。getTotalXp 验证完整链路（掉落+吸收+获经验）。
// 修复前 MobEntity::dropExperience 无守卫也掉经验，故本正向测试不能单独区分修复前后——需配合
// mob_drops_no_xp_when_killed_without_player（负向）交叉验证守卫生效。
// Ref: LivingEntity.cpp actuallyHurt（玩家伤害设记忆）/ shouldDropExperienceOnDeath
function mobDropsXpWhenKilledByPlayer(test: Test): void {
  // Survival 玩家 (4,2,3) 紧邻 zombie (3,2,3) 直线 1 格（attackEntity 命中距离 + 经验球生成后被吸收）。
  // gameMode=0=Survival（attackEntity 需造伤害链路；创造模式 m_abilities.invulnerable 早返回跳过 hurt）。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "killer", 0 as any);
  const zombie = test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });

  // 记录初始经验（玩家刚 spawn，getTotalXp=0）。
  const xpBefore = (player as any).getTotalXp() as number;

  // tick 10 玩家攻击 zombie 1 次（留 spawn 注册稳定时间）。设 lastHurtByPlayerMemoryTime=100。
  test.runAtTickTime(10, () => {
    player.attackEntity(zombie);
  });

  // tick 12 立即 test.kill zombie（记忆窗口 100tick 内，反击 goal 未及评估）。
  test.runAtTickTime(12, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言玩家 getTotalXp 增加（经验球生成→被玩家吸收→totalExperience 增 5）。
  // startTick=30 给 test.kill(tick12) + die 链路 + 经验球生成 + _followNearestPlayer 追踪 +
  // onCollideWithPlayer 吸收留充足时序余量（经验球飞向玩家需若干 tick）。
  pollUntilSucceed(test, () => (player as any).getTotalXp() as number > xpBefore, {
    startTick: 30,
    maxTick: 75,
    onTimeout: () => test.assert(false,
      `mob killed by player should increase player xp (got ${xpBefore} -> ${(player as any).getTotalXp()})`),
  });
}

// 非玩家击杀的普通生物不掉落经验球（lastHurtByPlayerMemoryTime 守卫负向）。
//
// 直接 test.kill zombie（虚空伤害 outOfWorld，无实体来源，m_lastHurtByPlayerMemoryTime=0）。
// die → MobEntity::dropExperience → shouldDropExperienceOnDeath：lastHurtByPlayerMemoryTime=0 不满足
// "被玩家伤害过"条件 → 守卫拒绝 → 不掉经验球。
//
// 修复前 MobEntity::dropExperience 无守卫直接掉落（m_experienceValue=5>0 即掉），test.kill 的 zombie
// 仍掉经验球——本测试负向断言经验球==0 验证守卫拦截（回归测试，修复前会失败）。
//
// 注：test.kill 虚空伤害 zombie 仍会掉物品（rotten_flesh 等，dropFromLootTable 不查玩家击杀），
// 但物品是 "minecraft:item" 实体不计入 experience_orb 计数，故不受影响。无玩家在场吸收，经验球若
// 生成会留存区域可被计数——计数==0 即证明未生成。
// Ref: LivingEntity.cpp shouldDropExperienceOnDeath（lastHurtByPlayerMemoryTime=0 拒绝）
function mobDropsNoXpWhenKilledWithoutPlayer(test: Test): void {
  const zombie = test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });

  // tick 5 等 spawn 稳定后 test.kill（虚空伤害无玩家来源，lastHurtByPlayerMemoryTime=0）。
  test.runAtTickTime(5, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言区域无 experience_orb（守卫拒绝掉经验）。无玩家吸收，经验球若生成会留存可计数。
  // startTick=10 给 test.kill(tick5) + die 链路留时序余量（即使掉物品也不影响经验球计数）。
  pollUntilSucceed(test, () => countXpOrbs(test) === 0, {
    startTick: 10,
    interval: 5,
    maxTick: 50,
    onTimeout: () => test.assert(false,
      `mob killed without player should drop no xp orbs, got ${countXpOrbs(test)} orbs`),
  });
}

// doMobLoot=false 时玩家击杀的普通生物不掉落经验球（doMobLoot 守卫）。
//
// 对齐 vanilla dropExperience 守卫含 doMobLoot 条件：doMobLoot=false 时即使被玩家伤害过
// （lastHurtByPlayerMemoryTime>0）也不掉经验。Survival 玩家 attackEntity zombie 1 次 + test.kill，
// 但 doMobLoot=false → shouldDropExperienceOnDeath 中 doMobLoot 条件不满足 → 守卫拒绝 → 不掉经验球
// → 玩家无经验球可吸收 → getTotalXp 不增加。
//
// 【并行污染隔离】doMobLoot 是世界级单例状态，GameTest 共享单一 ServerWorld 跨测试持久化不自动重置，
// 设 false 会污染同批依赖 mob 经验/物品掉落的测试。故独占 batch（mob_xp_loot_solo）串行执行 +
// runOnFinish 恢复 true（同 gametest-world-state-gamerule 隔离范式）。
// Ref: LivingEntity.cpp shouldDropExperienceOnDeath（doMobLoot 条件）
function mobDropsNoXpWhenDoMobLootFalse(test: Test): void {
  // 创造玩家执行管理命令（permLevel=4 ≥2）。doMobLoot 默认 true，显式设 false。
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "op");
  player.chat("/gamerule doMobLoot false");
  // runOnFinish 恢复 true 防污染后续批次（doMobLoot 世界级跨测试持久化）。
  test.runOnFinish(() => {
    player.chat("/gamerule doMobLoot true");
  });

  // 切 Survival 执行 attackEntity 造伤害（命令已在创造下生效）。
  player.chat("/gamemode survival");

  const zombie = test.spawn("minecraft:zombie", { x: 3, y: 2, z: 3 });
  const xpBefore = (player as any).getTotalXp() as number;

  // tick 15 等 doMobLoot=false + gamemode survival 命令生效（chat 命令队列有 tick 延迟）后攻击+杀死。
  test.runAtTickTime(15, () => {
    player.attackEntity(zombie);
  });
  test.runAtTickTime(17, () => {
    (test as any).kill(zombie);
  });

  // 轮询断言玩家 getTotalXp 未增加（doMobLoot=false 守卫拦截经验掉落，无经验球可吸收）。
  // 用负向轮询：等待足够时间确认经验始终不增。pollUntilSucceed 条件"不增"在 maxTick 内恒成立即 succeed。
  pollUntilSucceed(test, () => (player as any).getTotalXp() as number === xpBefore, {
    startTick: 35,
    maxTick: 70,
    onTimeout: () => test.assert(false,
      `mob killed by player should drop no xp when doMobLoot=false (got ${xpBefore} -> ${(player as any).getTotalXp()})`),
  });
}

export function registerMobDeathXpTests(): void {
  GameTest.register("MobBehaviorTests", "mob_drops_xp_when_killed_by_player", mobDropsXpWhenKilledByPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(90);

  GameTest.register("MobBehaviorTests", "mob_drops_no_xp_when_killed_without_player", mobDropsNoXpWhenKilledWithoutPlayer)
    .structureName("gametests:glass_pit")
    .maxTicks(80);

  // doMobLoot 是世界级状态，独占 batch 串行避免污染同批依赖 mob 经验/物品掉落的测试 + runOnFinish 恢复 true
  //（见 mobDropsNoXpWhenDoMobLootFalse 注释的并行污染隔离说明）。
  GameTest.register("MobBehaviorTests", "mob_drops_no_xp_when_do_mob_loot_false", mobDropsNoXpWhenDoMobLootFalse)
    .structureName("gametests:glass_pit")
    .batch("mob_xp_loot_solo")
    .maxTicks(90);
}
