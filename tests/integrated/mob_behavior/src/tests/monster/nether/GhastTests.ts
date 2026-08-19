// 恶魂行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// ghast_arena 结构尺寸（15×30×15），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const ARENA_FROM = { x: 0, y: 0, z: 0 };
const ARENA_VOLUME = { x: 15, y: 30, z: 15 };

// 恶魂向玩家发射大火球（wiki tech_恶魂.txt#攻击：恶魂会向 100 格内的玩家发射大火球，
//   大火球命中造成 17 爆炸伤害 + 点燃；恶魂闭眼时正在充能火球）。
//
// C++ 链路：GhastEntity : MonsterEntity（NetherEntities.cpp），registerGoals：
//   targetSelector 优先级1：NearestAttackableTargetGoal<Player>(checkSight=true, chance=10)
//     ——每 10 tick 检查一次，FOLLOW_RANGE=100 格内 canSee 玩家即设 attackTarget。
//   goalSelector 优先级5：GhastRandomFlyGoal（随机飞行，占 Move flag，WANDER_RANGE=16）。
//   goalSelector 优先级7：GhastLookAroundGoal（随机看向，占 Look flag）。
//   goalSelector 优先级7：GhastFireballAttackGoal（火球攻击，空 flag 集合）。
//
// 关键：GhastFireballAttackGoal 与 GhastLookAroundGoal 同为优先级 7，但前者 flag
//   为空集（对齐 vanilla GhastShootFireballGoal 无 setFlags 调用，默认空集），不与
//   占 Look flag 的 GhastLookAroundGoal 互斥，两者同时运行。若误给
//   GhastFireballAttackGoal 设 Look flag，Cubium GoalSelector 同优先级无法互相抢占
//   （canBeReplacedBy 要求 p.priority < this.priority），先注册的
//   GhastLookAroundGoal 永久独占 Look，GhastFireballAttackGoal 永不 tick，火球不发。
//
// GhastFireballAttackGoal::tick（GhastGoals.cpp）：attackTarget 存在且
//   distSq < ATTACK_RANGE_SQ(64²=4096) && canSee(target) 时 ++m_attackTimer；
//   m_attackTimer >= CHARGE_DURATION(20) 时调 ghast->shootFireball() 并进入 -40 冷却。
//   canSee 是视线射线方块遮挡检查，不依赖 ghast 朝向（yaw/pitch），故恶魂无论朝向哪，
//   只要与玩家无方块遮挡 + 距离<64 即充能开火。
//
// shootFireball（NetherEntities.cpp）：构造 FireballEntity(typeId="minecraft:fireball")，
//   setTypeId + setWorld + setShooter + setPosition + setAcceleration + setExplosionPower，
//   world->spawnEntity 生成实体。对应 Java LargeFireball（爆炸威力 1，命中 17 伤害）。
//   【修复】原漏调 fireball->setWorld(worldPtr)，致火球 m_world==nullptr，spawnEntity 后
//   DamagingProjectileEntity::tick 的 performRayTrace 因 m_world==nullptr 恒返 Miss，
//   火球不移动也不被 getEntities 查到（spawnEntity 未将 m_world==nullptr 实体纳入世界查询）。
//   对照 BlazeFireballAttackGoal/DragonFireballEntity 均显式 setWorld，ghast 漏调是 bug。
//
// 环境选择：ghast_arena（15×30×15 高耸玻璃竞技场）。结构内 x∈[1,13]/z∈[1,13]/y∈[1,28]
//   为空气腔，y=0 圆石地板，y=29 圆石天花板。恶魂体积 4×4×4，spawn 于 (3,2,3)，
//   setNoGravity(true) 后悬浮（对齐 vanilla Ghast 飞行），与 Survival 玩家 (11,2,3) 同层。
//   水平距 8 格 << 64 ATTACK_RANGE，canSee 水平射线穿空气腔不触地板/玻璃墙。
//
// 为何需 30 格高：GhastRandomFlyGoal WANDER_RANGE=16 随机漂浮，setNoGravity 简化实现
//   无飞行摩擦衰减（对齐 vanilla travelFlying 的 TODO 未完成），velocity 累积致恶魂持续
//   上升最多 ~16 格。mediumglass(9 格高) 恶魂漂 3 格即撞天花板，火球从恶魂眼睛（贴天花
//   板）生成立即 onBlockHit 消失，pollUntilSucceed 查不到。ghast_arena 30 格高给恶魂 25
//   格漂浮空间，火球水平/斜向飞向玩家不撞顶，飞行期间持续存在于世界被轮询抓到。
//
// 恶魂不主动追击玩家（无 pursue goal，仅 GhastRandomFlyGoal 随机飞行，对齐 Java Ghast），
//   但 GhastFireballAttackGoal 开火不依赖恶魂移动——恶魂悬浮后 canSee 玩家即充能开火。
//
// 判定手段：检测区域内 minecraft:fireball 实体出现（对齐 BlazeTests 检测 small_fireball 范式）。
//   火球生成是确定性行为（充能 20 tick 后必然发射），不受散布/命中随机性影响。火球飞行期间
//   持续存在于世界（直到命中方块/实体 remove），pollUntilSucceed 每 tick 轮询必能抓到。
//   不判定"玩家掉血/爆炸"——火球飞行方向由 ghast 朝向决定，可能飞向墙壁爆炸而非命中玩家，
//   命中随机性大致掉血断言不稳；检测火球实体验证"恶魂充能并发射大火球"核心语义即可。
// 时序：NearestAttackableTarget 每 10 tick 选目标 + GhastFireballAttackGoal 充能 20 tick + 发射。
//   maxTicks=600 留充裕余量（目标选择 + 恶魂漂浮稳定 + 充能 + 余量）。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 TargetGoal 滤掉不可被攻击）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_恶魂.txt#攻击（向玩家发射大火球）
function ghastShootsFireballAtPlayer(test: Test): void {
  const ghastType = "ghast";

  // 恶魂 (3,2,3) 悬浮（setNoGravity）于圆石地板上方，与 Survival 玩家 (11,2,3) 同层。
  // 水平距 8 格 << 64 ATTACK_RANGE，canSee 水平射线穿空气腔不触地板/玻璃墙。
  test.spawn(ghastType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 11, y: 2, z: 3 }, "bait", 0 as any);

  // 断言恶魂向玩家发射了大火球：succeedWhen 每 tick 检查区域内是否存在 minecraft:fireball 实体。
  // 时序：NearestAttackableTarget 选目标(每10tick) + 恶魂漂浮稳定 + GhastFireballAttackGoal 充能 20 tick + 发射。
  // 火球 velocity 由加速度（target-fireballPos，约 5 格/tick）驱动，飞行 1-2 tick 即命中玩家/方块爆炸消失，
  // 存活窗口极短——必须每 tick 轮询（succeedWhen）才能抓到，不能用 pollUntilSucceed（默认 interval=20 tick
  // 会跳过火球存活窗口）。对齐 BlazeTests 用 succeedWhen 检测 small_fireball 范式。
  // 火球查询用区域限定排除并行测试污染；type 用 "minecraft:fireball"（带前缀，对应 Java LargeFireball）。
  test.succeedWhen(() => {
    const fireballs = test.getDimension().getEntities({
      type: "minecraft:fireball",
      location: test.worldLocation(ARENA_FROM),
      volume: ARENA_VOLUME,
    });
    test.assert(fireballs.length > 0, "ghast did not shoot fireball at player");
  });
}

export function registerGhastTests(): void {
  GameTest.register("MobBehaviorTests", "ghast_shoots_fireball_at_player", ghastShootsFireballAtPlayer)
    .structureName("gametests:ghast_arena")
    .maxTicks(600);
}
