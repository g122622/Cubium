// 抗性药水减伤行为类 GameTest。
//
// 验证抗性提升（Resistance）效果对非 BYPASSES_RESISTANCE 伤害源的减伤管线
// （LivingEntity::applyPotionDamageCalculations 抗性分支）正常工作。
//
// 减伤公式（CombatRules::getDamageAfterResistance，CombatRules.cpp:69）：
//   final = damage * max(0, 1 - level * 0.2)，抗性 I（level=1）= 20% 减伤。
// 与 Java LivingEntity.getDamageAfterMagicAbsorb:1825 的 (25-(amp+1)*5)/25 等价
// （amp=0 → 20/25=0.8，减 20%）。
//
// C++ 链路：
//   addEffect("resistance", duration, {amplifier:0})（EffectType.cpp:55 映射→EffectType::Resistance）
//     → EffectManager::addEffect 施加效果，getEffectLevel(Resistance) 立即返 1。
//   addEffect("instant_damage", 1, {amplifier:0}) → applyInstantly→_applyEffect（EffectInstance.cpp）
//     → 非反转实体 hurt(DamageSources::magic(), 6<<0=6)。
//   hurt→actuallyHurt（LivingEntity.cpp:322）→ applyPotionDamageCalculations（:378）
//     → 抗性门控 !source.is(BYPASSES_RESISTANCE)（LivingEntity.cpp:544）：magic 源不在
//       BYPASSES_RESISTANCE（成员={OutOfWorld, GenericKill}），抗性减伤生效，6×0.8=4.8。
//
// 抗性门控标签说明：BYPASSES_RESISTANCE 成员={OutOfWorld, GenericKill}，与 BYPASSES_INVULNERABILITY
//   成员相同。此前 Cubium 误用 !source.bypassesInvulnerability()（=BYPASSES_INVULNERABILITY）门控抗性，
//   两者成员当前恰好相同故行为暂对但语义错位——一旦数据包扩展任一标签即偏离。本次改为查
//   BYPASSES_RESISTANCE 对齐标签语义（LivingEntity.getDamageAfterMagicAbsorb:1825）。
//   注：因两标签成员相同，本测试无法区分 INVULNERABILITY vs RESISTANCE（行为无差异），仅守护抗性
//   减伤功能经标签门控改动后不回归——验证抗性 I 对 magic 伤害减伤 20% 仍生效。
//
// 测试设计（正反对照，防假通过）：
//   - resistance_reduces_magic_damage：同结构内抗性 I villager + 无抗性 villager 各受瞬间伤害 6。
//     抗性实扣 ~4.8（20% 减伤），无抗性实扣 ~6（全额）。交叉断言 resDrop < baseDrop 证明减伤生效。
//   - no_resistance_takes_full_magic_damage：独立基线，无抗性 villager 受瞬间伤害 6 实扣 ~6。
//     若本测试失败（drop≈0）说明瞬间伤害链路本身失效，抗性测试的"减伤"是假通过。
//
// 判定手段：用闭包持有的 test.spawn 实体句柄直接读 getComponent("minecraft:health").currentValue，
//   记录施加前后 HP 差值（drop）。抗性 drop≈4.8（容差 [4.0,5.5]），无抗性 drop≈6（容差 [5.9,6.1]）。
//   闭包句柄不依赖实体位置/类型查询，规避两个同类型 villager 无法区域区分 + night 自然刷怪污染。
//
// 时序：tick 5 读基准 HP + 施加抗性 I → tick 6 施加瞬间伤害（addEffect 同步 applyInstantly 扣血）
//   → tick 8 读受击后 HP 断言 drop。tick 5→6 间隔确保抗性效果在瞬间伤害 applyInstantly
//   → hurt → applyPotionDamageCalculations 之前已施加（getEffectLevel 返回 1）。
//
// 环境选择：grass_pen（9×5×9 玻璃墙围）限制 villager 漂移出结构。day batch（默认）即可，抗性减伤
//   不依赖光照/时间。两个 villager 分放 (3,2,3)/(6,2,6) 间距足够不互相碰撞干扰。
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: CombatRules.cpp:69（getDamageAfterResistance 抗性减伤公式）
// Ref: LivingEntity.cpp:544（applyPotionDamageCalculations BYPASSES_RESISTANCE 抗性门控）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN。
function readHp(entity: unknown): number {
  const health = (entity as any)?.getComponent?.("minecraft:health") as
    | { currentValue?: number }
    | undefined;
  return health?.currentValue ?? NaN;
}

// 抗性 I 的村民受瞬间伤害 I（6 点 magic 伤害）实扣 ~4.8（20% 抗性减伤）。
//
// 验证 LivingEntity::applyPotionDamageCalculations 抗性分支（LivingEntity.cpp:544-548）对 magic 源生效。
// villager HP=20，抗性 I（amplifier=0 → level=1）减伤 20%：6×0.8=4.8，HP 20→15.2。
// 对照无抗性 villager 受同样 6 伤害实扣 6（HP 20→14）。
//
// 两个 villager 分放 (3,2,3)（抗性）与 (6,2,6)（无抗性），同结构内并行测试。抗性 villager 先
// addEffect("resistance") 再 addEffect("instant_damage")，确保抗性效果在瞬间伤害 applyInstantly
// → hurt → applyPotionDamageCalculations 之前已施加（getEffectLevel 返回 1）。
function resistanceReducesMagicDamage(test: Test): void {
  const villagerType = "villager_v2";

  // 两个 villager 分放 grass_pen 两侧，间距足够不互相碰撞。grass_pen y=0 满铺 grass_block，
  // spawn 于 y=2 下落至 y=1 脚踩 grass_block（下落 1 格不扣血）。
  const resistant = test.spawn(villagerType, { x: 3, y: 2, z: 3 });
  const baseline = test.spawn(villagerType, { x: 6, y: 2, z: 6 });

  let resHp0 = NaN;
  let baseHp0 = NaN;

  // tick 5：读基准 HP（应满血 20），并对 resistant 施加抗性 I（amplifier=0）。
  // duration=200 远超测试时长，确保抗性效果在瞬间伤害施加时仍生效。
  test.runAtTickTime(5, () => {
    resHp0 = readHp(resistant);
    baseHp0 = readHp(baseline);
    (resistant as any).addEffect("resistance", 200, { amplifier: 0, showParticles: false });
  });

  // tick 6：对两个 villager 施加瞬间伤害 I（amplifier=0 → 6<<0=6 magic 伤害）。addEffect 同步
  // applyInstantly 扣血。抗性 villager 经 applyPotionDamageCalculations 抗性减伤 6×0.8=4.8；
  // 无抗性 villager 全额 6。
  test.runAtTickTime(6, () => {
    (resistant as any).addEffect("instant_damage", 1, { amplifier: 0, showParticles: false });
    (baseline as any).addEffect("instant_damage", 1, { amplifier: 0, showParticles: false });
  });

  // tick 8：读受击后 HP，断言 drop（下降量）。抗性 drop≈4.8（容差 [4.0,5.5]），无抗性 drop≈6（[5.9,6.1]）。
  // 交叉验证：抗性 drop 显著小于无抗性 drop（减伤生效），而非两者相同（减伤失效）。
  test.runAtTickTime(8, () => {
    const resHp1 = readHp(resistant);
    const baseHp1 = readHp(baseline);

    const resDrop = resHp0 - resHp1;
    const baseDrop = baseHp0 - baseHp1;

    // 无抗性基线：drop≈6（全额 magic 伤害），容差 [5.9,6.1]。
    test.assert(baseDrop >= 5.9 && baseDrop <= 6.1,
      `baseline villager should take ~6 magic damage (no resistance), hp0=${baseHp0} hp1=${baseHp1} drop=${baseDrop}`);
    // 抗性 I：drop≈4.8（20% 减伤），容差 [4.0,5.5]。
    test.assert(resDrop >= 4.0 && resDrop <= 5.5,
      `resistant villager should take ~4.8 magic damage (20% resistance reduction), hp0=${resHp0} hp1=${resHp1} drop=${resDrop}`);
    // 交叉验证：抗性 drop 显著小于无抗性 drop（减伤生效）。
    test.assert(resDrop < baseDrop,
      `resistance should reduce damage: resDrop=${resDrop} should be < baseDrop=${baseDrop} (20% reduction not working — regression in BYPASSES_RESISTANCE gate)`);

    test.succeed();
  });
}

// 对照：无抗性村民受瞬间伤害 I 实扣全额 6（排除"瞬间伤害本身失效"假通过）。
//
// 与 resistance_reduces_magic_damage 对称：同 villager 同位置，唯一差异是不施加抗性药水。
// villager 应受全额 6 伤害 HP 20→14。若本测试失败（villager 不掉血）说明瞬间伤害链路本身失效，
// 则 resistance_reduces_magic_damage 的"抗性减伤"是假通过（伤害根本没生效，抗性与否都掉 0）。
//
// 注：本测试与 resistance_reduces_magic_damage 内的 baseline villager 逻辑重复，但独立成测试保证
// 单一职责——单独跑本测试即可验证瞬间伤害基线，不依赖抗性测试的对照逻辑。
function noResistanceTakesFullMagicDamage(test: Test): void {
  const villagerType = "villager_v2";

  // villager (3,2,3) 脚踩 grass_pen y=0 grass_block。HP=20，无抗性应受全额 6 伤害。
  const villager = test.spawn(villagerType, { x: 3, y: 2, z: 3 });

  let hp0 = NaN;

  // tick 5 读基准 HP（满血 20）。
  test.runAtTickTime(5, () => {
    hp0 = readHp(villager);
  });

  // tick 6 施加瞬间伤害 I（amplifier=0 → 6<<0=6 magic 伤害）。addEffect 同步 applyInstantly 扣血。
  test.runAtTickTime(6, () => {
    (villager as any).addEffect("instant_damage", 1, { amplifier: 0, showParticles: false });
  });

  // tick 8 读受击后 HP，断言 drop≈6（全额，无抗性不减伤）。容差 [5.9,6.1]。
  test.runAtTickTime(8, () => {
    const hp1 = readHp(villager);
    const drop = hp0 - hp1;
    test.assert(drop >= 5.9 && drop <= 6.1,
      `villager without resistance should take ~6 full magic damage, hp0=${hp0} hp1=${hp1} drop=${drop} `
      + `(if drop≈0 instant_damage link itself is broken — resistance test may be false-passing)`);
    test.succeed();
  });
}

export function registerResistanceEffectTests(): void {
  GameTest.register("MobBehaviorTests", "resistance_reduces_magic_damage", resistanceReducesMagicDamage)
    .structureName("gametests:grass_pen")
    .maxTicks(200);

  GameTest.register("MobBehaviorTests", "no_resistance_takes_full_magic_damage", noResistanceTakesFullMagicDamage)
    .structureName("gametests:grass_pen")
    .maxTicks(200);
}
