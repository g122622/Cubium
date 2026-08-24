// 横扫攻击行为类 GameTest。
//
// 验证 Cubium Player::attack 横扫攻击触发条件对齐 MC Java 1.21.11 Player.isSweepAttack
// 与 doSweepAttack。
//
// 横扫触发条件（对齐 vanilla Player.java:1042-1048 isSweepAttack）：
//   满冷却（progress>0.9）&& 非暴击 && 非疾跑击退 && 在地面 && 几乎静止（distanceWalked<aiMoveSpeed）
//   && 主手持剑（ItemTags.SWORDS）。
//   【关键】vanilla 不要求 SweepingEdge 附魔——无附魔剑满冷却站立攻击仍横扫周围生物。
//
// 横扫伤害公式（对齐 vanilla Player.java:1143,1151 doSweepAttack）：
//   sweepBase = 1.0 + SWEEPING_DAMAGE_RATIO属性值 × 冷却后基础伤害
//   sweepFinal = sweepBase × 冷却进度
//   无附魔（ratio=0）：sweepFinal = 1.0 × 1.0 = 1.0（满冷却）。
//
// 修复（任务 #308）：Cubium 此前误加 if (sweepRatio > 0.0f) 门控，致无附魔剑不横扫（vanilla 无附魔
// 剑满冷却站立攻击仍横扫，横扫伤害=1.0），偏离 vanilla。已移除门控，无附魔剑也横扫。
// 同时横扫伤害补乘冷却进度（对齐 vanilla sweepFinal = sweepBase × f1），此前 Cubium 漏乘。
//
// 防假通过设计（正反对照）：
//   - sword_sweeps_nearby_mob_without_enchantment：攻击者无附魔钻石剑满冷却站立攻击主目标 victimA，
//     周围 1 格内的 victimB 应受横扫伤害 1.0（HP 20→19）。若横扫门控未移除（仍要求附魔），victimB
//     不掉血（HP=20）→FAIL，暴露"无附魔不横扫"偏差。
//   - axe_does_not_sweep：攻击者钻石斧（非剑）满冷却站立攻击，victimB 不受横扫（横扫要求主手剑）。
//     若横扫未检查主手是否为剑（对所有武器横扫），victimB 掉血→FAIL。两测试交叉验证横扫触发条件。
//
// 横扫范围（对齐 vanilla doSweepAttack:1145）：victimA 碰撞箱 expand(1, 0.25, 1) 内 + 距 attacker
// ≤3 格（distanceSqTo < 9）。victimB 置于 victimA 旁 1 格（横扫范围内）且距 attacker √5≈2.24 格（<3）。
//
// 几乎静止条件：SimulatedPlayer spawn 后不施加移动指令，distanceWalkedDelta≈0 < aiMoveSpeed（0.1），
// 满足横扫的"几乎静止"要求。attackEntity 不改变玩家位置（近战无位移），故 tick 30 攻击时玩家静止。
//
// 无敌帧规避：victimA 被直接攻击（满冷却 7.0 伤害），victimB 被横扫（1.0 伤害）。两者是不同实体，
// 无敌帧独立。victimB 仅受 1.0 横扫伤害，HP 20→19，断言 HP∈[18,20]（掉 0-2，容差）。
//
// 主目标 victimA 也断言掉血（满冷却 7.0，HP 20→13），证明攻击本身命中（非"攻击未中致横扫也不触发"）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch + killAllEntities 清场。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// attacker 居中，victimA 在前方 2 格（攻击距离内），victimB 在 victimA 旁 1 格（横扫范围内）。
const ATTACKER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_A_POS = { x: 3, y: 2, z: 5 }; // 距 attacker 2 格（攻击距离 3.0 内）
const VICTIM_B_POS = { x: 4, y: 2, z: 5 }; // 距 victimA 1 格（横扫范围），距 attacker √5≈2.24 格（<3）

// 读取实体当前血量（HP）。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（两份 @minecraft/server 类型分裂，as any 绕过）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给攻击者主手装备武器。
function equipWeapon(player: any, itemId: string): void {
    player.setItem(makeItem(itemId), 0, true);
}

// 无附魔剑满冷却站立攻击横扫周围 mob（验证横扫门控移除，无附魔也横扫）。
//
// 攻击者无附魔钻石剑（主手剑），spawn 后等 30 tick（满冷却）站立攻击 victimA。
// 横扫触发（主手剑+满冷却+非暴击+非疾跑+在地面+几乎静止），victimB 受横扫伤害 1.0（HP 20→19）。
//
// 判定：tick 30 attackEntity(victimA)，pollUntilSucceed 轮询：
//   victimA HP∈[11,14]（满冷却 7.0 直接伤害）AND victimB HP∈[18,20]（横扫 1.0 伤害，掉 0-2）。
//   若横扫门控未移除（仍要求 SweepingEdge 附魔），victimB 不掉血 HP=20，HP∈[18,20] 满足但需进一步
//   确认 victimB 确实掉血——故断言 victimB HP≤19（掉≥1），排除"victimB 未受横扫"假通过。
//
// Ref: Player.cpp:2750-2755（canSweep 条件，主手剑+满冷却+非暴击+非疾跑+在地面+几乎静止）
// Ref: Player.cpp:2759（sweepDamage = (1.0 + sweepRatio*damage) * cooldownProgress，无附魔=1.0）
function swordSweepsNearbyMobWithoutEnchantment(test: Test): void {
    (test as any).killAllEntities();
    const victimA = test.spawn("minecraft:villager", VICTIM_A_POS);
    const victimB = test.spawn("minecraft:villager", VICTIM_B_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipWeapon(attacker, "minecraft:diamond_sword"); // 无附魔钻石剑

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victimA);
    });

    pollUntilSucceed(test, () => {
        const hpA = readHp(victimA);
        const hpB = readHp(victimB);
        // victimA 满冷却直接伤害 7.0（HP∈[11,14]）；victimB 横扫伤害 1.0（HP≤19，掉≥1）。
        return hpA >= 11 && hpA <= 14 && hpB <= 19 && hpB >= 17;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `unenchanted sword sweep should hit nearby mob (1.0 dmg): `
            + `victimA HP=${readHp(victimA)} (expect ~13, direct 7.0), `
            + `victimB HP=${readHp(victimB)} (expect ~19, sweep 1.0). `
            + `If victimB HP=20, sweep gate not removed (still requires SweepingEdge enchant).`),
    });
}

// 斧头不触发横扫（验证横扫要求主手剑，非剑武器不横扫）。
//
// 攻击者钻石斧（非剑），满冷却站立攻击 victimA。横扫要求主手剑（isSweepAttack 检查 ItemTags.SWORDS），
// 斧头不满足，victimB 不受横扫（HP=20）。
//
// 判定：tick 30 attackEntity(victimA)，pollUntilSucceed 轮询 victimB HP=20（不掉血）。
//   若横扫未检查主手是否为剑（对所有武器横扫），victimB 掉血 HP≤19→FAIL。
//
// 注：钻石斧 ATTACK_DAMAGE=9（1+5+3? 需核实），满冷却直接伤害高于剑，victimA 掉血更多。
//   victimA 断言掉血（HP≤17）证明攻击命中，victimB 断言不掉血（HP=20）证明斧头不横扫。
//
// Ref: Player.cpp:2757（dynamic_cast<SwordItem>，非剑 sword=nullptr 不进横扫分支）
function axeDoesNotSweep(test: Test): void {
    (test as any).killAllEntities();
    const victimA = test.spawn("minecraft:villager", VICTIM_A_POS);
    const victimB = test.spawn("minecraft:villager", VICTIM_B_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipWeapon(attacker, "minecraft:diamond_axe"); // 钻石斧（非剑）

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victimA);
    });

    pollUntilSucceed(test, () => {
        const hpA = readHp(victimA);
        const hpB = readHp(victimB);
        // victimA 斧头直接伤害（掉血，HP≤17）；victimB 不受横扫（HP=20）。
        return hpA <= 17 && hpB >= 19.5;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `axe should NOT sweep nearby mob (sweep requires sword): `
            + `victimA HP=${readHp(victimA)} (expect ≤17, axe direct dmg), `
            + `victimB HP=${readHp(victimB)} (expect 20, no sweep). `
            + `If victimB HP≤19, sweep not checking main-hand is sword.`),
    });
}

export function registerSweepAttackTests(): void {
    GameTest.register("MobBehaviorTests", "sword_sweeps_nearby_mob_without_enchantment", swordSweepsNearbyMobWithoutEnchantment)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "axe_does_not_sweep", axeDoesNotSweep)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
