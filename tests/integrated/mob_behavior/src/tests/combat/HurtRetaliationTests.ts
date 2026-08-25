// "伤害被完全抵消仍记录攻击者触发反击"行为类 GameTest。
//
// 验证 LivingEntity::hurt 中"记录最近攻击者"（resolveMobResponsibleForDamage /
// resolvePlayerResponsibleForDamage 等价逻辑）在伤害被护甲/药水/吸收完全抵消（amount 归零）时
// 仍无条件执行，对齐 MC Java 1.21.11 LivingEntity.hurtServer:1208-1209。
//
// 背景（任务 #342 修复的缺陷）：
//   vanilla hurtServer 顺序：actuallyHurt（只扣血，受 f1!=0 门控）→ resolveMobResponsibleForDamage
//   （无条件 setLastHurtByMob）→ resolvePlayerResponsibleForDamage（无条件 setLastHurtByPlayer）。
//   即便伤害被护甲/药水/吸收完全抵消（actuallyHurt 内 amount<=0 提前 return 不扣血），受害方仍记录
//   "谁打了我"，供 HurtByTargetGoal / OwnerHurtByTargetGoal 反击链路消费。
//   此前 Cubium 把等价逻辑放在 actuallyHurt 内 amount<=0 提前返回**之后**，致伤害被完全抵消时
//   攻击者不被记录、反击不触发，偏离 vanilla。修复：迁到 hurt() 的 actuallyHurt 调用之后无条件执行。
//
// 抵消手段选择——抗性 V（amplifier=4 → level=5）100% 减伤到 0：
//   CombatRules::getDamageAfterResistance（CombatRules.cpp:99-113）= damage * max(0, 1 - level*0.2)。
//   level=5 → 1-1.0=0 → 减伤到 0。RESISTANCE_MAX_LEVEL=5、RESISTANCE_FACTOR=0.2（CombatRules.hpp:127-128）。
//   抗性门控 !source.is(BYPASSES_RESISTANCE)（LivingEntity.cpp:544）：玩家近战 playerAttack
//   （DamageType::PlayerAttack）不在 BYPASSES_RESISTANCE（成员={OutOfWorld, GenericKill}），抗性减伤生效。
//   不用钻石套——护甲减伤有 armor*0.2 下限减不到 0（CombatRules::getDamageAfterAbsorb），无法触发
//   amount<=0 提前返回。抗性 V 是唯一能确定性把近战伤害减到 0 的手段。
//
// #1 缺陷触发链路（修复前）：
//   玩家 attackEntity(iron_golem) → iron_golem.hurt(playerAttack, 木剑伤害)
//   → actuallyHurt → applyPotionDamageCalculations 抗性 V 减伤到 0
//   → if (amount <= 0.0f) return;（LivingEntity.cpp:472）提前返回
//   → 修复前 setLastHurtBy(player) 在此 return 之后，不执行 → lastHurtBy 未设
//   → HurtByTargetGoal::shouldExecute（TargetGoals.cpp:267）取 getLastHurtBy()=null → 不反击
//   → 玩家 HP 恒 20 → 测试超时 FAIL（回归捕获）。
//   修复后：resolve 块在 hurt() actuallyHurt 之后无条件执行（LivingEntity.cpp:270-318），setLastHurtBy(player)
//   → HurtByTargetGoal 触发 → 铁傀儡反击 → 玩家 HP<20 → 测试 PASS。
//
// 测试设计（正反对照，防假通过）：
//   - mob_retaliates_when_damage_fully_negated：受害者=自然 iron_golem + 抗性 V（减伤到 0），攻击者=
//     生存玩家穿钻石套（防反击致死）+ 主手木剑。玩家攻击铁傀儡造 0 伤害（被抗性 V 抵消），但铁傀儡
//     仍反击玩家（resolve 无条件记录攻击者）。断言玩家 HP<20。
//   - mob_retaliates_when_damage_not_negated_baseline：对照组，受害者=自然 iron_golem（无抗性），玩家
//     攻击造正常伤害，铁傀儡反击。断言玩家 HP<20。排除"attackEntity 本身失效/铁傀儡反击 goal 坏"
//     致主测试假通过——若基线也失败，说明问题不在 #1 修复而在攻击/反击链路本身。
//
// 防反击致死设计：铁傀儡 attackEntityAsMob 随机化伤害 7~21，玩家 20 血，约 13% 概率一击≥20 致死。
//   致死则玩家消失、HP 检测失败（flaky）。给攻击者玩家穿钻石套（护甲 20 + 韧性 8），反击伤害经
//   getDamageAfterAbsorb 减伤：21*(1-18.25/25)=2.74（满伤最小减伤），7→0.63。1~2 击仅掉 1~3 HP，
//   绝不致死，玩家存活可被检测 HP<20。穿钻石套不影响测试本质——玩家是攻击者，护甲只影响其承伤。
//
// 受害者用自然 iron_golem（test.spawn 创建，isPlayerCreated=false）：
//   canAttackType(PLAYER) 不走 isPlayerCreated 守卫 → 返 true → HurtByTargetGoal 可选玩家为目标反击。
//   铁傀儡 HP 100，玩家木剑 4 伤害/次（被抗性 V 抵消到 0）不致死。
//
// 判定手段：玩家攻击铁傀儡后，轮询断言玩家 HP<20（铁傀儡反击造伤害）。攻击 tick 10，反击约
//   tick 30-80（HurtByTargetGoal 评估 + MeleeAttackGoal 接近 + 攻击），maxTick=220 留余量。
//   用 pollUntilSucceed（正向断言 HP 降，条件满足即 succeed 合理）。区域限定 getEntities 防并行污染。
//
// 时序：
//   tick 5：给 iron_golem 施加抗性 V（amplifier=4，duration=400 远超测试时长，确保攻击时仍生效）。
//   tick 10：玩家 attackEntity(iron_golem)（留 spawn 注册稳定时间）。
//   tick 30+：轮询玩家 HP<20。
//
// 环境选择：glass_pit（7×5×7 开放坑，无玻璃墙阻挡寻路）+ killAllEntities 清场（隔离自然刷怪）。
//   玩家 (4,2,3) 脚下 (4,1,3) 放 glass 支撑（glass_pit y=1 air，防 Survival 玩家下落）。
//   iron_golem (3,2,3) 紧邻玩家直线 1 格（MeleeAttackGoal 攻击距离内）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: LivingEntity.cpp:270-318（hurt 内 resolve 块，actuallyHurt 之后无条件执行）
// Ref: LivingEntity.cpp:472（actuallyHurt 内 amount<=0 提前返回，#1 缺陷触发点）
// Ref: CombatRules.cpp:99-113（getDamageAfterResistance 抗性 V 减伤到 0）
// Ref: TargetGoals.cpp:267（HurtByTargetGoal::shouldExecute 取 getLastHurtBy）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const IRON_GOLEM_TYPE = "iron_golem";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取区域内某类型实体的当前 HP（currentValue）。找不到返回 NaN。
function readEntityHp(
    test: Test,
    type: string,
    from: { x: number; y: number; z: number },
    volume: { x: number; y: number; z: number },
): number {
    const ents = test.getDimension().getEntities({
        type,
        location: test.worldLocation(from),
        volume,
    });
    if (ents.length === 0) {
        return NaN;
    }
    const health = ents[0].getComponent("minecraft:health") as unknown as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给玩家穿钻石套（护甲 20 + 韧性 8），防铁傀儡反击致死（反击伤害经护甲减伤到 1~3 HP/击）。
// 玩家是攻击者，护甲只影响其承伤不影响其造伤，故不干扰"伤害被抗性抵消"的测试本质。
function equipDiamondArmor(player: any): void {
    const eq = player.getComponent("minecraft:equippable");
    eq.setEquipment("Head", makeItem("minecraft:diamond_helmet"));
    eq.setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
    eq.setEquipment("Legs", makeItem("minecraft:diamond_leggings"));
    eq.setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 伤害被抗性 V 完全抵消（减伤到 0）时，铁傀儡仍反击玩家（验证 resolve 无条件记录攻击者）。
//
// #1 缺陷核心测试：抗性 V 把玩家木剑近战伤害减到 0 → actuallyHurt 内 amount<=0 提前返回不扣血。
//   修复前：setLastHurtBy 在提前返回之后，不执行 → 铁傀儡不反击 → 玩家 HP 恒 20 → 超时 FAIL。
//   修复后：resolve 在 hurt() actuallyHurt 之后无条件执行 → setLastHurtBy(player) → 铁傀儡反击 → 玩家 HP<20 → PASS。
//
// 抗性 V（amplifier=4 → level=5）：getDamageAfterResistance = dmg * max(0, 1-5*0.2) = dmg * 0 = 0。
// 玩家木剑 baseDamage 4（满冷却），抗性 V 抵消到 0，铁傀儡 HP 100 不变。
// 铁傀儡反击 7~21，玩家穿钻石套减伤到 ~0.6~2.7/击，1~2 击后 HP<20。
function mobRetaliatesWhenDamageFullyNegated(test: Test): void {
    (test as any).killAllEntities();

    // 自然 iron_golem (3,2,3)（test.spawn 创建，isPlayerCreated=false，可反击玩家）。
    const golem = test.spawn(IRON_GOLEM_TYPE, { x: 3, y: 2, z: 3 });

    // 玩家脚下 (4,1,3) 放 glass 支撑（glass_pit y=1 air，防 Survival 玩家下落）。
    test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });

    // Survival 玩家 (4,2,3)，紧邻铁傀儡直线 1 格（MeleeAttackGoal 攻击距离内）。
    const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any); // 0=Survival
    // 穿钻石套防反击致死（玩家是攻击者，护甲不影响其造伤）。
    equipDiamondArmor(player);
    // 主手木剑 baseDamage 4（造伤害，被抗性 V 抵消到 0）。
    player.setItem(makeItem("minecraft:wooden_sword"), 0, true);

    // tick 5：给 iron_golem 施加抗性 V（amplifier=4 → level=5 → 100% 减伤到 0）。
    // duration=400 远超测试时长，确保 tick 10 攻击时抗性仍生效。
    test.runAtTickTime(5, () => {
        (golem as any).addEffect("resistance", 400, { amplifier: 4, showParticles: false });
    });

    // tick 10：玩家攻击铁傀儡（留 spawn 注册稳定时间）。木剑伤害被抗性 V 抵消到 0，
    // 但 resolve 无条件记录攻击者 → 铁傀儡 HurtByTargetGoal 触发反击。
    test.runAtTickTime(10, () => {
        const golems = test.getDimension().getEntities({
            type: IRON_GOLEM_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (golems.length > 0) {
            player.attackEntity(golems[0]);
        }
    });

    // 轮询断言玩家 HP<20（铁傀儡反击造伤害，经钻石套减伤到 1~3/击）。
    // 攻击 tick 10，反击约 tick 30-80，maxTick=220 留余量。
    pollUntilSucceed(test, () => {
        const hp = readEntityHp(test, "minecraft:player", PIT_FROM, PIT_VOLUME);
        if (Number.isNaN(hp)) return false; // 玩家未就绪或已消失
        return hp < 20;
    }, {
        startTick: 30,
        interval: 10,
        maxTick: 220,
        onTimeout: () => {
            const hp = readEntityHp(test, "minecraft:player", PIT_FROM, PIT_VOLUME);
            const golemHp = readEntityHp(test, IRON_GOLEM_TYPE, PIT_FROM, PIT_VOLUME);
            test.assert(false,
                `mob_retaliates_when_damage_fully_negated: failed: iron_golem did not counterattack when `
                + `damage fully negated by Resistance V (resolve not recording attacker when amount<=0), `
                + `player hp=${hp} golem hp=${golemHp} `
                + `(if player hp=20 retaliation not triggered — #1 regression; if golem hp<100 attack dealt `
                + `damage meaning Resistance V not negating — check amplifier/duration)`);
        },
    });
}

// 对照基线：无抗性时玩家攻击铁傀儡造正常伤害，铁傀儡反击（排除攻击/反击链路本身失效）。
//
// 与 mob_retaliates_when_damage_fully_negated 对称：唯一差异是不给铁傀儡抗性 V。玩家木剑造 4 伤害
// （铁傀儡 HP 100→96），铁傀儡 HurtByTargetGoal 触发反击玩家。断言玩家 HP<20。
//
// 若本基线失败（玩家 HP 恒 20），说明 attackEntity 造伤害链路 / HurtByTargetGoal 反击链路本身失效，
// 主测试的"抗性抵消仍反击"是假通过（攻击根本没生效或反击 goal 坏，与 #1 修复无关）。
// 两个测试互补：基线证明攻击+反击链路通 + 主测试证明抗性抵消仍记录攻击者 = #1 修复有效。
function mobRetaliatesWhenDamageNotNegatedBaseline(test: Test): void {
    (test as any).killAllEntities();

    // 自然 iron_golem (3,2,3)，无抗性（玩家攻击造正常伤害）。
    test.spawn(IRON_GOLEM_TYPE, { x: 3, y: 2, z: 3 });

    // 玩家脚下 glass 支撑。
    test.setBlockType("minecraft:glass", { x: 4, y: 1, z: 3 });

    // Survival 玩家 (4,2,3)，穿钻石套防反击致死 + 主手木剑。
    const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "attacker", 0 as any);
    equipDiamondArmor(player);
    player.setItem(makeItem("minecraft:wooden_sword"), 0, true);

    // tick 10：玩家攻击铁傀儡（无抗性，造 4 伤害）。铁傀儡 HurtByTargetGoal 触发反击。
    test.runAtTickTime(10, () => {
        const golems = test.getDimension().getEntities({
            type: IRON_GOLEM_TYPE,
            location: test.worldLocation(PIT_FROM),
            volume: PIT_VOLUME,
        });
        if (golems.length > 0) {
            player.attackEntity(golems[0]);
        }
    });

    // 轮询断言玩家 HP<20（铁傀儡反击造伤害）。
    pollUntilSucceed(test, () => {
        const hp = readEntityHp(test, "minecraft:player", PIT_FROM, PIT_VOLUME);
        if (Number.isNaN(hp)) return false;
        return hp < 20;
    }, {
        startTick: 30,
        interval: 10,
        maxTick: 220,
        onTimeout: () => {
            const hp = readEntityHp(test, "minecraft:player", PIT_FROM, PIT_VOLUME);
            const golemHp = readEntityHp(test, IRON_GOLEM_TYPE, PIT_FROM, PIT_VOLUME);
            test.assert(false,
                `mob_retaliates_when_damage_not_negated_baseline: failed: iron_golem did not counterattack `
                + `when damage NOT negated (attackEntity or HurtByTargetGoal link itself broken — main test `
                + `may be false-passing), player hp=${hp} golem hp=${golemHp}`);
        },
    });
}

export function registerHurtRetaliationTests(): void {
    GameTest.register("MobBehaviorTests", "mob_retaliates_when_damage_fully_negated", mobRetaliatesWhenDamageFullyNegated)
        .structureName("gametests:glass_pit")
        .maxTicks(220);

    GameTest.register("MobBehaviorTests", "mob_retaliates_when_damage_not_negated_baseline", mobRetaliatesWhenDamageNotNegatedBaseline)
        .structureName("gametests:glass_pit")
        .maxTicks(220);
}
