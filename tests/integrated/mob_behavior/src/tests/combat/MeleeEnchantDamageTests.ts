// 近战附魔伤害行为类 GameTest。
//
// 验证 Cubium 近战攻击链路（Player::attack → PlayerAttackHelper::getEnchantmentDamageBonus
// → EnchantmentHelper::getTotalDamageBonus → 各附魔 getDamageBonus）正确接入锋利/亡灵杀手/
// 节肢杀手附魔伤害，且目标判定用 EntityTypeTags 标签（对齐 MC Java 1.21.11）。
//
// 此前缺陷（任务 #203 修复）：PlayerAttackHelper::getEnchantmentDamageBonus 旧实现手动按
// getCreatureAttribute 枚举查附魔等级累加，与 DamageEnchantment::getDamageBonus 逻辑重复且用枚举。
// DamageEnchantment::getDamageBonus 也用枚举（亡灵/节肢）。修复：
//   1. PlayerAttackHelper::getEnchantmentDamageBonus 签名 CreatureAttribute→const LivingEntity*，
//      委托 EnchantmentHelper::getTotalDamageBonus(weapon, target) 汇总各附魔 getDamageBonus 虚函数。
//   2. DamageEnchantment::getDamageBonus 亡灵/节肢改查 SENSITIVE_TO_SMITE/SENSITIVE_TO_BANE_OF_ARTHROPODS
//      标签（同穿刺 SENSITIVE_TO_IMPALING 范式），移除枚举依赖。
//   3. EntityTypeTags 补全 vanilla 成员：UNDEAD/ZOMBIES 增 zombie_horse/zombie_nautilus；
//      AQUATIC 增 nautilus/zombie_nautilus。
//
// C++ 链路：
//   脚本 player.attackEntity(target)（ScriptSimulatedPlayer.cpp:983-1000，转发 Player::attack）
//     → Player::attack（Player.cpp:2524-2537）：baseDamage=ATTACK_DAMAGE（玩家1.0+钻石剑modifier6.0=7.0），
//       enchantDamage=PlayerAttackHelper::getEnchantmentDamageBonus(mainHand, livingTarget)
//     → PlayerAttackHelper::getEnchantmentDamageBonus（PlayerAttackHelper.cpp:176）委托
//       EnchantmentHelper::getTotalDamageBonus(weapon, target)
//     → EnchantmentHelper::getTotalDamageBonus（EnchantmentHelper.cpp:270）遍历武器附魔调
//       enchantment->getDamageBonus(level, target)
//     → DamageEnchantment::getDamageBonus（DamageEnchantment.cpp:57）：锋利 0.5+level*0.5（与target无关），
//       亡灵杀手 level*2.5（查 SENSITIVE_TO_SMITE.contains(target->getTypeId())），
//       节肢杀手 level*2.5（查 SENSITIVE_TO_BANE_OF_ARTHROPODS.contains(target->getTypeId())）。
//
// 伤害数值（满冷却 progress=1.0，钻石剑 baseDamage=7.0）：
//   - 无附魔：7.0（zombie 20→13）
//   - 锋利 V：7.0 + 3.0 = 10.0（zombie 20→10）
//   - 亡灵杀手 V（对亡灵 zombie）：7.0 + 12.5 = 19.5（zombie 20→0.5，近乎一击致死）
//   - 节肢杀手 V（对节肢 spider HP16）：7.0 + 12.5 = 19.5 > 16（一击致死，spider 16→0）
//   - 亡灵杀手 V（对非亡灵 villager）：7.0（villager 20→13，无加成）
// Player::attack 伤害分离：baseDamage 用二次冷却 0.2+progress²*0.8，enchantDamage 用线性冷却 progress。
// 满冷却 progress=1.0 时两者均原值。
//
// 附魔施加：脚本无 addEnchantment 绑定，用 player.chat("/enchant @s <enchant> <level>")（需 permLevel≥2，
// SimulatedPlayer permLevel=4 与 gameMode 解耦，survival 亦可执行）。附魔 ID：sharpness/smite/bane_of_arthropods。
//
// 防假通过设计（正反对照）：
//   - smite_v_devastates_undead：亡灵杀手V对 zombie（亡灵）近乎一击致死（HP≤1）。
//   - smite_v_no_bonus_to_non_undead：亡灵杀手V对 villager（非亡灵）仅基线 7 伤害（HP≈13）。
//     若 getDamageBonus 对所有生物都加 12.5（标签判定失效恒返加成），本测试 FAIL（villager HP 应 ≤1 而非 ≈13），
//     从而暴露 smite_v_devastates_undead 的假通过风险。两测试互补。
//   - bane_v_devastates_arthropod：节肢杀手V对 spider（节肢）一击致死（HP≤1）。
//   - sharpness_v_bonus：锋利V对 zombie 掉 ~10（HP≈10），证明锋利对所有生物加成且数值正确。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch。
//   - night batch 避免 zombie/skeleton 白天燃烧掉血干扰 HP 断言。
//   - zombie（亡灵）night 不燃；spider night 低亮度可能主动追击玩家，但玩家紧邻 2 格首次攻击即命中，
//     节肢杀手V 一击致死 spider，spider 无机会反击。
//   - villager 非亡灵不燃，被动不攻击玩家；night batch 可能自然刷 zombie 追杀 villager，故用 killAllEntities
//     清场 + runOnFinish 无需恢复（killAllEntities 仅清当前结构活体，世界级状态不受影响）。
//
// 实体身份隔离：用闭包直接持有 test.spawn 返回的实体句柄读 getComponent("minecraft:health").currentValue，
// 不按 type 区域查询，规避 night 自然刷怪污染。
//
// 攻击冷却：SimulatedPlayer spawn 后未攻击过，首次 attackEntity 时冷却 progress≈1.0（满冷却）。
//   玩家 spawn 后等 30 tick（就位 + AI 稳定）再首次攻击，确保满冷却 + 玩家已落地。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑（helper y=1）。
// 玩家与目标距 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）。
const PLAYER_POS = { x: 3, y: 2, z: 3 };
const TARGET_POS = { x: 3, y: 2, z: 5 };

// 读取实体当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造钻石剑 ItemStack。Cubium 的 @minecraft/server 与 @minecraft/server-gametest 依赖的
// @minecraft/server 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过（同 EnchantTests 范式）。
function makeDiamondSword(): any {
    return new ItemStack("minecraft:diamond_sword", 1);
}

// 装备附魔钻石剑到玩家主手并施加指定附魔。
//   - setItem(stack, 0, true)：slot=0 主手，selectSlot=true 同步选中。
//   - /enchant @s <enchant> <level>：permLevel=4（SimulatedPlayer 与 gameMode 解耦）survival 亦可执行。
function equipEnchantedSword(player: any, enchant: string, level: number): void {
    player.setItem(makeDiamondSword(), 0, true);
    player.chat(`/enchant @s ${enchant} ${level}`);
}

// 亡灵杀手 V 对亡灵生物（僵尸）近乎一击致死（验证 SENSITIVE_TO_SMITE 标签判定 + 近战附魔接入）。
//
// zombie 亡灵（UNDEAD/SENSITIVE_TO_SMITE 标签成员），亡灵杀手 V 加成 12.5，钻石剑 baseDamage 7.0，
// 满冷却总伤害 19.5，zombie HP 20→0.5（≤1）。
//
// 判定：tick 30 玩家首次 attackEntity（满冷却），pollUntilSucceed 轮询 zombie HP ≤ 1。
//   若近战附魔未接入（getEnchantmentDamageBonus 返 0）或标签判定失效，zombie 仅掉 7（HP=13），HP≤1 不满足→超时 FAIL。
//
// night batch：zombie 亡灵白天燃烧掉血干扰，night 避开。creeper_pit 开放坑无顶，night 无阳光 zombie 不燃。
// Ref: DamageEnchantment.cpp:57（getDamageBonus 查 SENSITIVE_TO_SMITE 标签）
// Ref: PlayerAttackHelper.cpp:176（getEnchantmentDamageBonus 委托 getTotalDamageBonus）
function smiteVDevastatesUndead(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any); // 0=Survival

    equipEnchantedSword(player, "smite", 5);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(zombie);
    });

    // 亡灵杀手 V 加成 12.5 + 钻石剑 baseDamage 7.0 = 19.5（满冷却），zombie HP 20→0.5（≤1）。
    // 修复 detectEquipmentUpdates 首帧漏应用 modifier 后，钻石剑 ATTACK_DAMAGE +6 正确生效（baseDamage 7.0）。
    pollUntilSucceed(test, () => readHp(zombie) >= 0 && readHp(zombie) <= 1, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `smite V should devastate undead zombie (7+12.5=19.5 dmg, HP 20→0.5), `
            + `but zombie HP=${readHp(zombie)} (melee enchant not wired or SENSITIVE_TO_SMITE tag broken — `
            + `expected HP<=1, if HP~13 base damage 7 but no 12.5 smite bonus, if HP~19 sword modifier not applied)`),
    });
}

// 亡灵杀手 V 对非亡灵生物（村民）仅基线伤害（正向对照，防 smiteVDevastatesUndead 假通过）。
//
// villager 非亡灵（不在 SENSITIVE_TO_SMITE 标签），亡灵杀手 V 无加成，仅钻石剑 baseDamage 7.0，
// villager HP 20→13。
//
// 判定：tick 30 玩家首次 attackEntity，pollUntilSucceed 轮询 villager HP ∈ [11,14]（掉 6-9，容差含冷却波动）。
//   若 getDamageBonus 对所有生物都加 12.5（标签判定失效恒返加成），villager HP 应 ≤1 而非 ≈13 → 本测试 FAIL。
//   若近战附魔链路整体失效，villager 仅掉 7（HP=13）本测试仍 PASS，但 smiteVDevastatesUndead 会 FAIL（zombie 也仅掉7）。
//   两测试交叉验证：zombie 被加成 + villager 不被加成 = 标签判定正确。
//
// killAllEntities 清场：night batch doMobSpawning 可能自然刷 zombie 追杀 villager 致 villager 受伤干扰 HP。
//   清场后单 spawn villager + 玩家，无追杀源。玩家 survival 持剑攻击 villager（被动不反击）。
// Ref: DamageEnchantment.cpp:69（Type::Undead 查 SENSITIVE_TO_SMITE，villager 不在标签返 0）
function smiteVNoBonusToNonUndead(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn("minecraft:villager", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any);

    equipEnchantedSword(player, "smite", 5);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(villager);
    });

    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        return hp >= 11 && hp <= 14;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `smite V should NOT bonus non-undead villager (only base 7 dmg, HP 20→13), `
            + `but villager HP=${readHp(villager)} (if HP<=1, SENSITIVE_TO_SMITE tag hit non-undead — tag broken; `
            + `if HP~13 correct)`),
    });
}

// 节肢杀手 V 对节肢生物（蜘蛛）一击致死（验证 SENSITIVE_TO_BANE_OF_ARTHROPODS 标签判定）。
//
// spider 节肢（ARTHROPOD/SENSITIVE_TO_BANE_OF_ARTHROPODS 标签成员），节肢杀手 V 加成 12.5，
// 钻石剑 baseDamage 7.0，满冷却总伤害 19.5 > spider HP 16，一击致死（HP→0）。
//
// 判定：tick 30 玩家首次 attackEntity，pollUntilSucceed 轮询 spider HP ≤ 1。
//   spider HP=16，19.5 伤害致死（HP=0）。若节肢附魔未接入，仅掉 7（HP=9），HP≤1 不满足→超时 FAIL。
//
// night batch：spider 夜晚低亮度主动追击玩家，但玩家紧邻 2 格（PLAYER_POS z=3, TARGET z=5 距 2），
//   首次攻击 tick 30 即命中，节肢杀手V 一击致死 spider 无机会反击。
// Ref: DamageEnchantment.cpp:77（Type::Arthropods 查 SENSITIVE_TO_BANE_OF_ARTHROPODS 标签）
function baneVDevastatesArthropod(test: Test): void {
    (test as any).killAllEntities();
    const spider = test.spawn("minecraft:spider", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any);

    equipEnchantedSword(player, "bane_of_arthropods", 5);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(spider);
    });

    // 节肢杀手 V 加成 12.5 + 钻石剑 baseDamage 7.0 = 19.5 > spider HP 16，一击致死。
    // spider 死亡后实体移除，getComponent("health") 返回 undefined → readHp 返回 -1，故断言接受 ≤1（含 -1=已死）。
    // 攻击前 spider HP=16>1 不满足；攻击后死亡 HP→0 或实体移除 -1，均 ≤1 满足。
    pollUntilSucceed(test, () => readHp(spider) <= 1, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `bane of arthropods V should devastate spider (7+12.5=19.5>16 HP, one-shot kill), `
            + `but spider HP=${readHp(spider)} (melee enchant not wired or SENSITIVE_TO_BANE_OF_ARTHROPODS tag broken — `
            + `expected HP<=1 or -1(killed), if HP~9 enchant damage missing)`),
    });
}

// 锋利 V 对所有生物加成（验证锋利 Type::All 不依赖 target 标签，对所有生物 +0.5+level*0.5=3.0）。
//
// zombie 受锋利 V：钻石剑 baseDamage 7.0 + 锋利 3.0 = 10.0，zombie HP 20→10。
//
// 判定：tick 30 玩家首次 attackEntity，pollUntilSucceed 轮询 zombie HP ∈ [8,12]（掉 8-12，容差含冷却波动）。
//   若锋利未接入，仅掉 7（HP=13），HP∈[8,12] 不满足→超时 FAIL。
//   锋利对所有生物生效（Type::All 分支不查 target），与亡灵/节肢杀手标签判定互补验证。
//
// night batch：zombie 亡灵白天燃烧，night 避开。
// Ref: DamageEnchantment.cpp:62（Type::All 锋利返 0.5+level*0.5，与 target 无关）
function sharpnessVBonus(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", TARGET_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "attacker", 0 as any);

    equipEnchantedSword(player, "sharpness", 5);

    test.runAtTickTime(30, () => {
        (player as any).attackEntity(zombie);
    });

    pollUntilSucceed(test, () => {
        const hp = readHp(zombie);
        return hp >= 8 && hp <= 12;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `sharpness V should add 3.0 bonus to all mobs (7+3=10 dmg, zombie HP 20→10), `
            + `but zombie HP=${readHp(zombie)} (sharpness not wired — expected HP~10, if HP~13 sharpness missing)`),
    });
}

export function registerMeleeEnchantDamageTests(): void {
    GameTest.register("MobBehaviorTests", "smite_v_devastates_undead", smiteVDevastatesUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(250);

    GameTest.register("MobBehaviorTests", "smite_v_no_bonus_to_non_undead", smiteVNoBonusToNonUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "bane_v_devastates_arthropod", baneVDevastatesArthropod)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "sharpness_v_bonus", sharpnessVBonus)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
