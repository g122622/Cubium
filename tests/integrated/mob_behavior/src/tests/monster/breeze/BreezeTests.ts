// 旋风人行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// mediumglass 结构尺寸（12×9×11），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const MED_FROM = { x: 0, y: 0, z: 0 };
const MED_VOLUME = { x: 12, y: 9, z: 11 };

// 旋风人向玩家发射风弹（wiki tech_旋风人.txt#攻击：旋风人锁定半径 24 格内的玩家和铁傀儡，
// 向半径 16 格内的玩家和铁傀儡发射风弹，风弹命中造成 1 弹射物伤害）。
//
// C++ 链路：BreezeEntity : MonsterEntity，registerGoals 注册：
//   targetSelector 优先级2：NearestAttackableTargetGoal<Player>(checkSight=true)——主动选玩家为目标。
//   goalSelector 优先级2：BreezeShootGoal（射击风弹）。
//   goalSelector 优先级3：BreezeLongJumpGoal（长跳移动）。
//   goalSelector 优先级4：BreezeShootWhenStuckGoal（卡住时紧急射击）。
//   goalSelector 优先级5：BreezeSlideGoal（滑行移动）。
//
// BreezeShootGoal::shouldExecute 要求 hasShootPermit()（射击许可）+ distSq<=ATTACK_RANGE_MAX_SQ(16²) +
// pose==Standing + shootCooldown<=0。射击许可由 BreezeSlideGoal::startExecuting（BreezeGoals.cpp:651）
// 或 BreezeLongJumpGoal/BreezeShootWhenStuckGoal 调 setShootPermit 授予。故 Breeze 须先启动滑行/长跳
// goal 获得许可，BreezeShootGoal 才能触发射击。BreezeSlideGoal::shouldExecute 要求有目标 + onGround +
// jumpCooldown<=0 + 无 shootPermit——Breeze 在地面有目标时滑行 → 授予许可 → 下次评估 BreezeShootGoal
// 射击。射击流程：startExecuting 充能 CHARGE_TICKS → tick 中 shootWindCharge() 生成 WindChargeEntity。
//
// 环境选择：mediumglass（12×9×11 玻璃盒空腔）。结构内 x∈[2,10]/z∈[1,9] 为空气腔，y=0 圆石地板。
// 旋风人 helper(2,2,5)+玩家 helper(10,2,5)：水平距 8 格 < 16 ATTACK_RANGE_MAX 且 < 24 FOLLOW_RANGE，
// Breeze 可锁定并射击。坐标布局同 EvokerTests/CowTests（已验证空腔可用）。旋风人受重力下落到圆石
// 地板 onGround 后方可滑行授予射击许可。旋风人免疫摔落伤害（causeFallDamage override），下落不损血。
// 旋风人 setBurnsInDaylight 未 override（MonsterEntity 默认 false，对齐原版旋风人不燃），白天默认环境。
//
// 判定手段：检测区域内 minecraft:wind_charge 实体出现。风弹射击是确定性生成实体（充能结束即发射），
// 不受伤害命中随机性影响（区别于"玩家掉血"判定）。对齐 wiki"向玩家发射风弹"核心语义。
// 旋风人射击需先滑行授予许可 + 充能 CHARGE_TICKS，时序较长，maxTicks 留足余量。
// 玩家用 Survival（gameMode=0，0 as any 绕过 TS 枚举校验，创造模式被 NearestAttackableTarget 滤掉）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_旋风人.txt#攻击（向玩家发射风弹）
function breezeShootsWindChargeAtPlayer(test: Test): void {
  const breezeType = "breeze";

  // 旋风人 (2,2,5)、Survival 玩家 (10,2,5)，水平距 8 格，同处结构 y=2 空气腔（地板 y=1 圆石支撑）。
  // 距 8 格 < 16 ATTACK_RANGE_MAX 且 < 24 FOLLOW_RANGE，旋风人可锁定玩家并射击风弹。
  // mediumglass 空气腔内 checkSight 射线沿 x 轴穿过空气，不触玻璃墙。
  // 坐标布局同 EvokerTests（已验证空腔可用），避免旋风人卡玻璃墙。
  test.spawn(breezeType, { x: 2, y: 2, z: 5 });
  test.spawnSimulatedPlayer({ x: 10, y: 2, z: 5 }, "bait", 0 as any);

  // 断言旋风人向玩家发射了风弹：succeedWhen 每 tick 检查区域内是否存在 wind_charge 实体。
  // 时序：NearestAttackableTarget 选目标 + BreezeSlideGoal 滑行授予射击许可 + BreezeShootGoal 充能
  // CHARGE_TICKS + 发射 wind_charge。多 goal 协调时序较长，maxTicks 留足余量。
  // 风弹查询用区域限定排除并行测试污染；type 用 "minecraft:wind_charge"（带前缀）。
  test.succeedWhen(() => {
    const charges = test.getDimension().getEntities({
      type: "minecraft:wind_charge",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    test.assert(charges.length > 0, "breeze did not shoot wind charge at player");
  });
}

export function registerBreezeTests(): void {
  GameTest.register("MobBehaviorTests", "breeze_shoots_wind_charge_at_player", breezeShootsWindChargeAtPlayer)
    .structureName("gametests:mediumglass")
    .maxTicks(400);
}
