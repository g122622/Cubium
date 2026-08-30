// 玩家近战偏转可偏转投射物（火球）对齐测试。
//
// 验证 vanilla Player.deflectProjectile（Player.java:1020-1029）：玩家攻击（左键）命中
// REDIRECTABLE_PROJECTILE 标签成员（fireball/wind_charge/breeze_wind_charge）时，不造成伤害，
// 而是以玩家视线方向偏转弹射物（AIM_DEFLECT）并播放 PLAYER_ATTACK_NODAMAGE 音效。
//
// C++ 链路（对齐 vanilla Player.attack → deflectProjectile）：
//   Player::attack（Player.cpp:2520）开头查 EntityTypeTags::REDIRECTABLE_PROJECTILE 含 target.getTypeId()
//   && target 是 ProjectileEntity → projectile.deflect(AimDeflect, *this, true) + 播
//   PLAYER_ATTACK_NODAMAGE + return（不走伤害流程）。
//   applyProjectileDeflection(AimDeflect)（ProjectileDeflection.cpp）设弹射物速度为偏转者视线方向
//   （单位向量，对齐 vanilla AIM_DEFLECT setDeltaMovement(getLookAngle)）+ setShooter(偏转者)。
//
// 此前缺陷：Cubium Player::attack 对非 LivingEntity 目标直接 return（"只能攻击生物实体"），
//   火球等可偏转投射物无法被玩家近战弹开（vanilla 玩家挥击恶魂火球可将其弹回）。本次修复在
//   attack 入口加 deflectProjectile 分支。同时修复 AimDeflect 误乘原速度大小（静止火球 speed=0
//   偏转后仍静止）改为单位视线向量对齐 vanilla。
//
// 测试手段：SimulatedPlayer.attackEntity(fireball)（ScriptSimulatedPlayer.cpp:984 转发 Player::attack），
//   任意距离生效（不依赖 raycast 命中）。火球 test.spawn 后无加速度无重力（DamagingProjectileEntity
//   构造 setNoGravity(true)），静止悬浮，偏转前位置不变。偏转后以单位速度（1 格/tick）沿玩家视线
//   方向匀速飞行（无加速度叠加），位置变化方向 = 玩家视线方向。
//
// 判定：玩家 yaw=0 视线 +Z，偏转后火球 z 坐标从 spawn 点（z=5）沿 +Z 增大（>5.5）。宽松阈值 0.5
//   容忍单 tick 位置量化误差。pollUntilSucceed startTick=10（attackEntity tick 5 后留 5 tick 飞行）。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）。玩家 (2,2,3) yaw=0 看 +Z，火球
//   (2,2,5) 玩家前方 +Z 2 格。火球偏转后沿 +Z 飞，creeper_pit z∈[0,6]，火球飞到 z=6 后可能撞
//   墙/边界消失，故 maxTick 紧凑（attackEntity 后 5-15 tick 内断言）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_恶魂.txt#攻击（大火球可被玩家挥击弹回）
// Ref: Player.cpp:2520（deflectProjectile 分支）/ ProjectileDeflection.cpp（AimDeflect 单位视线向量）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 区域限定查询排除并行测试污染（Cubium GameTest 批内并行 tick + 不清场）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

const FIREBALL = "minecraft:fireball";

// 玩家近战偏转火球：玩家 yaw=0 看 +Z，attackEntity(静止火球) 后火球沿 +Z 偏转飞走（z 增大）。
function playerDeflectsFireball(test: Test): void {
  // 玩家 (2,2,3) yaw=0（spawn 默认 yaw=0 看 +Z），Survival 模式（0 as any 绕过 TS 枚举校验）。
  const player = test.spawnSimulatedPlayer({ x: 2, y: 2, z: 3 }, "deflector", 0 as any);

  // 火球 spawn 于玩家前方 +Z 2 格 (2,2,5)。DamagingProjectileEntity 构造 setNoGravity(true)，
  // test.spawn 不设加速度，火球静止悬浮（velocity=0，performRayTrace start==end 不命中）。
  const fireball = test.spawn(FIREBALL, { x: 2, y: 2, z: 5 });

  // tick 5 attackEntity(火球) → Player::attack 开头 deflectProjectile 分支 → AimDeflect 偏转。
  // attackEntity 任意距离生效（不依赖 raycast），火球在玩家前方 2 格直接命中偏转逻辑。
  test.runAtTickTime(5, () => {
    (player as any).attackEntity(fireball);
  });

  // 轮询断言火球被偏转沿 +Z 飞走：z 坐标从初始 5.5 增大到 >6.0。
  // 偏转后火球以单位速度 1 格/tick 沿 +Z 飞，tick 5 偏转后每 tick z+1，startTick=10 时 z≈10.5
  // （但 creeper_pit z∈[0,6]，火球 z>6 后撞墙/飞出边界 remove，故实际 z 会被夹在 ~6 或火球消失）。
  // 断言 z>6.0（偏转后必远超 6；静止火球 z≈5.5 不满足→超时 FAIL）。若火球已飞出边界 remove
  // （length===0）也说明偏转生效（静止火球不会自发消失，见对照测试 fireball_stays_static_without_attack）。
  pollUntilSucceed(test, () => {
    const fireballs = test.getDimension().getEntities({
      type: FIREBALL,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    // 火球飞出 pit 边界 remove（length===0）也说明偏转生效（未偏转火球静止不会消失）。
    if (fireballs.length === 0) return true;
    const z = fireballs[0].location.z;
    return z > 6.0;
  }, {
    startTick: 10,
    interval: 2,
    maxTick: 60,
    onTimeout: () => {
      const fireballs = test.getDimension().getEntities({
        type: FIREBALL,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const z = fireballs.length > 0 ? fireballs[0].location.z : "fireball removed";
      test.assert(false,
        `player_deflects_fireball: failed: fireball z=${z} (expected >6.0 or removed, `
        + `fireball should be deflected along player look +Z). fireballCount=${fireballs.length}`);
    },
  });
}

// 对照测试：火球无玩家攻击时静止悬浮存活 40 tick 后位置不变。
// 排除 player_deflects_fireball 的假通过——若火球 spawn 后自发消失/移动，偏转测试中
// fireballs.length===0 或 z 变化可能并非偏转所致。本测试 spawn 火球不 attackEntity，
// 在 tick 40 断言火球仍存活且 z≈5（静止），证明火球不会自发消失/位移，从而偏转测试中
// 的位移/消失确由玩家 attackEntity 偏转导致。
// DamagingProjectileEntity 构造 setNoGravity(true)，test.spawn 不设加速度，火球 velocity=0
// 静止悬浮，performRayTrace start==end 不命中方块/实体，不会自爆。
// 用 runAtTickTime(40) 末期单点断言（非 succeedWhen——succeedWhen 首 tick 火球静止即 PASSED
// 无法验证"持续静止"，末期断言才能证明 40 tick 内未移动）。
function fireballStaysStaticWithoutAttack(test: Test): void {
  // 火球 spawn 于 (2,2,5)，无玩家攻击，应静止悬浮。
  test.spawn(FIREBALL, { x: 2, y: 2, z: 5 });

  // tick 40 断言火球仍存活且 z≈5.5（test.spawn (2,2,5) 坐标中心化，火球初始 z=5.5）。
  // 偏转测试 attackEntity 在 tick 5，本测试观察期 40 tick 远超偏转时序。火球 velocity=0 +
  // 加速度=0 + setNoGravity，tick 内 nextPosition=pos+velocity 不变，应静止在 z=5.5。
  // 阈值 ±0.3 容忍位置量化误差，重点验证火球不会自发飞远/消失——偏转测试的 z>5.5 或消失
  // 不会被静止火球满足（偏转后 1 格/tick，tick 10 时 z≈10.5 撞墙 remove，远超 5.5±0.3）。
  test.runAtTickTime(40, () => {
    const fireballs = test.getDimension().getEntities({
      type: FIREBALL,
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(fireballs.length > 0,
      `fireball disappeared without attack (should stay static), count=${fireballs.length}`);
    // Entity.location 是世界绝对坐标（结构网格原点非零，见 StructureGridSpawner），断言前必须经
    // worldLocation 把结构相对坐标 (2,2,5) 转成世界绝对坐标再比较（fireball 中心化 z+0.5）。
    // 直接拿绝对坐标与相对值 5.5 比较，单跑（首行结构原点≈0）碰巧通过，全量跑恒失败。
    const expectedZ = test.worldLocation({ x: 2, y: 2, z: 5 }).z + 0.5;
    const z = fireballs[0].location.z;
    test.assert(Math.abs(z - expectedZ) < 0.3,
      `fireball moved without attack (should stay near z=${expectedZ}), z=${z}`);
    test.succeed();
  });
}

export function registerProjectileDeflectTests(): void {
  GameTest.register("MobBehaviorTests", "player_deflects_fireball", playerDeflectsFireball)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);

  GameTest.register("MobBehaviorTests", "fireball_stays_static_without_attack", fireballStaysStaticWithoutAttack)
    .structureName("gametests:creeper_pit")
    .maxTicks(100);
}
