// 旋风人行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

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

// 旋风人反弹弹射物（wiki tech_旋风人.txt#行为：旋风人不会受到除风弹以外的任何弹射物的直接伤害，
// 命中旋风人的任何除风弹外的弹射物会被减速并朝发射者的方向弹回）。
// 这是旋风人最具辨识度的防御机制——箭矢、雪球、三叉戟等弹射物命中旋风人后被反弹，旋风人不受伤。
//
// C++ 链路（对齐 MC Java 1.21.11 Breeze.deflection，Breeze.java:195-201）：
//   1) 箭矢命中实体时，ProjectileEntity::onImpact（ProjectileEntity.cpp:309-329）先查目标 deflection：
//        const ProjectileDeflection deflection = result.hitEntity->deflection(*this);
//   2) BreezeEntity::deflection（BreezeEntity.cpp:275-294）：弹射物非 WindCharge 时，若 Breeze 在
//      DEFLECTS_PROJECTILES 标签 → 播放 ENTITY_BREEZE_DEFLECT 音效 + 返回 ProjectileDeflection::Reverse。
//      对齐 Java Breeze.deflection：非 BREEZE_WIND_CHARGE/WIND_CHARGE 的弹射物 → DEFLECTS_PROJECTILES 标签 →
//      PROJECTILE_DEFLECTION（REVERSE + 播 BREEZE_DEFLECT 音效）。
//   3) deflection != None → ProjectileEntity::deflect(Reverse, breeze, false)（ProjectileEntity.cpp:321）
//      → applyProjectileDeflection（ProjectileDeflection.cpp:39-55）：速度 *= -0.5（反向减速）+ 随机偏航
//      170~190 度 + setShooter(breeze)（Breeze 成新发射者）。return true。
//   4) **被偏转后 onImpact 直接 return（:327-328），不调用 onEntityHit**——箭矢不造成伤害，Breeze 不受伤。
//      即 deflection 优先于 onEntityHit，弹射物在命中瞬间被反弹，根本不进入伤害判定。
//
// 与末影人瞬移躲避弹射物（enderman_dodges_projectile）的本质区别：
//   末影人靠 hurt() 中 isProjectile 检查瞬移（hurt 返 true 但不扣血），弹射物仍"命中"只是不扣血；
//   旋风人靠 deflection 在 onImpact 入口反弹弹射物，弹射物根本不进入 onEntityHit，物理上被弹开。
//   两者判定手段不同：末影人查 HP==40 + 位移；旋风人查 HP==30 + 箭矢反弹位移。
//
// 投射物选择——直接 spawn 箭矢 + setVelocity（参考 enderman_dodges_projectile 范式）：
//   雪球不适合——SnowballEntity::onEntityHit 对非烈焰人 damage=0 不调 hurt，但雪球仍会被 Breeze deflection
//   反弹（deflection 在 onImpact 入口查，先于 onEntityHit）。然而雪球反弹后无伤害，观测"反弹"需查位移，
//   雪球速度慢反弹后位移小难判定。箭矢反弹后速度 ×-0.5 仍可见位移，且箭矢是典型弹射物（wiki 明确"箭矢"）。
//   arrow spawn 后 setVelocity 无 shooter（getShooter 返 null），不影响 deflection 判定（deflection 只看
//   弹射物类型是否 WindCharge + Breeze 是否在 DEFLECTS_PROJECTILES 标签，不依赖 shooter）。
//
// 环境选择：mediumglass（12×9×11 玻璃盒空腔，y=0 圆石地板，x∈[2,10]/z∈[1,9] 空气腔）。
//   Breeze spawn (5,2,5) 空腔中心；箭矢 spawn (2,2,5) 距 Breeze 3 格，setVelocity({3,0,0}) 朝 +X 飞，
//   1 tick 跨 3 格命中 Breeze（箭矢 x 从 2→5，Breeze 碰撞箱 x∈[4.7,5.3]，射线 x∈[2,5] 覆盖）。
//   Breeze 脚下 y=1（y=0 圆石地板支撑），spawn y=2 落到 y=1 站立。
//
// 不 spawn 玩家：Breeze 的 NearestAttackableTargetGoal<Player> 找不到玩家不触发，Breeze 不会主动移动
//   追击玩家；但 WaterAvoidingRandomWalkingGoal（优先级6）会让 Breeze 随机游荡。箭矢 1 tick 即命中，
//   Breeze spawn 后首 tick 内游荡 goal 尚未推动 Breeze 位移（goalTick 在 tick 末尾，箭矢在 tick 头命中），
//   故 Breeze 仍在 (5,2,5) 原位被箭矢命中。mediumglass 空腔足够大，Breeze 即使微移仍在箭矢射线上。
//
// 判定手段：复合断言——Breeze HP==30（未受伤，deflection 生效未走 onEntityHit）且 箭矢已反弹
//   （箭矢 x 坐标 < Breeze x 坐标，证明 Reverse 偏转把箭矢弹回 -X 方向）。
//   - HP==30 单独不够：若箭矢未命中（setVelocity 失效/箭矢飞行偏移），Breeze HP 也==30，假性通过。
//   - 箭矢反弹位移单独不够：箭矢本就朝 +X 飞，反弹后应朝 -X，但若箭矢飞过 Breeze 继续前进 x 增大，
//     单看箭矢 x 不能区分"反弹"与"穿过"。故用箭矢 x < Breeze x 复合：反弹后箭矢朝 -X 飞回，
//     x 减小到 Breeze 左侧；若未反弹箭矢穿过 Breeze 朝 +X，x 增大到 Breeze 右侧。
//   - 复合断言：HP==30 且 箭矢 x < Breeze x（反弹到 Breeze 左侧），证明"箭矢命中→deflection Reverse→
//     反弹回 -X→Breeze 未受伤"完整链路。
//   末影人查询区域限定（MED_VOLUME 12×9×11）排除并行测试污染。
//   pollUntilSucceed startTick=20 留箭矢飞行(1tick)+命中+反弹时序，interval=5，maxTick=200 留反弹
//   非确定性余量（Reverse 偏航随机 170~190 度，箭矢反弹后 x 方向位移需轮询确认）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_旋风人.txt#行为（弹射物被反弹，不受直接伤害）
// Ref: BreezeEntity.cpp:275-294（deflection 返回 Reverse，对齐 Java Breeze.deflection）
// Ref: ProjectileEntity.cpp:309-329（onImpact deflection 优先于 onEntityHit，反弹后 return 不伤害）
// Ref: ProjectileDeflection.cpp:39-55（Reverse 速度×-0.5 + 随机偏航 + setShooter）
function breezeDeflectsArrow(test: Test): void {
  const breezeType = "minecraft:breeze";

  // Breeze spawn (5,2,5) 空腔中心，脚下 y=1 圆石地板支撑（spawn y=2 落到 y=1 站立）。
  test.spawn(breezeType, { x: 5, y: 2, z: 5 });

  // 箭矢 spawn (2,2,5) 距 Breeze 3 格，setVelocity({3,0,0}) 朝 +X 飞，1 tick 命中 Breeze。
  // arrow spawn 后立即 setVelocity，无 shooter 不影响 deflection 判定。
  const arrow = test.spawn("minecraft:arrow", { x: 2, y: 2, z: 5 });
  (arrow as any).setVelocity({ x: 3.0, y: 0, z: 0 });

  // 复合断言：Breeze HP==30（未受伤）且 箭矢 x < Breeze x（反弹回 -X 方向）。
  pollUntilSucceed(test, () => {
    const breezes = test.getDimension().getEntities({
      type: breezeType,
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    if (breezes.length === 0) return false;
    const breeze = breezes[0];
    const health = breeze.getComponent("minecraft:health");
    if (health === undefined) return false;
    const hp = (health as any).currentValue;
    // Breeze HP==30（未受伤，deflection 生效）。满血 30，箭矢未走 onEntityHit 故不扣血。
    if (hp < 30) return false;

    // 箭矢反弹回 -X 方向：箭矢 x < Breeze x 证明 Reverse 偏转把箭矢弹回 Breeze 左侧。
    const arrows = test.getDimension().getEntities({
      type: "minecraft:arrow",
      location: test.worldLocation(MED_FROM),
      volume: MED_VOLUME,
    });
    if (arrows.length === 0) return false;
    // 箭矢反弹后朝 -X 飞，x 应 < Breeze x（反弹位移）。取距 Breeze 最远的箭矢（反弹飞回的）。
    const arrowDeflected = arrows.find(a => a.location.x < breeze.location.x);
    return arrowDeflected !== undefined;
  }, {
    startTick: 20,
    interval: 5,
    maxTick: 200,
    onTimeout: () => {
      const breezes = test.getDimension().getEntities({
        type: breezeType,
        location: test.worldLocation(MED_FROM),
        volume: MED_VOLUME,
      });
      const arrows = test.getDimension().getEntities({
        type: "minecraft:arrow",
        location: test.worldLocation(MED_FROM),
        volume: MED_VOLUME,
      });
      const bHp = breezes.length > 0
        ? (breezes[0].getComponent("minecraft:health") as any)?.currentValue : "gone";
      const bPos = breezes.length > 0
        ? `(${breezes[0].location.x.toFixed(1)},${breezes[0].location.y.toFixed(1)},${breezes[0].location.z.toFixed(1)})`
        : "gone";
      const aPos = arrows.length > 0
        ? arrows.map(a => `(${a.location.x.toFixed(1)},${a.location.y.toFixed(1)},${a.location.z.toFixed(1)})`).join(",")
        : "gone";
      // 区分失败原因：bHp<30 说明箭矢命中受伤（deflection 未生效，走了 onEntityHit）；
      //   箭矢 x 全 ≥ Breeze x 说明箭矢穿过 Breeze 未反弹（deflection 未触发或反弹方向判定错）。
      test.assert(false,
        `breeze did not deflect arrow (breeze=${breezes.length} hp=${bHp} pos=${bPos}; arrows=${arrows.length}@${aPos}; ` +
        `expected hp==30 [unhurt] AND arrow deflected to x<breeze.x [Reverse bounce])`);
    },
  });
}

export function registerBreezeTests(): void {
  GameTest.register("MobBehaviorTests", "breeze_shoots_wind_charge_at_player", breezeShootsWindChargeAtPlayer)
    .structureName("gametests:mediumglass")
    .maxTicks(400);

  GameTest.register("MobBehaviorTests", "breeze_deflects_arrow", breezeDeflectsArrow)
    .structureName("gametests:mediumglass")
    .maxTicks(300);
}
