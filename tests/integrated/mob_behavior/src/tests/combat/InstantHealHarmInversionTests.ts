// 瞬间伤害/瞬间治疗对亡灵生物反转行为类 GameTest。
//
// 验证 Cubium HealOrHarmMobEffect（InstantHealth/InstantDamage）对亡灵生物的反转判定
// 正确查询 INVERTED_HEALING_AND_HARM 标签（= #minecraft:undead）。对齐 MC Java 1.21.11
// EffectInstance.cpp:246-276 + EffectEntities.cpp:79-137 + LivingEntity.cpp:2768-2778。
//
// 反转矩阵（amplifier=0，满血实体）：
//                       非亡灵（villager）       亡灵（zombie）
//   instant_health      治疗（HP 上升）          反转→伤害（HP 下降）
//   instant_damage      伤害（HP 下降）          反转→治疗（HP 上升）
//
// 数值（wiki tech_瞬间治疗.txt / tech_瞬间伤害.txt，amplifier=0）：
//   - instant_health 非亡灵治疗 4；亡灵伤害 JE=6 / BE=4
//   - instant_damage 非亡灵伤害 6；亡灵治疗 JE=4 / BE=6
// Cubium 对齐 Java（EffectInstance.cpp：伤害 6<<amp、治疗 4<<amp）。
//
// 【防 run_diff 误报设计】瞬间伤害/治疗对亡灵的反转数值 Java 版与基岩版不一致
// （instant_health 亡灵伤害 JE=6/BE=4；instant_damage 亡灵治疗 JE=4/BE=6）。
// 若断言精确数值，run_diff 对比基岩 BDS 会恒报 error-mismatch（P2），且非 Cubium 缺陷
// （Cubium 对齐 Java 是正确的）。故本测试**全部采用定性方向断言**：
//   - 伤害方向：断言 HP 下降 ≥3（JE 6 / BE 4 / 伤害非反转均 ≥3，方向一致通过）
//   - 治疗方向：断言 HP 上升 ≥3（JE 4 / BE 6 / 治疗非反转方向，一致通过）
//   - 反转失效（isInvertedHealAndHarm 标签查询缺失/错误）→ HP 不变或反向变化 → FAIL
// 这样两端反转方向一致均通过，真实缺陷（反转失效）仍 FAIL。JE/BE 数值差异不污染对比报告。
//
// 亡灵标签判定链（LivingEntity.cpp:2768-2778 isInvertedHealAndHarm）：
//   isInvertedHealAndHarm() → EntityTypeTags::INVERTED_HEALING_AND_HARM().contains(getTypeId())
//   INVERTED_HEALING_AND_HARM 派生自 UNDEAD（EntityTypeTags.cpp:700-702）。
//   zombie 在 UNDEAD 标签内（亡灵），villager 不在。
//
// 测试矩阵（四测试，单一职责，纯定性方向）：
//   1. instant_health_harms_undead：zombie 满血→instant_health→HP 下降（反转伤害）
//   2. instant_damage_harms_non_undead：villager 满血→instant_damage→HP 下降（正常伤害）
//   3. instant_damage_heals_undead：zombie 先 instant_health 残血→instant_damage→HP 上升（反转治疗）
//   4. instant_health_heals_non_undead：villager 先 instant_damage 残血→instant_health→HP 上升（正常治疗）
//
// 测试 3/4 的"预残血"步骤本身依赖反转正确性：若反转坏了，预残血也失败，测试 1/2 先 FAIL 暴露。
// 这是有意设计的分层依赖——测试 1/2 验证反转造伤，测试 3/4 验证反转治疗，互补不冗余。
//
// 实体身份隔离：用闭包直接持有 test.spawn 返回的实体句柄读 getComponent，不按 type 区域查询。
// night batch 下 doMobSpawning 默认开启，creeper_pit 开放坑低亮度可能在结构内自然刷怪，
// 按 type 查询会污染；闭包持句柄只读自己 spawn 的实体，规避污染。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_瞬间治疗.txt（亡灵反转：非亡灵治疗4/亡灵伤害6）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_瞬间伤害.txt（亡灵反转：非亡灵伤害6/亡灵治疗4）
// Ref: src\common\entity\effect\EffectInstance.cpp:246-276（InstantHealth/InstantDamage 反转分支）
// Ref: src\common\entity\core\LivingEntity.cpp:2768-2778（isInvertedHealAndHarm 查 INVERTED_HEALING_AND_HARM）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// 读取实体当前血量（HP）。返回 -1 表示组件未就绪。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 瞬间治疗对亡灵生物（僵尸）造成反转伤害（验证 INVERTED_HEALING_AND_HARM 标签反转造伤分支）。
//
// zombie 满血 20，tick 5 施加 instant_health（amplifier=0），亡灵反转→伤害 6，HP 20→14。
// tick 30 断言 HP 下降 ≥3（JE 6 / BE 4 均 ≥3，方向一致通过）。
//   若 isInvertedHealAndHarm 标签查询缺失/错误（zombie 不在 INVERTED_HEALING_AND_HARM），
//   instant_health 走非反转治疗分支，zombie 满血治疗无效 HP=20，HP 下降 0 <3 → FAIL。
//
// night batch：zombie 亡灵白天燃烧扣血干扰 HP，night batch 避开阳光。creeper_pit 开放坑无顶。
function instantHealthHarmsUndead(test: Test): void {
    const zombie = test.spawn("zombie", { x: 3, y: 2, z: 3 });

    test.runAtTickTime(5, () => {
        (zombie as any).addEffect("instant_health", 1, { amplifier: 0 });
    });

    test.runAtTickTime(30, () => {
        const hp = readHp(zombie);
        const damage = hp >= 0 ? 20 - hp : -1;
        test.assert(hp >= 0 && (20 - hp) >= 3,
            `instant_health should harm undead zombie (inversion), hp=${hp} damage=${damage} `
            + `(expected HP drop >=3; if HP==20 inversion broken — zombie not in INVERTED_HEALING_AND_HARM tag)`);
        test.succeed();
    });
}

// 瞬间伤害对非亡灵生物（村民）造成正常伤害（验证非反转伤害分支 + 正向对照）。
//
// villager 满血 20，tick 5 施加 instant_damage（amplifier=0），非亡灵→伤害 6，HP 20→14。
// tick 30 断言 HP 下降 ≥3。
//   若 instant_damage 链路失效（addEffect 未接通/EffectManager 未派发），HP=20 下降 0 <3 → FAIL。
//
// night batch：villager 非亡灵不燃，但 night 自然刷怪可能追杀 villager，故 killAllEntities 清场。
function instantDamageHarmsNonUndead(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn("villager", { x: 3, y: 2, z: 3 });

    test.runAtTickTime(5, () => {
        (villager as any).addEffect("instant_damage", 1, { amplifier: 0 });
    });

    test.runAtTickTime(30, () => {
        const hp = readHp(villager);
        const damage = hp >= 0 ? 20 - hp : -1;
        test.assert(hp >= 0 && (20 - hp) >= 3,
            `instant_damage should harm non-undead villager, hp=${hp} damage=${damage} `
            + `(expected HP drop >=3; if HP==20 addEffect/instant_damage chain broken)`);
        test.succeed();
    });
}

// 瞬间伤害对亡灵生物（僵尸）造成反转治疗（验证 INVERTED_HEALING_AND_HARM 标签反转治疗分支）。
//
// 三阶段：
//   1. tick 5 施加 instant_health→zombie 反转受伤残血（20→14，amplifier=0 伤害 6）。
//   2. tick 10 记录残血基线 hpWounded（验证 instant_health 造伤成功，为治疗测试铺垫）。
//   3. tick 15 施加 instant_damage→亡灵反转治疗 4（amplifier=0），HP 14→18（上升 ≥3）。
//   4. tick 30 断言 HP 相对 hpWounded 上升 ≥3。
//   若反转治疗失效（instant_damage 对亡灵走伤害分支而非治疗），HP 继续下降而非上升 → FAIL。
//
// 依赖测试 1（instant_health_harms_undead）验证的反转造伤正确性作为前置。
function instantDamageHealsUndead(test: Test): void {
    const zombie = test.spawn("zombie", { x: 3, y: 2, z: 3 });

    let hpWounded = 20;

    // 阶段1：instant_health 对亡灵反转造伤（20→14）。
    test.runAtTickTime(5, () => {
        (zombie as any).addEffect("instant_health", 1, { amplifier: 0 });
    });

    // 阶段2：记录残血基线（证明造伤成功，hpWounded<20）。
    test.runAtTickTime(10, () => {
        hpWounded = readHp(zombie);
        test.assert(hpWounded >= 0 && hpWounded < 20,
            `instant_health should wound undead zombie first (HP<20), hp=${hpWounded} `
            + `(if HP==20 instant_health inversion broken — cannot test instant_damage heal)`);
    });

    // 阶段3：instant_damage 对亡灵反转治疗（14→18）。
    test.runAtTickTime(15, () => {
        (zombie as any).addEffect("instant_damage", 1, { amplifier: 0 });
    });

    // 阶段4：断言 HP 相对残血基线上升 ≥3。
    test.runAtTickTime(40, () => {
        const hp = readHp(zombie);
        const heal = hp >= 0 ? hp - hpWounded : -1;
        test.assert(hp >= 0 && (hp - hpWounded) >= 3,
            `instant_damage should heal undead zombie (inversion), hp=${hp} hpWounded=${hpWounded} heal=${heal} `
            + `(expected HP rise >=3; if HP fell, instant_damage inversion broken — zombie not healed)`);
        test.succeed();
    });
}

// 瞬间治疗对非亡灵生物（村民）造成正常治疗（验证非反转治疗分支 + 正向对照）。
//
// 三阶段：
//   1. tick 5 施加 instant_damage→villager 受伤害残血（20→14，amplifier=0 伤害 6）。
//   2. tick 10 记录残血基线 hpWounded（验证 instant_damage 造伤成功）。
//   3. tick 15 施加 instant_health→非亡灵正常治疗 4（amplifier=0），HP 14→18（上升 ≥3）。
//   4. tick 30 断言 HP 相对 hpWounded 上升 ≥3。
//   若治疗链路失效（instant_health 未接通/EffectManager 未派发），HP 不回升 → FAIL。
//
// 依赖测试 2（instant_damage_harms_non_undead）验证的造伤正确性作为前置。
function instantHealthHealsNonUndead(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn("villager", { x: 3, y: 2, z: 3 });

    let hpWounded = 20;

    // 阶段1：instant_damage 对非亡灵造伤（20→14）。
    test.runAtTickTime(5, () => {
        (villager as any).addEffect("instant_damage", 1, { amplifier: 0 });
    });

    // 阶段2：记录残血基线（证明造伤成功，hpWounded<20）。
    test.runAtTickTime(10, () => {
        hpWounded = readHp(villager);
        test.assert(hpWounded >= 0 && hpWounded < 20,
            `instant_damage should wound non-undead villager first (HP<20), hp=${hpWounded} `
            + `(if HP==20 instant_damage chain broken — cannot test instant_health heal)`);
    });

    // 阶段3：instant_health 对非亡灵正常治疗（14→18）。
    test.runAtTickTime(15, () => {
        (villager as any).addEffect("instant_health", 1, { amplifier: 0 });
    });

    // 阶段4：断言 HP 相对残血基线上升 ≥3。
    test.runAtTickTime(40, () => {
        const hp = readHp(villager);
        const heal = hp >= 0 ? hp - hpWounded : -1;
        test.assert(hp >= 0 && (hp - hpWounded) >= 3,
            `instant_health should heal non-undead villager, hp=${hp} hpWounded=${hpWounded} heal=${heal} `
            + `(expected HP rise >=3; if HP did not rise, instant_health heal chain broken)`);
        test.succeed();
    });
}

export function registerInstantHealHarmInversionTests(): void {
    GameTest.register("MobBehaviorTests", "instant_health_harms_undead", instantHealthHarmsUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(80);

    GameTest.register("MobBehaviorTests", "instant_damage_harms_non_undead", instantDamageHarmsNonUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(80);

    GameTest.register("MobBehaviorTests", "instant_damage_heals_undead", instantDamageHealsUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(100);

    GameTest.register("MobBehaviorTests", "instant_health_heals_non_undead", instantHealthHealsNonUndead)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(100);
}
