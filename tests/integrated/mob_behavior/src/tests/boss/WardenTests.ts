// 监守者（Warden）Boss 行为类 GameTest。
//
// 监守者是 mob_behavior 包中 boss 类零覆盖实体（WitherTests 之外无 boss 测试）。WardenEntity.cpp
// 实现完整：MeleeAttackGoal(1.2) + HurtByTargetGoal(优先级1) + NearestAttackableTargetGoal<Player>
// (优先级2) + 简化怒气系统（m_anger + WardenAngerLevel）+ 伤害免疫（Drown/Wither/摔落）。
// 本测试补全其最具辨识度的 vanilla 行为：被激怒后近战反击玩家。
//
// C++ 链路（对齐 MC Java 1.21.11 Warden + WardenAi）：
//   test.spawn("warden") → WardenEntity::create() 生成。Emerging/Digging 姿态系统未实现（TODO，
//   WardenEntity.cpp:279-293），故无生成免疫阶段，spawn 即满血 500 可活动可受击。
//   registerGoals（WardenEntity.cpp:239-294）：
//     goalSelector 优先级2 MeleeAttackGoal(this, 1.2, false)；
//     targetSelector 优先级1 HurtByTargetGoal(this)（被攻击反击设攻击者为 attackTarget）；
//     targetSelector 优先级2 NearestAttackableTargetGoal<Player>(this, true)（checkSight=true，
//       直接选 Survival 玩家为目标，区别 Wither 的 MobEntity 模板需玩家先攻击）。
//   registerAttributes：MAX_HEALTH=500, ATTACK_DAMAGE=30, MOVEMENT_SPEED=0.3,
//     KNOCKBACK_RESISTANCE=1.0, FOLLOW_RANGE=24。
//
// 免疫行为（WardenEntity.cpp:197-233）已实现但暂不测：
//   - isInvulnerableTo 免疫 Drown/Wither：EffectCommand 用 EntityArgumentType::players() 只支持玩家
//     选择器，无法对监守者（非玩家实体）施加凋零效果；溺水对照需 500+ tick 且对照实体溺水掉血慢，
//     测试不稳定。待 EffectCommand 扩展支持实体目标后补测。
//   - onLivingFall 返 false 摔落免疫：监守者体积大（0.9×3.5）从高空自由下落速度累积穿模，穿过
//     圆石地板跌入虚空受虚空伤害掉血，掩盖摔落免疫判定。自由下落穿模是物理碰撞检测限制，非监守者
//     逻辑缺陷。低空下落（<4格）不造成摔落伤害无法构造对照。待有可靠的伤害施加手段后补测。
//
// 结构选择：ghast_arena（15×30×15 圆石地板玻璃竞技场）。监守者不悬浮（无 setNoGravity），站在 y=1
// 圆石地板上。FOLLOW_RANGE=24 足以覆盖 15 格结构内玩家。MeleeAttackGoal 寻路接近玩家。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: WardenEntity.cpp:239-294（registerGoals）/ 197-233（伤害免疫）/ 327-353（属性）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_监守者.txt#行为（近战攻击）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// ghast_arena 结构尺寸（15×30×15），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const ARENA_FROM = { x: 0, y: 0, z: 0 };
const ARENA_VOLUME = { x: 15, y: 30, z: 15 };

// 监守者被激怒后近战反击玩家（wiki tech_监守者.txt#攻击：监守者是近战型 boss，锁定目标后接近
//   造成 30 点近战伤害，足以一击秒杀满血玩家）。
//
// C++ 链路：玩家 attackEntity(warden) → SimulatedPlayer::attack → Warden hurt →
//   HurtByTargetGoal.onHurt 设 attackTarget=玩家（优先级1）。MeleeAttackGoal(优先级2) shouldExecute
//   读 attackTarget 非空 → 寻路接近 → attackEntityAsMob → hurt(玩家, 30.0)。
//   监守者 ATTACK_DAMAGE=30，玩家满血 20，一击致死（20-30=-10 → 死亡）。
//
// 环境选择：ghast_arena（15×30×15 圆石地板）。监守者(3,2,3)站地板上，Survival 玩家(5,2,3)距 2 格
//   （Player::attack 攻击距离 ~3 格，可 attackEntity 触发 HurtByTarget）。监守者 MOVEMENT_SPEED=0.3
//   接近 2 格约需 10+ tick，MeleeAttack 攻击冷却后 hurt(30)。
//
// 判定手段：断言玩家 HP<20 或已死亡消失。30 伤害一击致死，玩家 HP 会从 20 直降为 0 死亡消失。
//   succeedWhen 每 tick 检查玩家 HP<20 或 length==0（被秒杀消失）。
// 时序：tick 10 玩家攻击监守者 + HurtByTargetGoal 设目标 + MeleeAttackGoal 寻路接近 2 格 + 攻击冷却
//   + hurt(30)。maxTicks=600 留充裕余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验；创造模式 hurt 链路早返回不触发 HurtByTarget，
//   且 NearestAttackableTargetGoal<Player> checkSight 谓词排除创造玩家）。
// Ref: WardenEntity.cpp:239-294（registerGoals MeleeAttackGoal+HurtByTargetGoal）
function wardenAttacksPlayerWhenProvoked(test: Test): void {
  const wardenType = "warden";

  // 监守者 (3,2,3) 站 y=1 圆石地板上（ghast_arena 自带地板，无需额外铺支撑）。
  // Survival 玩家 (5,2,3) 距 2 格。
  const warden = test.spawn(wardenType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 5, y: 2, z: 3 }, "wardenVictim", 0 as any);

  // tick 10 后玩家攻击监守者：留 10 tick 让实体完成 spawn 注册 + 首 tick 稳定。
  // attackEntity 远程命中触发 HurtByTargetGoal → 设 attackTarget=玩家。
  test.runAtTickTime(10, () => {
    player.attackEntity(warden);
  });

  // 断言玩家掉血或死亡：succeedWhen 每 tick 持续检查玩家 HP<20 或已消失。
  // 30 伤害一击秒杀满血玩家（20→0 死亡消失），length==0 也算通过。
  test.succeedWhen(() => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(ARENA_FROM),
      volume: ARENA_VOLUME,
    });
    // 玩家已死亡消失（被监守者 30 伤害秒杀）——反击行为生效。
    if (players.length === 0) {
      return;
    }
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    test.assert((health as any).currentValue < 20,
      `warden did not retaliate with melee attack, hp=${(health as any).currentValue}`);
  });
}

export function registerWardenTests(): void {
  GameTest.register("MobBehaviorTests", "warden_attacks_player_when_provoked", wardenAttacksPlayerWhenProvoked)
    .structureName("gametests:ghast_arena")
    .maxTicks(600);
}
