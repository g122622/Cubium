// 凋零（Wither）状态效果伤害行为类 GameTest。
//
// 验证 Cubium 凋零效果伤害链路（EffectInstance::_applyEffect Wither 分支 → DamageSources::wither()
// → LivingEntity::hurt）对齐 MC Java/基岩 1.21.11。核心对齐点有二：
//
// 1. 凋零可致死（与中毒不可致死的关键差异）。
//    wiki 原文（tech_凋零.txt:16）："凋零（Wither）是一种使生物持续受到伤害，且会致命的状态效果。"
//    Cubium 实现（EffectInstance.cpp:303-311）Wither 分支无 health>1 门控：
//      case EffectType::Wither: {
//          i32 interval = 40 >> m_amplifier;
//          if (m_duration % interval == 0) {
//              auto source = DamageSources::wither();
//              entity.hurt(source, 1.0f);   // 无 health>1 门控，可致死
//          }
//      }
//    对比 Poison 分支（EffectInstance.cpp:290-301）有 `if (entity.health() > 1.0f)` 门控——中毒只能把
//    实体打到 HP=1，不可致死。凋零无此门控，可将实体打到 HP=0 死亡。这是凋零与中毒的本质行为差异，
//    此前无集成测试覆盖（WitherRoseTests 仅验证凋零玫瑰施加凋零 + 亡灵免疫，未验证凋零效果本身的致死性）。
//
// 2. 凋零周期伤害数值（interval = 40 >> amplifier）。
//    wiki 原文（tech_凋零.txt:21-35 伤害周期表）：等级1周期40tick、等级2周期20tick、等级3周期10tick...
//    Cubium `i32 interval = 40 >> m_amplifier`（amplifier=0→40，amplifier=1→20，amplifier=2→10）对齐 vanilla
//    MobEffectUtil 是否ApplyEffectTickThisTick 的 `50 >> amplifier`（注：vanilla 凋零用 40>>amp，中毒用 25>>amp，
//    再生用 50>>amp，各效果独立）。每次造成 1.0 伤害（wither 伤害源）。
//
// 伤害源标签（DamageTypeTags.cpp）：Wither ∈ BYPASSES_ARMOR（绕盔甲）/BYPASSES_SHIELD（绕盾牌）/
//   BYPASSES_WOLF_ARMOR，∉ BYPASSES_EFFECTS（受抗性药水减免，对齐 vanilla——凋零受抗性减免）/
//   ∉ BYPASSES_INVULNERABILITY（受无敌帧节流）。
//
// 无敌帧与凋零周期：MAX_HURT_RESISTANT_TIME=20 tick（LivingEntity.hpp:1984）。凋零 I interval=40 tick
//   > 20 tick 无敌帧，每次伤害都在无敌帧结束后触发，全额生效不被吞。凋零 II interval=20 tick ==
//   无敌帧，边界情况可能被吞——故周期伤害数值测试用凋零 I（interval=40）避开边界。
//
// 时序（EffectInstance::tick，EffectInstance.cpp:114-148）：先用递减前 m_duration 执行 _applyEffect
//   （m_duration % interval == 0 触发），再 --m_duration。故 duration=D 时触发次数 = D/interval（含
//   D%interval==0 的首次）。凋零 I duration=200 → 触发 5 次（m_duration=200,160,120,80,40）共 5 伤害。
//
// 防假通过设计（正反对照）：
//   - wither_effect_is_lethal：凋零 I 打死 villager（20 HP，20 次×1 伤=20，800 tick 致死）。凋零无
//     health>1 门控，HP 可降到 0 死亡消失。若 Cubium 误加 health>1 门控（与中毒混淆），villager
//     停在 HP=1 永不消失 → 超时 FAIL。
//   - wither_periodic_damage_amount：凋零 I duration=200 触发 5 次共 5 伤害，villager HP 20→15。
//     断言 HP 下降 ∈[4,6]（容忍时序偏差 1 次）。若周期公式错（如 interval 算错）触发次数偏离 → FAIL。
//   - poison_not_lethal_vs_wither_lethal：正反对照——同结构内中毒 I villager 与凋零 I villager 同样
//     duration=2000。中毒 I（health>1 门控）villager HP 降到 1 后不再下降，存活 HP=1；凋零 I villager
//     HP 降到 0 死亡消失。交叉验证：中毒不致死 vs 凋零致死 = 门控差异对齐。
//
// 环境选择：grass_pen（9×5×9 玻璃墙围）限制 villager 漂移出结构。day batch（默认）即可，凋零伤害
//   不依赖光照/时间。villager_v2 HP=20 被动不反击（受凋零伤害仅 hurt 动画，不干扰 HP 读取）。
// 判定手段：用闭包持有的 test.spawn 实体句柄直接读 getComponent("minecraft:health").currentValue，
//   不依赖实体位置/类型查询（规避两个同类型 villager 无法区域区分 + night 自然刷怪污染）。
//   致死判定用 getEntities 区域查询 villager 是否消失（句柄在死亡后失效，须区域查询确认消失）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_凋零.txt:16（会致命）+ :21-35（伤害周期表）
// Ref: EffectInstance.cpp:303-311（Wither 分支，无 health>1 门控）
// Ref: EffectInstance.cpp:290-301（Poison 分支，health>1 门控，对照）
// Ref: DamageSource.hpp:874-876（DamageSources::wither() = EnvironmentalDamage(Wither)）
// Ref: DamageTypeTags.cpp:487（Wither ∈ BYPASSES_ARMOR）/ :523（BYPASSES_SHIELD）/ :575（BYPASSES_WOLF_ARMOR）
// Ref: LivingEntity.hpp:1984（MAX_HURT_RESISTANT_TIME=20，凋零 I interval=40 避开边界）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const VILLAGER_TYPE = "villager_v2";

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 凋零 I（amplifier=0，interval=40）持续伤害致死 villager（验证凋零无 health>1 门控，可致死）。
//
// villager HP=20，凋零 I 每 40 tick 1 伤（interval=40>>0=40），20 次伤害致死（20×40=800 tick）。
// 凋零 I interval=40 > MAX_HURT_RESISTANT_TIME=20，每次伤害都在无敌帧结束后触发，全额生效不被吞。
// duration=2000 远超致死时间，保证凋零效果持续到 villager 死亡。
//
// 判定：villager 在 maxTicks 内死亡（闭包句柄失效返回 NaN）。用闭包句柄而非区域查询——villager 受伤
// panic 可能漂移出 grass_pen 9×5×9 区域查询范围，区域查询 villagerCount=0 会误判通过（假通过风险）。
// 闭包句柄跟随实体不受位置影响，实体死亡后句柄失效返回 NaN，Number.isNaN 精确判定死亡。
//   - 修复正确（无 health>1 门控）：villager HP 降到 0 死亡，句柄失效 NaN → 通过。
//   - 若 Cubium 误加 health>1 门控（与中毒混淆）：villager HP 停在 1 不死，句柄有效 HP=1 → !NaN → 超时 FAIL。
//   - 若凋零伤害链路失效（hurt 未生效）：villager HP=20 不掉血，句柄有效 → !NaN → 超时 FAIL。
function witherEffectIsLethalTest(test: Test): void {
    const villager = test.spawn(VILLAGER_TYPE, { x: 4, y: 2, z: 4 });

    // tick 5 施加凋零 I（amplifier=0，对应 level=1）。duration=2000 远超致死时间（~800 tick）。
    // addEffect 同步 EffectManager::addEffect → effect.apply()，下一 tick EffectManager::tick 开始周期伤害。
    test.runAtTickTime(5, () => {
        (villager as any).addEffect("wither", 2000, { amplifier: 0, showParticles: false });
    });

    // 轮询断言：villager 死亡（闭包句柄失效 NaN）。startTick=400 给凋零足够时间累积伤害
    // （20 HP × 40 tick/伤 = 800 tick 致死，从 tick 5 起约 tick 805 死亡，startTick=400 起轮询覆盖）。
    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        return Number.isNaN(hp);
    }, {
        startTick: 400,
        interval: 20,
        maxTick: 1100,
        onTimeout: () => {
            const hp = readHp(villager);
            test.assert(false,
                `wither_effect_is_lethal: failed: villager hp=${hp} (expected NaN = killed by wither [no health>1 gate]; `
                + `if hp=1 wither wrongly gated by health>1 like poison; if hp=20 wither damage link broken; `
                + `if hp in 2..19 wither still damaging, extend maxTicks)`);
        },
    });
}

// 凋零 I（amplifier=0，interval=40）周期伤害数值验证（duration=200 触发 5 次共 5 伤害）。
//
// 凋零 I interval=40>>0=40。duration=200 时，EffectInstance::tick 用递减前 m_duration 判定
// m_duration % 40 == 0 触发：m_duration=200,160,120,80,40 各触发 1 次，共 5 次 5 伤害。
// villager HP 20→15（下降 5）。断言 HP 下降 ∈[4,6]（容忍时序偏差 1 次，如首 tick 触发时机偏移）。
//
// 凋零 I interval=40 > MAX_HURT_RESISTANT_TIME=20，每次伤害全额生效不被无敌帧吞。
// villager 不自然回血（非和平难度），HP 单调下降，周期伤害数值可稳定断言。
function witherPeriodicDamageAmountTest(test: Test): void {
    const villager = test.spawn(VILLAGER_TYPE, { x: 4, y: 2, z: 4 });

    let hp0 = NaN;

    // tick 5 读基准 HP（应满血 20），施加凋零 I（amplifier=0，interval=40，duration=200）。
    test.runAtTickTime(5, () => {
        hp0 = readHp(villager);
        (villager as any).addEffect("wither", 200, { amplifier: 0, showParticles: false });
    });

    // tick 240 读受击后 HP（duration=200 从 tick 5 起约 tick 205 结束，tick 240 留余量）。
    // 凋零 I duration=200 触发 5 次共 5 伤害，HP 20→15，断言下降 ∈[4,6]。
    test.runAtTickTime(240, () => {
        const hp1 = readHp(villager);
        const drop = hp0 - hp1;
        test.assert(drop >= 4 && drop <= 6,
            `wither_periodic_damage: failed: hp0=${hp0} hp1=${hp1} drop=${drop} `
            + `(expected drop in [4,6] = 5 wither I ticks over duration=200, interval=40; `
            + `if drop=0 wither damage link broken; if drop<4 interval too long / ticks missed; `
            + `if drop>6 interval too short / extra ticks)`);
        test.succeed();
    });
}

// 中毒不致死 vs 凋零致死正反对照（验证 health>1 门控差异对齐）。
//
// 同结构内两个 villager：poisonVillager 施加中毒 I（amplifier=0，interval=25，health>1 门控），
// witherVillager 施加凋零 I（amplifier=0，interval=40，无门控）。duration 都 2000。
//
// 中毒 I：每 25 tick 1 伤，但 health>1 门控使 HP 降到 1 后停止伤害（EffectInstance.cpp:295
//   `if (entity.health() > 1.0f)`）。villager HP 20→1 后不再下降，存活 HP=1（经 diag_poison_trajectory
//   诊断验证：t100 hp=16, t300 hp=8, t500 hp=1, t700 hp=1，门控精确生效）。
// 凋零 I：每 40 tick 1 伤，无门控，HP 20→0 死亡消失。
//
// 判定（tick 1000，远超两者伤害累积时间）：
//   - poisonVillager 用闭包句柄读 HP（存活，HP∈[1,2]）。
//   - witherVillager 用闭包句柄读 HP——实体死亡后句柄失效返回 NaN，故 Number.isNaN 判定死亡。
// 交叉验证：中毒停在 HP∈[1,2] vs 凋零句柄失效（死亡）= health>1 门控差异对齐。
//   - 若凋零误加 health>1 门控：witherVillager 也停在 HP=1，句柄有效 HP∈[1,2] → !NaN → FAIL。
//   - 若中毒漏 health>1 门控：poisonVillager 被打死，句柄失效 NaN → poisonHp NaN → FAIL。
//
// 用闭包句柄而非区域查询：grass_pen 长时间（tick 1000）下 villager 受伤 panic 可能漂移出 9×5×9
// 结构边界，区域查询 villagerCount 不可靠（曾因此误判两个 villager 都消失）。闭包句柄跟随实体，
// 不受位置影响，且实体死亡后句柄失效返回 NaN 可用于判定死亡。
function poisonNotLethalVsWitherLethalTest(test: Test): void {
    const poisonVillager = test.spawn(VILLAGER_TYPE, { x: 2, y: 2, z: 2 });
    const witherVillager = test.spawn(VILLAGER_TYPE, { x: 6, y: 2, z: 6 });

    // tick 5：poisonVillager 施加中毒 I，witherVillager 施加凋零 I。duration=2000 远超测试时长。
    test.runAtTickTime(5, () => {
        (poisonVillager as any).addEffect("poison", 2000, { amplifier: 0, showParticles: false });
        (witherVillager as any).addEffect("wither", 2000, { amplifier: 0, showParticles: false });
    });

    // tick 1000 检查（远超凋零 I 致死时间 ~800 tick 与中毒 I 降到 HP=1 时间 ~475 tick）：
    //   - poisonVillager 句柄读 HP（存活 HP∈[1,2]）。
    //   - witherVillager 句柄读 HP（死亡则 NaN）。
    test.runAtTickTime(1000, () => {
        const poisonHp = readHp(poisonVillager);
        const witherHp = readHp(witherVillager);

        // poisonVillager 存活且 HP∈[1,2]（中毒 health>1 门控，不可致死）。
        // HP==1 为门控精确生效；HP==2 为最后一次伤害时序边界。HP>2 说明门控失效或伤害不足。
        test.assert(!Number.isNaN(poisonHp) && poisonHp >= 1 && poisonHp <= 2,
            `poison_vs_wither: poisonVillager should survive at HP 1-2 (poison health>1 gate), poisonHp=${poisonHp} `
            + `(if NaN poison wrongly killed [missing health>1 gate]; if >2 poison damage insufficient)`);

        // witherVillager 死亡（句柄失效 NaN），凋零无门控可致死。
        test.assert(Number.isNaN(witherHp),
            `poison_vs_wither: witherVillager should be killed (wither no health>1 gate, handle invalid), witherHp=${witherHp} `
            + `(if witherHp is a number wither not lethal [wrongly gated like poison])`);

        test.succeed();
    });
}

export function registerWitherEffectTests(): void {
    GameTest.register("MobBehaviorTests", "wither_effect_is_lethal", witherEffectIsLethalTest)
        .structureName("gametests:grass_pen")
        .maxTicks(1200);

    GameTest.register("MobBehaviorTests", "wither_periodic_damage_amount", witherPeriodicDamageAmountTest)
        .structureName("gametests:grass_pen")
        .maxTicks(300);

    GameTest.register("MobBehaviorTests", "poison_not_lethal_vs_wither_lethal", poisonNotLethalVsWitherLethalTest)
        .structureName("gametests:grass_pen")
        .maxTicks(1100);
}
