// 攻击冷却缩放行为类 GameTest。
//
// 验证 Cubium Player::attack 中攻击冷却对伤害的缩放公式对齐 MC Java 1.21.11
// Player.attack（baseDamageScaleFactor / getAttackStrengthScale）。
//
// 冷却缩放公式（对齐 vanilla Player.java:957-959,1177-1180）：
//   f1 = getAttackStrengthScale(0.5F)        // 冷却进度 = (ticksSinceLastAttack+0.5)/(20/attackSpeed)
//   f2 = f1 * (getEnchantedDamage - f)       // 附魔伤害 × 线性冷却 f1
//   f *= baseDamageScaleFactor()             // 基础伤害 × 二次冷却 (0.2 + f1²×0.8)
//   baseDamageScaleFactor = 0.2 + f1*f1*0.8
//
// 即基础伤害用二次冷却系数（0.2 + progress²×0.8），附魔伤害用线性冷却系数（progress）。
// 满冷却（progress=1.0）：基础伤害 ×1.0；零冷却（progress=0）：基础伤害 ×0.2（非 0）。
//
// 钻石剑数值：ATTACK_DAMAGE = 1(玩家基础) + 3(注册) + 3(钻石 tier) = 7.0，
//             ATTACK_SPEED = 4(玩家基础) - 2.4(注册) = 1.6，cooldownPeriod = 20/1.6 = 12.5 tick。
//   满冷却攻击无护甲 villager（HP 20）：伤害 7.0，HP 20→13。
//   零冷却攻击（progress≈0.04，刚 resetCooldown）：伤害 7×(0.2+0.04²×0.8)≈7×0.201≈1.41，HP 20→18.6。
//
// 防假通过设计（正反对照）：
//   - full_cooldown_deals_full_damage：满冷却攻击掉 ~7（HP∈[11,14]），证明满冷却不衰减。
//   - partial_cooldown_reduces_damage：先满冷却攻击 victimA（掉 7），同回调内立即攻击 victimB
//     （resetCooldown 致 progress≈0.04，掉 ~1.4）。断言 victimB HP 明显高于 victimA
//     （victimB 掉≤3，victimA 掉≥5）。若冷却缩放失效（progress 恒 1.0），victimB 也掉 7，
//     victimB HP≈13 与 victimA 相近，断言不满足→FAIL，暴露"冷却缩放形同虚设"假通过。
//
// 无敌帧规避：vanilla LivingEntity 有 20 tick 无敌帧（同额伤害被吞）。两次攻击用两个不同 victim
// （victimA/victimB），避开无敌帧对第二次伤害的吞没。同一 runAtTickTime 回调内连续两次 attackEntity：
//   第一次 attackEntity(victimA)：满冷却（spawn 后等 30 tick，m_ticksSinceLastAttack=30，progress=1.0），
//     内部 resetCooldown 置 m_ticksSinceLastAttack=0。
//   第二次 attackEntity(victimB)：紧接其后，m_ticksSinceLastAttack=0，progress=(0+0.5)/12.5=0.04。
//
// 重锤下落加成双重缩放修复（任务 #308）：vanilla Player.attack:972 中 getAttackDamageBonus 返回值
// （重锤分段伤害）直接加到冷却缩放后的 f 上，返回值本身不再乘冷却。Cubium 此前误写
// totalDamage += maceSmashBonus * cooldownProgress（双重缩放），已修为 totalDamage += maceSmashBonus。
// 重锤下落攻击需玩家从高处下落（fallDistance>1.5 且 !isFallFlying），GameTest 内构造该场景复杂
// （玩家下落时序非确定），故重锤修复以代码审查 + vanilla 对齐注释保证，本测试聚焦冷却缩放公式本身。
//
// 横扫攻击附魔门控修复（任务 #308）：vanilla isSweepAttack 不要求 SweepingEdge 附魔，无附魔剑满冷却
// 站立攻击仍横扫（横扫伤害=1.0）。横扫测试见 SweepAttackTests（若存在），此处不重复。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch + killAllEntities 清场（隔离自然刷怪）。
// 两个 victim 间距 ≥3 格避免横扫误伤（横扫范围 1x0.25x1，距主目标 ≤3 格）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// attacker 居中，两个 victim 分置两侧（距 attacker 各 2 格，互相间距 4 格 > 横扫范围）。
const ATTACKER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_A_POS = { x: 3, y: 2, z: 5 }; // 距 attacker 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）
const VICTIM_B_POS = { x: 1, y: 2, z: 3 }; // 距 attacker 2 格，与 victimA 间距 4 格避免横扫误伤

// 读取实体当前血量（HP）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（两份 @minecraft/server 类型分裂，as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给攻击者主手装备钻石剑（ATTACK_DAMAGE=7.0，ATTACK_SPEED=1.6，cooldownPeriod=12.5 tick）。
function equipDiamondSword(player: any): void {
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);
}

// 满冷却攻击造成完整伤害（验证冷却恢复后伤害不衰减）。
//
// 攻击者钻石剑，spawn 后等 30 tick（m_ticksSinceLastAttack=30，progress=(30.5)/12.5=1.0 满冷却），
// 攻击无护甲 villager（HP 20），满冷却伤害 7.0，HP 20→13。
//
// 判定：tick 30 attackEntity，pollUntilSucceed 轮询 victim HP ∈ [11,14]（掉 6-9，容差含冷却波动）。
//   若攻击链路失效（baseDamage 0），HP=20 不在 [11,14]→超时 FAIL。
//   此测试同时作为 partial_cooldown_reduces_damage 的正向基线（满冷却掉 7）。
//
// Ref: Player.cpp:2648-2655（cooldownProgress 计算 + 二次冷却缩放）
// Ref: Player.cpp:2556-2570（getCooledAttackStrength 冷却进度公式）
function fullCooldownDealsFullDamage(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_A_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any); // 0=Survival

    equipDiamondSword(attacker);

    // tick 30 攻击（spawn 后 30 tick，m_ticksSinceLastAttack=30 满冷却）。
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
            `full-cooldown diamond sword should deal ~7.0 dmg (HP 20→13), `
            + `but victim HP=${readHp(victim)} (if HP=20 attack chain broken; if HP~16 sword ATTACK_DAMAGE wrong)`),
    });
}

// 冷却未恢复攻击造成衰减伤害（验证二次冷却缩放公式，核心测试）。
//
// 攻击者钻石剑，spawn 后等 30 tick（满冷却）。tick 30 同一回调内连续攻击两个 victim：
//   1. attackEntity(victimA)：满冷却 progress=1.0，伤害 7.0，victimA HP 20→13。内部 resetCooldown。
//   2. attackEntity(victimB)：紧接其后，progress=(0+0.5)/12.5=0.04，伤害 7×(0.2+0.04²×0.8)≈1.41，
//      victimB HP 20→18.6。
//
// 判定：pollUntilSucceed 轮询 victimA HP∈[11,14]（掉 6-9）且 victimB HP∈[17,20]（掉 0-3）。
//   victimB 掉血明显少于 victimA（victimB 掉≤3 vs victimA 掉≥5）证明冷却缩放生效。
//   若冷却缩放失效（progress 恒 1.0 或二次系数恒 1.0），victimB 也掉 7（HP≈13），HP∈[17,20] 不满足→FAIL。
//
// 无敌帧规避：两次攻击作用于不同 victim（victimA/victimB），无敌帧只对同一实体生效，互不干扰。
// 横扫规避：victimA 与 victimB 间距 4 格 > 横扫范围（1x0.25x1，距主目标≤3 格），victimB 不会被
//   攻击 victimA 的横扫误伤。且横扫要求主手剑 + 满冷却 + 站立，攻击 victimA 时若触发横扫只影响
//   victimA 周围 1 格内实体，victimB 在 4 格外不受影响。
//
// Ref: Player.cpp:2652-2655（quadraticCooldown=0.2+progress²×0.8，damage=baseDamage×quadraticCooldown）
// Ref: Player.cpp:2658（resetCooldown 重置 m_ticksSinceLastAttack=0）
// Ref: Player.cpp:2556-2570（getCooledAttackStrength(0.5) 取 (ticksSinceLastAttack+0.5)/cooldownPeriod）
function partialCooldownReducesDamage(test: Test): void {
    (test as any).killAllEntities();
    const victimA = test.spawn("minecraft:villager", VICTIM_A_POS);
    const victimB = test.spawn("minecraft:villager", VICTIM_B_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipDiamondSword(attacker);

    // tick 30 同一回调内连续攻击两个 victim：第一次满冷却，第二次零冷却（resetCooldown 后）。
    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victimA); // 满冷却 progress=1.0，掉 7；内部 resetCooldown
        (attacker as any).attackEntity(victimB); // 零冷却 progress≈0.04，掉 ~1.4
    });

    pollUntilSucceed(test, () => {
        const hpA = readHp(victimA);
        const hpB = readHp(victimB);
        // victimA 满冷却掉 ~7（HP∈[11,14]），victimB 零冷却掉 ~1.4（HP∈[17,20]）。
        // victimB HP 必须明显高于 victimA（差≥3），证明冷却缩放生效。
        return hpA >= 11 && hpA <= 14 && hpB >= 17 && hpB <= 20 && (hpB - hpA) >= 3;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `partial-cooldown attack should deal reduced dmg vs full-cooldown: `
            + `victimA HP=${readHp(victimA)} (expect ~13, full cooldown 7.0 dmg), `
            + `victimB HP=${readHp(victimB)} (expect ~18.6, partial cooldown ~1.4 dmg). `
            + `If victimB HP~13 (same as A), cooldown scaling is broken (progress always 1.0).`),
    });
}

export function registerCooldownScalingTests(): void {
    GameTest.register("MobBehaviorTests", "full_cooldown_deals_full_damage", fullCooldownDealsFullDamage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "partial_cooldown_reduces_damage", partialCooldownReducesDamage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
