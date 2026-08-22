// 凋灵 Boss 行为类 GameTest。
//
// 凋灵是 mob_behavior 包中首个 boss 测试。凋灵实现链路完整（WitherEntity.cpp），但此前零集成测试。
// test.spawn("wither") 经工厂 WitherEntity::create() 生成——create() 函数体为空不调 ignite()，
// 故生成的凋灵 invulTime=0（跳过 220 tick 无敌阶段 + 7.0 生成爆炸 + DoNothingGoal 冻结），
// canRangedAttack()=true（getInvulTime()<=0）立即可远程攻击。这是测试利好：无需等待无敌阶段。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// ghast_arena 结构尺寸（15×30×15），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const ARENA_FROM = { x: 0, y: 0, z: 0 };
const ARENA_VOLUME = { x: 15, y: 30, z: 15 };

// 凋灵被玩家攻击后向玩家发射凋灵之首（wiki tech_凋灵.txt#攻击：凋灵是弓箭手型 boss，会向目标
//   发射凋灵之首投射物；凋灵之首命中造成爆炸伤害 + 凋零 II 效果）。
//
// C++ 链路（对齐 MC Java 1.21.11 WitherBoss + RangedAttackGoal）：
//   1) test.spawn("wither") → WitherEntity::create()（空体不调 ignite）→ 凋灵 invulTime=0、满血 300、
//      setNoGravity(true) 悬浮、canRangedAttack()=true。构造期 registerGoals：
//      goalSelector 优先级0 WitherDoNothingGoal（isInvulnerablePhase()=getInvulTime()>0，invulTime=0 不执行）、
//      优先级2 RangedAttackGoal(speed=1.0, minInterval=40, maxInterval=60, radius=20.0)（主头发射凋灵之首）、
//      优先级5 WitherRandomFlyGoal（0.001/tick 随机飞行，无目标时几乎不动）。
//      targetSelector 优先级1 HurtByTargetGoal（被攻击反击设攻击者为 attackTarget）、
//      优先级2 NearestAttackableTargetGoal<MobEntity>（checkSight=false，排除亡灵；模板参 MobEntity 故
//      不选 Player 为目标——玩家须通过 HurtByTargetGoal 路径成为 attackTarget）。
//   2) 玩家 attackEntity(凋灵) → SimulatedPlayer::attack → Player::attack → 凋灵 hurt →
//      HurtByTargetGoal.onHurt 设 attackTarget=玩家（玩家非亡灵，凋灵 hurt 链路不免疫玩家近战）。
//   3) RangedAttackGoal::shouldExecute（attackTarget 非空）→ tick 中 m_attackTime 倒计时到 0 且 canSee
//      → performAttack → IRangedAttackMob::attackEntityWithRangedAttack(target, charge) →
//      WitherEntity::launchWitherSkullToEntity(0, target)（主头）→ 构造 WitherSkullEntity(typeId=
//      "minecraft:wither_skull")，setShooter + shoot + setBlue + world->spawnEntity。
//   4) 凋灵之首 WitherSkullEntity 飞向玩家，存活窗口短（黑色初速 1.5/tick，几 tick 命中消失）。
//
// 为何需玩家主动攻击触发：凋灵 NearestAttackableTargetGoal 模板参是 MobEntity，Player 不继承 MobEntity
//   （Player : LivingEntity 非 MobEntity），故凋灵不会主动选玩家为目标。须玩家 attackEntity(凋灵) 经
//   HurtByTargetGoal 路径设玩家为 attackTarget，RangedAttackGoal 才发射之首。这与 GhastTests 不同
//   （恶魂 NearestAttackableTargetGoal<Player> 直接选玩家，无需玩家攻击）。
//
// 环境选择：ghast_arena（15×30×15 高耸玻璃竞技场）。凋灵 setNoGravity(true) 悬浮不坠落，无目标时
//   WitherRandomFlyGoal 0.001/tick 极少触发，凋灵基本停在 spawn 位置。凋灵体积 0.9×3.5，spawn (3,2,3)
//   悬浮在圆石地板上方。Survival 玩家 (5,2,3) 距 2 格，在玩家近战攻击范围内（Player::attack 攻击距离
//   ~3 格），可 attackEntity(凋灵) 触发 HurtByTargetGoal。canSee 水平射线穿空气腔不触地板/玻璃墙。
//
// 判定手段：检测区域内 minecraft:wither_skull 实体出现（对齐 GhastTests 检测 fireball 范式）。凋灵之首
//   生成是确定性行为（HurtByTarget 设目标后 RangedAttackGoal 40-60tick 必发），不受散布/命中随机性
//   影响。首存活窗口极短（1.5/tick 几 tick 命中消失），必须 succeedWhen 每 tick 轮询才能抓到，不能用
//   pollUntilSucceed（默认 interval=20 tick 会跳过首存活窗口）。不判定"玩家掉血/爆炸"——首飞行方向由
//   凋灵朝向决定，命中随机性大致掉血断言不稳；检测首实体验证"凋灵发射凋灵之首"核心语义即可。
// 时序：tick 10 玩家攻击凋灵 → HurtByTargetGoal 设目标 → RangedAttackGoal 首次攻击 40-60tick + 余量。
//   maxTicks=400 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验；创造模式 hurt 链路早返回不触发 HurtByTarget）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵.txt#攻击（向目标发射凋灵之首）
// Ref: WitherEntity.cpp registerGoals（RangedAttackGoal/HurtByTargetGoal）+ launchWitherSkullToEntity
// Ref: RangedAttackGoals.cpp RangedAttackGoal::tick（m_attackTime 倒计时发射）
function witherShootsSkullAtPlayerAfterAttacked(test: Test): void {
  const witherType = "wither";

  // 凋灵 (3,2,3) 悬浮（setNoGravity，构造期已设）于圆石地板上方，Survival 玩家 (5,2,3) 距 2 格。
  // 玩家在近战攻击范围内（~3 格），可 attackEntity(凋灵) 触发 HurtByTargetGoal。canSee 水平射线无遮挡。
  const wither = test.spawn(witherType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "bait", 0 as any);

  // tick 10 玩家攻击凋灵触发 HurtByTargetGoal：凋灵 setAttackTarget(玩家) → RangedAttackGoal 发射之首。
  // 攻击凋灵经 Player::attack → 凋灵 hurt（invulTime=0 不免疫玩家近战）→ HurtByTargetGoal.onHurt。
  // attackEntity 是 Cubium 扩展绑定（转发 Player::attack 走完整伤害链），as any 绕过 TS 类型。
  test.runAtTickTime(10, () => {
    (player as any).attackEntity(wither);
  });

  // 断言凋灵向玩家发射了凋灵之首：succeedWhen 每 tick 检查区域内是否存在 minecraft:wither_skull 实体。
  // 凋灵之首存活窗口极短（黑色初速 1.5/tick，几 tick 命中方块/玩家爆炸消失），必须每 tick 轮询
  // （succeedWhen）才能抓到，不能用 pollUntilSucceed（默认 interval=20 会跳过存活窗口）。对齐
  // GhastTests 用 succeedWhen 检测 fireball 范式。首查询用区域限定排除并行测试污染。
  // type 用 "minecraft:wither_skull"（带前缀，对应 Java WitherSkull）。
  test.succeedWhen(() => {
    const skulls = test.getDimension().getEntities({
      type: "minecraft:wither_skull",
      location: test.worldLocation(ARENA_FROM),
      volume: ARENA_VOLUME,
    });
    test.assert(skulls.length > 0, "wither did not shoot skull at player after being attacked");
  });
}

// 凋灵主动锁定玩家并发射凋灵之首（wiki tech_凋灵.txt#攻击：凋灵会主动攻击邻近的非亡灵生物，
//   生成/无敌阶段结束后即锁定目标发射凋灵之首）。
//
// 本测试专项验证凋灵主目标选择器 NearestAttackableTargetGoal 对 Player 的选取（对齐缺陷修复）。
// C++ 链路（对齐 MC Java 1.21.11 WitherBoss.registerGoals）：
//   targetSelector 优先级2 NearestAttackableTargetGoal<LivingEntity>（WitherEntity.cpp:901-911，
//   修复后模板参 MobEntity→LivingEntity），谓词 getCreatureAttribute()!=Undead。
//   shouldExecute（TargetGoals.cpp:176-227）用 findClosestEntity<LivingEntity>（dynamic_cast<LivingEntity*>）
//   搜索 FOLLOW_RANGE(16) 内最近满足谓词的 LivingEntity → 设为 attackTarget。
//   Player : LivingEntity（Player.hpp:112），dynamic_cast<LivingEntity*>(player) 成功；Player
//   getCreatureAttribute()=Undefined≠Undead 满足谓词 → 凋灵主动选玩家为 attackTarget。
//   RangedAttackGoal::shouldExecute（attackTarget 非空）→ tick 倒计时 m_attackTime→0 且 canSee →
//   performAttack → attackEntityWithRangedAttack(player, charge) → launchWitherSkullToEntity(0, player)
//   → 生成 minecraft:wither_skull 投射物。
//
// 此前缺陷（修复前）：模板参是 MobEntity，Player : LivingEntity 非 MobEntity（Player.hpp:112），
//   dynamic_cast<MobEntity*>(player) 失败，凋灵主目标永远不会主动选玩家为 attackTarget。玩家须靠
//   attackEntity(凋灵) 经 HurtByTargetGoal 被动成为目标（即 wither_shoots_skull_at_player_after_attacked
//   测试的路径）。这与 vanilla WitherBoss.java:105 NearestAttackableTargetGoal<>(this, LivingEntity.class)
//   主动涵盖 Player 的行为相悖。
//
// 关键设计——玩家不攻击凋灵（区分被动 HurtByTarget 路径）：
//   - 本测试**不调用 attackEntity(凋灵)**，凋灵的 attackTarget 只能来自主目标选择器主动锁定。
//   - 修复前：主目标 MobEntity 不选 Player，凋灵无 attackTarget，RangedAttackGoal 不发射 → 0 首 → FAIL。
//   - 修复后：主目标 LivingEntity 选 Player，凋灵发射之首 → PASSED。
//   - 此设计是修复的充分验证：若模板参改回 MobEntity（回归），本测试必 FAIL。
//
// 环境选择：ghast_arena（15×30×15 高耸玻璃竞技场）。凋灵 setNoGravity(true) 悬浮不坠落。
//   凋灵 (3,2,3) 悬浮，Survival 玩家 (10,2,3) 距 7 格——在 RangedAttackGoal 攻击半径 20 格内，
//   在 FOLLOW_RANGE 16 格搜索范围内，凋灵主目标选择器能锁定玩家。玩家距凋灵 7 格**远超玩家近战
//   攻击距离 ~3 格**，确保玩家不会意外攻击凋灵（即便会，也与测试逻辑独立，但保持距离排除干扰）。
//   canSee 水平射线穿空气腔不触地板/玻璃墙。
//
// 玩家存活：Survival 玩家满血 20，凋灵之首单发伤害（黑色 ~5-12）不致死，测试窗口（maxTicks=400）
//   内玩家存活，凋灵持续锁定玩家发射多枚之首。首存活窗口极短（1.5/tick 几 tick 命中消失），必须
//   succeedWhen 每 tick 轮询才能抓到（对齐 wither_shoots_skull_at_player_after_attacked 范式）。
//
// 时序：凋灵主目标选择器（chance=0 每 tick 检查）锁定玩家 → RangedAttackGoal 首次攻击 40-60 tick
//   + 余量。maxTicks=400 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验）：创造/旁观玩家被凋灵谓词... 实际凋灵
//   主目标谓词未排除创造玩家（区别副头 _updateHeadTargets:639-644 排除创造/旁观），但创造玩家不掉血
//   会被 hurt 早返回干扰判定一致性，统一用 Survival 对齐现有测试范式。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋灵.txt#攻击（主动攻击邻近生物）
// Ref: WitherEntity.cpp:901-911（NearestAttackableTargetGoal<LivingEntity> 修复）
// Ref: TargetGoals.cpp:176-227（shouldExecute findClosestEntity<LivingEntity>）/ :237（LivingEntity 实例化）
function witherProactivelyTargetsAndShootsPlayer(test: Test): void {
  const witherType = "wither";

  // 凋灵 (3,2,3) 悬浮（setNoGravity 构造期已设）。Survival 玩家 (10,2,3) 距 7 格：
  //   - 在 RangedAttackGoal 攻击半径 20 格内 + FOLLOW_RANGE 16 格搜索范围内 → 凋灵主目标能锁定玩家。
  //   - 远超玩家近战攻击距离 ~3 格 → 玩家不会攻击凋灵（测试仅验证凋灵主动锁定，非 HurtByTarget 路径）。
  // **不调用 attackEntity(凋灵)**：凋灵 attackTarget 只能来自主目标选择器主动选取，区分被动路径。
  test.spawn(witherType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 3 }, "bait", 0 as any);

  // 断言凋灵主动锁定玩家并发射之首：succeedWhen 每 tick 检查区域内 minecraft:wither_skull 实体。
  // 首存活窗口极短（黑色初速 1.5/tick，7 格距离约 5 tick 命中消失），必须每 tick 轮询（succeedWhen）
  // 才能抓到，不能用 pollUntilSucceed（默认 interval=20 跳过存活窗口）。对齐现有 wither 测试范式。
  // 修复前（MobEntity 模板）：凋灵不主动选玩家 → 0 首 → 本测试 FAIL（回归捕获）。
  // 修复后（LivingEntity 模板）：凋灵主动选玩家 → 发射之首 → PASSED。
  test.succeedWhen(() => {
    const skulls = test.getDimension().getEntities({
      type: "minecraft:wither_skull",
      location: test.worldLocation(ARENA_FROM),
      volume: ARENA_VOLUME,
    });
    test.assert(skulls.length > 0,
      "wither did not proactively target and shoot player "
      + "(NearestAttackableTargetGoal MobEntity->LivingEntity fix regressed: wither won't select player)");
  });
}

export function registerWitherTests(): void {
  GameTest.register("MobBehaviorTests", "wither_shoots_skull_at_player_after_attacked", witherShootsSkullAtPlayerAfterAttacked)
    .structureName("gametests:ghast_arena")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "wither_proactively_targets_and_shoots_player", witherProactivelyTargetsAndShootsPlayer)
    .structureName("gametests:ghast_arena")
    .maxTicks(400);
}
