// 护甲伤害减免行为类 GameTest。
//
// 验证 Cubium 护甲伤害减免链路（LivingEntity::actuallyHurt → applyArmorCalculations →
// CombatRules::getDamageAfterAbsorb）正确接入护甲值/盔甲韧性属性，对齐 MC Java 1.21.11
// 盾甲机制公式（tech_盔甲机制 wiki）。
//
// 减伤公式（CombatRules::getDamageAfterAbsorb，对齐 vanilla CombatRules.getDamageAfterAbsorb）：
//   protectionFactor = 2 + toughness / 4
//   effectiveArmor = clamp(armor - damage / protectionFactor, armor*0.2, 20)
//   finalDamage = damage * (1 - effectiveArmor / 25)
//
// 钻石套数值：护甲 20（头盔3+胸甲8+护腿6+靴子3）、盔甲韧性 8（每件2）。
//   攻击者钻石剑 baseDamage 7.0（满冷却）攻击穿钻石套受害者：
//     protectionFactor = 2 + 8/4 = 4
//     effectiveArmor = clamp(20 - 7/4, 20*0.2=4, 20) = clamp(18.25, 4, 20) = 18.25
//     finalDamage = 7 * (1 - 18.25/25) = 7 * 0.27 = 1.89
//   受害者 HP 20 → 18.11（掉 1.89）。
//   无护甲受害者：7.0，HP 20 → 13（掉 7）。
//   差异 1.89 vs 7.0 明显，证明护甲减伤生效。
//
// 依赖 detectEquipmentUpdates 修复（任务 #205）：护甲 ARMOR/ARMOR_TOUGHNESS modifier 走装备属性
// 应用路径，equippable.setEquipment 写入装备数组后须等首次 tick detectEquipmentUpdates 应用 modifier。
// 修复前 modifier 永不应用（护甲值恒 0）；修复后护甲 modifier 正确应用，armor=20/toughness=8 生效。
//
// 受害者用 villager（mob，HP 20，被动不反击攻击者）：
//   - mob 经 equippable.setEquipment 穿护甲（Cubium 对所有 LivingEntity 返回 equippable 组件，
//     善意扩展；基岩 mob 无 equippable，此差异按 docs/test/INTEGRATED_TEST.md 6.7 决策以 Cubium 验证为准）。
//   - attackEntity(villager) 成功（villager 是 Entity，可经 _unwrapEntity 解包；SimulatedPlayer 因
//     JS 类未继承 Entity 原型不可作 attackEntity 目标，故受害者须为 mob）。
//   - villager HP 20 与玩家同，减伤数值一致。
//
// 装备护甲：victim.getComponent("minecraft:equippable").setEquipment("Head"/"Chest"/"Legs"/"Feet", stack)。
// 攻击者主手钻石剑：player.setItem(钻石剑, 0, true) + 满冷却攻击。
//
// 防假通过设计（正反对照）：
//   - diamond_armor_reduces_melee_damage：穿钻石套受害者掉 ~1.89（远小于 7，证明护甲减伤）。
//   - no_armor_takes_full_melee_damage：无护甲受害者掉 ~7.0（证明攻击伤害基线正常，非"攻击本身失效
//     致穿甲也只掉很少"的假通过）。两测试交叉验证：穿甲减伤 + 无甲全伤 = 护甲机制正确。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch + killAllEntities 清场（隔离自然刷怪）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const ATTACKER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 5 }; // 距攻击者 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）

// 读取实体当前血量（HP）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给 mob 受害者装备完整钻石套（头盔/胸甲/护腿/靴子），提供护甲 20 + 盔甲韧性 8。
// 经 equippable 组件 setEquipment（Cubium 善意扩展，mob 有 equippable）。
function equipDiamondArmor(victim: any): void {
    const eq = victim.getComponent("minecraft:equippable");
    eq.setEquipment("Head", makeItem("minecraft:diamond_helmet"));
    eq.setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
    eq.setEquipment("Legs", makeItem("minecraft:diamond_leggings"));
    eq.setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 给攻击者主手装备钻石剑（baseDamage 7.0）。
function equipDiamondSword(player: any): void {
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);
}

// 无护甲受害者承受完整近战伤害（正向对照，防 diamond_armor_reduces_melee_damage 假通过）。
//
// 攻击者钻石剑 baseDamage 7.0（满冷却），受害者（villager）无护甲，HP 20→13（掉 7）。
//
// 判定：tick 30 攻击者 attackEntity，pollUntilSucceed 轮询受害者 HP ∈ [11,14]（掉 6-9，容差含冷却波动）。
//   若攻击链路失效（baseDamage 0），受害者不掉血 HP=20，HP∈[11,14] 不满足→超时 FAIL。
//   与 diamond_armor_reduces_melee_damage 交叉验证：无甲全伤 7 + 穿甲减伤 1.89 = 护甲机制正确。
// Ref: CombatRules.cpp:31（getDamageAfterAbsorb 护甲减伤公式）
// Ref: LivingEntity.cpp:336（actuallyHurt 调 applyArmorCalculations，无护甲 armor=0 不减伤）
function noArmorTakesFullMeleeDamage(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any); // 0=Survival

    equipDiamondSword(attacker);

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        return hp >= 11 && hp <= 14;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `no-armor victim should take full 7.0 melee dmg (diamond sword, HP 20→13), `
            + `but victim HP=${readHp(victim)} (if HP=20 attack chain broken; if HP~18.1 sword modifier not applied)`),
    });
}

// 钻石套护甲大幅减伤（验证护甲值/盔甲韧性属性接入 + 减伤公式）。
//
// 受害者（villager）穿钻石套（护甲 20，韧性 8），攻击者钻石剑 baseDamage 7.0（满冷却），
// 减伤后 finalDamage = 7*(1-18.25/25) = 1.89，受害者 HP 20→18.11（掉 ~1.89）。
//
// 判定：tick 30 攻击者 attackEntity，pollUntilSucceed 轮询受害者 HP ∈ [16.5, 19]（掉 1-3.5，容差含冷却波动）。
//   若护甲 modifier 未应用（detectEquipmentUpdates bug，armor=0），受害者掉 7（HP=13），HP∈[16.5,19] 不满足→超时 FAIL。
//   若护甲减伤公式错误，同上 FAIL。
//   断言下界 HP≥16.5（掉≤3.5）排除"无护甲全伤 7"；上界 HP≤19（掉≥1）排除"攻击失效掉 0"假通过。
// Ref: CombatRules.cpp:31（getDamageAfterAbsorb：effectiveArmor=clamp(armor-dmg/(2+toughness/4), armor*0.2, 20)）
// Ref: ArmorItem.cpp:131（注册 ARMOR/ARMOR_TOUGHNESS modifier，经 detectEquipmentUpdates 应用）
// Ref: LivingEntity.cpp:336（actuallyHurt 调 applyArmorCalculations→getDamageAfterAbsorb）
function diamondArmorReducesMeleeDamage(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipDiamondArmor(victim);
    equipDiamondSword(attacker);

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    // 满冷却 finalDamage≈1.89（HP 20→18.11）。容差 [16.5,19] 覆盖冷却波动（progress 0.9~1.0）。
    pollUntilSucceed(test, () => {
        const hp = readHp(victim);
        return hp >= 16.5 && hp <= 19;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `diamond armor (20/8) should reduce 7.0 melee dmg to ~1.89 (HP 20→18.11), `
            + `but victim HP=${readHp(victim)} (if HP~13 armor modifier not applied [detectEquipmentUpdates bug]; `
            + `if HP~18.1 correct; if HP=20 attack failed)`),
    });
}

export function registerArmorDamageReductionTests(): void {
    GameTest.register("MobBehaviorTests", "no_armor_takes_full_melee_damage", noArmorTakesFullMeleeDamage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "diamond_armor_reduces_melee_damage", diamondArmorReducesMeleeDamage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
