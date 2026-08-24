// 破甲（Breach）附魔护甲穿透行为类 GameTest。
//
// 验证 Cubium 破甲附魔运行时消费链路（LivingEntity::applyArmorCalculations →
// CombatRules::getDamageAfterAbsorb(damage, armor, toughness, breachLevel)）正确接入，
// 对齐 MC Java 1.21.11 CombatRules.getDamageAfterArmor（CombatRules.java:16-30）的
// modifyArmorEffectiveness 修正。
//
// 破甲机制（对齐 vanilla Enchantments.java:1207 + CombatRules.java:20-26）：
//   ARMOR_EFFECTIVENESS 组件 = AddValue(perLevel(-0.15))，每级 -0.15 护甲有效率。
//   getDamageAfterArmor 内：
//     f2 = effectiveArmor / 25                       // armorRatio
//     f3 = clamp(f2 + breachModifier, 0, 1)          // Breach 修正后有效率
//     final = damage * (1 - f3)
//   即 Breach 降低护甲减伤效果（非直接增伤），每级少减 15%。
//
// 数值核算（重锤 baseDamage=6.0 满冷却，钻石套 armor=20 toughness=8）：
//   protectionFactor = 2 + 8/4 = 4
//   effectiveArmor = clamp(20 - 6/4, 20*0.2=4, 20) = clamp(18.5, 4, 20) = 18.5
//   armorRatio = 18.5/25 = 0.74
//   无 Breach：final = 6 * (1 - 0.74) = 6 * 0.26 = 1.56（HP 20→18.44）
//   Breach IV（-0.60）：f3 = clamp(0.74 - 0.60, 0, 1) = 0.14
//                      final = 6 * (1 - 0.14) = 6 * 0.86 = 5.16（HP 20→14.84）
//   差异 1.56 vs 5.16（HP 差 3.6），Breach IV 显著穿透护甲。
//
// 修复（任务 #311）：Cubium 此前 LivingEntity::applyArmorCalculations 直接调三参数
//   getDamageAfterAbsorb(damage, armor, toughness)，完全不查攻击者武器 Breach 等级，
//   致重锤破甲附魔定义了但运行时从未消费（BreachEnchantment::getArmorEffectivenessModifier
//   仅被 AttackContext 死代码路径调用）。修复后 applyArmorCalculations 从 source.directSource()
//   取攻击者主手武器查 Breach 等级，调四参数重载修正护甲有效率。
//
// 攻击武器用重锤（Breach 是重锤专属附魔，且 Breach 与锋利/亡灵杀手/节肢杀手互斥，
// 仅重锤可正常附 Breach）。站立攻击（fallDistance=0）：canSmashAttack=false 无下落加成，
// 纯 baseDamage 6.0 走标准 Player::attack → target.hurt → actuallyHurt →
// applyArmorCalculations 路径，Breach 在此消费（与钻石剑近战路径一致，仅 baseDamage 不同）。
//
// 受害者用 villager（mob，HP 20，被动不反击，attackEntity 可解包 mob 目标）穿钻石套
// （equippable.setEquipment，armor=20 toughness=8）。同 ArmorDamageReductionTests 范式。
//
// 防假通过设计（正反对照）：
//   - no_breach_mace_reduced_by_diamond_armor：无 Breach 重锤攻击穿甲受害者掉 ~1.56（证明护甲减伤
//     正常 + 攻击基线正常，非"攻击失效 0 伤害"）。
//   - breach_iv_bypasses_diamond_armor：Breach IV 重锤攻击穿甲受害者掉 ~5.16（证明 Breach 穿透生效）。
//   两测试交叉验证：无 Breach 减伤 1.56 + Breach IV 穿透 5.16 = Breach 机制正确。
//   若 Breach 未消费（修复前），Breach IV 受害者仍掉 ~1.56（≈无 Breach），断言掉 ≥3.5 FAIL。
//
// 独立 batch（breach_solo）：避免 night batch 并行污染（外来 villager/攻击者污染 HP 读取与区域查询）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: CombatRules.cpp:53（getDamageAfterAbsorb 四参数重载，Breach 修正 armorRatio）
// Ref: LivingEntity.cpp:540（applyArmorCalculations 从 source.directSource() 取武器查 Breach）
// Ref: BreachEnchantment.hpp:98（getArmorEffectivenessModifier = -0.15 * level）
// Ref: CombatRules.java:16-30（vanilla getDamageAfterArmor + modifyArmorEffectiveness）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// mediumglass 空腔 x∈[2,10]/z∈[1,9]，helper 相对坐标。
const ATTACKER_POS = { x: 4, y: 2, z: 5 };
const VICTIM_POS = { x: 7, y: 2, z: 5 }; // 距攻击者 3 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）

// 读取实体当前血量（HP）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给 mob 受害者装备完整钻石套（头盔/胸甲/护腿/靴子），提供护甲 20 + 盔甲韧性 8。
function equipDiamondArmor(victim: any): void {
    const eq = victim.getComponent("minecraft:equippable");
    eq.setEquipment("Head", makeItem("minecraft:diamond_helmet"));
    eq.setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
    eq.setEquipment("Legs", makeItem("minecraft:diamond_leggings"));
    eq.setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 无 Breach 重锤站立攻击穿钻石套受害者，护甲大幅减伤（正向对照，防 Breach 测试假通过）。
//
// 攻击者无附魔重锤（baseDamage 6.0 满冷却），受害者穿钻石套（armor 20 toughness 8）。
// 减伤后 finalDamage = 6*(1-0.74) = 1.56，受害者 HP 20→18.44（掉 ~1.56）。
//
// 判定：tick 30 攻击者 attackEntity，pollUntilSucceed 轮询受害者 HP ∈ [16.5, 19]（掉 1-3.5，
//   容差含冷却波动与 ATTACK_DAMAGE modifier 应用时机）。
//   - 若护甲 modifier 未应用（armor=0），受害者掉 6（HP=14），HP∈[16.5,19] 不满足→FAIL。
//   - 若攻击链路失效（0 伤害），受害者 HP=20，HP∈[16.5,19] 不满足→FAIL。
//   与 breach_iv_bypasses_diamond_armor 交叉验证：无 Breach 减伤 1.56 + Breach IV 穿透 5.16。
// Ref: CombatRules.cpp:32（三参数 getDamageAfterAbsorb，无 Breach 修正）
function noBreachMaceReducedByDiamondArmor(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any); // 0=Survival

    equipDiamondArmor(victim);
    attacker.setItem(makeItem("minecraft:mace"), 0, true); // 无附魔重锤

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    // 满冷却 finalDamage≈1.56（HP 20→18.44）。容差 [16.5,19] 覆盖冷却波动。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        return hp >= 16.5 && hp <= 19;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `no-breach mace vs diamond armor should deal ~1.56 dmg (HP 20→18.44), `
            + `but victim HP=${readHp(victim)} (if HP~14 armor modifier not applied; if HP~18.4 correct; `
            + `if HP=20 attack failed)`),
    });
}

// Breach IV 重锤站立攻击穿钻石套受害者，破甲穿透护甲（验证 Breach 运行时消费）。
//
// 攻击者 Breach IV 重锤（baseDamage 6.0 满冷却），受害者穿钻石套（armor 20 toughness 8）。
// Breach IV（-0.60）修正：f3=clamp(0.74-0.60, 0, 1)=0.14，finalDamage=6*0.86=5.16，
// 受害者 HP 20→14.84（掉 ~5.16）。
//
// 判定：tick 30 攻击者 attackEntity，pollUntilSucceed 轮询受害者 HP ∈ [13, 16]（掉 4-7，
//   容差含冷却波动）。
//   - 若 Breach 未消费（修复前 applyArmorCalculations 不查 Breach），受害者仍掉 1.56（HP~18.4），
//     HP∈[13,16] 不满足→超时 FAIL，暴露"Breach 定义了但运行时未消费"偏差。
//   - 若 Breach 修正数值错误（如作用于 effectiveArmor 而非 armorRatio），数值偏离 5.16，可能 FAIL。
//   断言 HP∈[13,16]（掉 4-7）区分 Breach 生效（5.16）与未生效（1.56）：5.16∈[4,7] 通过，1.56∉[4,7] 失败。
// Ref: CombatRules.cpp:53（四参数 getDamageAfterAbsorb，Breach 修正 armorRatio）
// Ref: LivingEntity.cpp:540（applyArmorCalculations 从攻击者武器查 breachLevel）
function breachIvBypassesDiamondArmor(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipDiamondArmor(victim);
    const mace = makeItem("minecraft:mace");
    (mace as any).addEnchantment({ type: "minecraft:breach", level: 4 });
    attacker.setItem(mace, 0, true); // Breach IV 重锤

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    // 满冷却 Breach IV finalDamage≈5.16（HP 20→14.84）。容差 [13,16]（掉 4-7）。
    // 若 Breach 未消费，受害者掉 1.56（HP~18.4）∉ [13,16] → FAIL。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        return hp >= 13 && hp <= 16;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `Breach IV mace vs diamond armor should deal ~5.16 dmg (HP 20→14.84), `
            + `but victim HP=${readHp(victim)} (if HP~18.4 Breach not consumed in applyArmorCalculations `
            + `[task #311 regression]; if HP~14.8 correct; if HP~14 armor modifier missing)`),
    });
}

export function registerBreachEnchantmentTests(): void {
    GameTest.register("MobBehaviorTests", "no_breach_mace_reduced_by_diamond_armor",
        noBreachMaceReducedByDiamondArmor)
        .batch("breach_solo")
        .structureName("gametests:mediumglass")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "breach_iv_bypasses_diamond_armor",
        breachIvBypassesDiamondArmor)
        .batch("breach_solo")
        .structureName("gametests:mediumglass")
        .maxTicks(200);
}
