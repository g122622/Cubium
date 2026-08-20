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

export function registerWitherTests(): void {
  GameTest.register("MobBehaviorTests", "wither_shoots_skull_at_player_after_attacked", witherShootsSkullAtPlayerAfterAttacked)
    .structureName("gametests:ghast_arena")
    .maxTicks(400);
}
