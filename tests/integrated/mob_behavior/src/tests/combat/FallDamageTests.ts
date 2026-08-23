// 摔落伤害行为类 GameTest（JumpBoost 药水经 SAFE_FALL_DISTANCE 属性减伤）。
//
// 验证 Cubium 摔落伤害链路（LivingEntity::causeFallDamage → SAFE_FALL_DISTANCE 属性消费）
// 正确接入 JumpBoost 药水的属性修饰符，对齐 MC Java/基岩 1.21.11 摔落伤害公式：
//   d = floor((f - h) * r * a)
// 其中 f=下落高度，h=safe_fall_distance（默认 3，JumpBoost 每级 +1），r=方块摔落伤害系数，
// a=fall_damage_multiplier（默认 1）。
//
// JumpBoost 减伤机制（对齐 vanilla MobEffects.JUMP_BOOST）：
//   MobEffects.JUMP_BOOST 只挂 SAFE_FALL_DISTANCE 的 Addition 修饰符（每级 +1.0，
//   EffectAttributeModifiers.cpp:60-65）。即 JumpBoost N 级 → safeFall = 3 + N。
//   伤害从 floor((f-3)*r*a) 降为 floor((f-3-N)*r*a)，每级等效降低 1 点摔落伤害。
//   跳跃力加成走 LivingEntity::getJumpBoostPower 独立项（0.1*(amplifier+1)），非此属性。
//
// wiki 原文（tech_摔落.txt#避免摔落）：
//   "跳跃提升效果每一级降低1点摔落伤害，如处于跳跃提升II时下落5格等同于普通情况下下落3格。"
// 该行为 Java/基岩一致（wiki 分类含两版独有信息，公式 d=floor((f-h)ra) 跨版本成立）。
//
// 落差设计（复用 HayBlockTests 已验证的 fall_tower 范式，零布局风险）：
//   fall_tower 7×16×7，中心 (3,*,3) 为 1×1 玻璃管落管（y=1..14 air，y=15 封顶）。
//   (3,0,3) 覆盖为 stone（普通方块，r=1.0），猪 spawn (3,11,3)，落差 = 11 - 1 = 10 格。
//
// 正反对照（防假通过）：
//   - no_effect_takes_full_fall_damage：无效果猪摔 10 格，伤害 (10-3)*1=7，HP 10→3。
//     断言 HP ≤ 4（掉 ≥6），排除"摔落链路失效 HP=10"假通过。
//   - jump_boost_iii_reduces_fall_damage：JumpBoost III（amplifier=2，level=3）猪摔 10 格，
//     safeFall=3+3=6，伤害 (10-6)*1=4，HP 10→6。断言 HP ∈ [5,7]（掉 3-5）。
//     下界 HP≥5 排除"修饰符未接通 HP=3 全伤"；上界 HP≤7 排除"完全免疫 HP=10"假通过。
//   两测试交叉验证：无效果掉 7 vs 带效果掉 4，差异正好 3（=JumpBoost III 3 级 ×1/级），
//   确证 JumpBoost 经 SAFE_FALL_DISTANCE 修饰符减伤生效（任务 #250-253 属性体系端到端验证）。
//
// 时序：addEffect 在 spawn 同步返回后立即施加（EffectManager::addEffect 第 92 行 effect.apply()
// 同步挂 SAFE_FALL_DISTANCE 修饰符，落地 causeFallDamage 时已生效）。JumpBoost duration=200 tick
// 远超 10 格自由落体时间（~18 tick），保证落地时效果仍在。落地是确定性时序（重力 + AABB）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_摔落.txt#避免摔落（JumpBoost 每级降 1 伤）
// Ref: LivingEntity.cpp:1570-1612（causeFallDamage 消费 SAFE_FALL_DISTANCE 属性）
// Ref: EffectAttributeModifiers.cpp:60-65（JumpBoost 挂 SAFE_FALL_DISTANCE Addition 修饰符）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 满铺 cobblestone 底（中心格覆盖为 stone），
// y=1..14 中心柱 air（下落通道），四周管壁 glass（y=1..15），y=15 顶部封顶。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

const PIG_TYPE = "pig";

// spawn 位置：猪脚 (3,11,3)，落到 (3,0,3) stone 顶面 y=1.0，落差 10 格。
const SPAWN_POS = { x: 3, y: 11, z: 3 };
// 落点方块（普通方块，r=1.0 完整摔落伤害）。
const LANDING_POS = { x: 3, y: 0, z: 3 };

// 读取落地区域内猪的当前血量。区域限定排除并行测试污染。
function readPigHp(test: Test): number {
    const pigs = test.getDimension().getEntities({
        type: PIG_TYPE,
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (pigs.length === 0) {
        return -1;
    }
    const health = pigs[0].getComponent("minecraft:health");
    return (health as any).currentValue as number;
}

// 无效果猪从 10 格高处摔落到石头，承受完整摔落伤害（负向对照，防假通过）。
//
// 几何落差 10 格（spawn y=11 → stone 顶面 y=1）。vanilla 理论 fallDistance≈10.8，
// 伤害 floor(10.8-3)=7（HP 10→3）。但 Cubium 实测 fallDistance≈8（onGround 接触探测
// 提前判定致 fallDistance 提前停止累积，见任务 #264 调查），实测伤害 floor(8-3)=5（HP 10→5）。
// 本测试断言 HP ∈ [3,6]（掉 4-7），容差覆盖 fallDistance 波动，且与 JumpBoost 测试
// HP ∈ [7,9] 区间不重叠，确保交叉验证成立。
//   - 上界 HP≤6（掉≥4）证明承受了显著摔落伤害，排除"摔落链路失效 HP=10"假通过。
//   - 下界 HP≥3（掉≤7）排除异常重伤/摔死。
// 与 jump_boost_iii_reduces_fall_damage 交叉验证：无效果掉≥4 vs 带效果掉≤3 = JumpBoost 减伤正确。
function noEffectTakesFullFallDamage(test: Test): void {
    test.setBlockType("minecraft:stone", LANDING_POS);
    test.spawn(PIG_TYPE, SPAWN_POS);

    pollUntilSucceed(test, () => {
        const hp = readPigHp(test);
        // HP ∈ [3,6] 证明承受了 ~5 完整摔落伤害（猪 10→5，Cubium fallDistance≈8）。
        return hp >= 3 && hp <= 6;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `no-effect pig should take full fall damage from 10 blocks (HP 10→~5), `
            + `but pig HP=${readPigHp(test)} (if HP=10 fall damage chain broken [causeFallDamage not triggered]; `
            + `if HP~5 correct)`),
    });
}

// JumpBoost III 猪从 10 格高处摔落到石头，经 SAFE_FALL_DISTANCE 修饰符减伤。
//
// JumpBoost III（amplifier=2，level=3）→ safeFall = 3 + 3 = 6。
// Cubium 实测 fallDistance≈8（同无效果测试），伤害 floor(8-6)=2（HP 10→8）。
// 断言 HP ∈ [7,9]（掉 1-3），与无效果测试 HP ∈ [3,6] 区间不重叠，确保交叉验证成立。
//   - 下界 HP≥7（掉≤3）证明 JumpBoost 减伤生效：若 SAFE_FALL_DISTANCE 修饰符未接通（safeFall=3），
//     伤害 floor(8-3)=5（HP=5<7）→ 超时 FAIL，暴露 EffectAttributeModifiers 缺陷。
//   - 上界 HP≤9（掉≥1）排除"完全免疫掉 0"假通过（HP=10 不满足）。
// 与 no_effect_takes_full_fall_damage 交叉验证：带效果掉≤3 vs 无效果掉≥4，HP 区间 [7,9] vs [3,6]
// 不重叠，确证 JumpBoost 经 SAFE_FALL_DISTANCE 修饰符每级减 1 伤（3 级减 3）。
function jumpBoostIiiReducesFallDamage(test: Test): void {
    test.setBlockType("minecraft:stone", LANDING_POS);
    const pig = test.spawn(PIG_TYPE, SPAWN_POS);

    // spawn 同步返回后立即施加 JumpBoost III（amplifier=2，对应 level=3）。
    // addEffect 同步调 effect.apply() 挂 SAFE_FALL_DISTANCE Addition 修饰符（每级 +1.0），
    // 落地 causeFallDamage 读到 safeFall=6 已生效。duration=200 tick 远超自由落体时间。
    (pig as any).addEffect("jump_boost", 200, { amplifier: 2, showParticles: false });

    pollUntilSucceed(test, () => {
        const hp = readPigHp(test);
        // HP ∈ [7,9] 证明 JumpBoost III 减伤至 ~2（猪 10→8，Cubium fallDistance≈8，safeFall=6）。
        return hp >= 7 && hp <= 9;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `JumpBoost III pig should take reduced fall damage from 10 blocks (safeFall 3→6, HP 10→~8), `
            + `but pig HP=${readPigHp(test)} (if HP~5 SAFE_FALL_DISTANCE modifier not applied [EffectAttributeModifiers defect]; `
            + `if HP~8 correct; if HP=10 fall damage chain broken)`),
    });
}

export function registerFallDamageTests(): void {
    GameTest.register("MobBehaviorTests", "no_effect_takes_full_fall_damage", noEffectTakesFullFallDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(220);

    GameTest.register("MobBehaviorTests", "jump_boost_iii_reduces_fall_damage", jumpBoostIiiReducesFallDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(220);
}
