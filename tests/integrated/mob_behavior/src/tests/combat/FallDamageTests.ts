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
//   中心柱方块（经 test.getBlock 实测）：y=0 tuff、y=1 cobblestone、y=2 air。故 pig 脚自
//   (3,11,3) 自由下落，落到 cobblestone 顶面 y=2.0，几何落差 = 11 - 2 = 9 格。
//   （任务 #273 诊断：fall_tower 结构基座中心柱有两层完整方块 tuff+cobblestone，落点在 y=2.0
//   而非 y=1.0，故 fallDistance≈9 是真实落差，非 onGround 接触探测提前停止累积。）
//
// 正反对照（防假通过）：
//   - no_effect_takes_full_fall_damage：无效果猪摔 9 格。vanilla 理论 fallDistance≈9
//     （任务 #264 已修复着地帧不累积偏差：updateFallDistance 改用 actualMovement.y 对齐
//     vanilla checkFallDamage，fallDistance 从≈8 提升到≈9），伤害 floor(9-3)=6（HP 10→4）。
//     断言 HP ∈ [3,5]（掉 5-7），与 JumpBoost 测试 HP ∈ [6,8] 区间不重叠。
//   - jump_boost_iii_reduces_fall_damage：JumpBoost III（amplifier=2，level=3）猪摔 9 格，
//     safeFall=3+3=6。伤害 floor(9-6)=3（HP 10→7）。断言 HP ∈ [6,8]（掉 2-4），与无效果测试
//     HP ∈ [3,5] 区间不重叠。
//   两测试交叉验证：无效果掉 6 vs 带效果掉 3，差异正好 3（=JumpBoost III 3 级 ×1/级），
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
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：中心柱 y=0 tuff、y=1 cobblestone（落点顶面 y=2.0）、
// y=2..14 中心柱 air（下落通道），四周管壁 glass（y=1..15），y=15 顶部封顶。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

const PIG_TYPE = "pig";

// spawn 位置：猪脚 (3,11,3)，落到 cobblestone 顶面 y=2.0，几何落差 9 格。
const SPAWN_POS = { x: 3, y: 11, z: 3 };
// 落点方块（中心柱 y=1 cobblestone，完整方块 r=1.0 完整摔落伤害）。
// 测试运行时在 LANDING_POS=(3,0,3) 放置 stone 是历史遗留，不影响落点——pig 仍落到 y=1 cobblestone
// 顶面 y=2.0（因 y=0 tuff 已在结构中，setBlockType 改的是 y=0 的 tuff→stone，落点顶面不变）。
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

// 无效果猪从 9 格高处摔落到圆石，承受完整摔落伤害（负向对照，防假通过）。
//
// 几何落差 9 格（spawn y=11 → cobblestone 顶面 y=2.0，中心柱 y=0 tuff+y=1 cobblestone 两层
// 完整方块）。Cubium 实测 fallDistance≈9（任务 #264 修复 updateFallDistance 改用
// actualMovement.y 对齐 vanilla checkFallDamage 后从≈8 提升），伤害 floor(9-3)=6（HP 10→4）。
// 本测试断言 HP ∈ [3,5]（掉 5-7），容差覆盖 fallDistance 波动，且与 JumpBoost 测试
// HP ∈ [6,8] 区间不重叠，确保交叉验证成立。
//   - 上界 HP≤5（掉≥5）证明承受了显著摔落伤害，排除"摔落链路失效 HP=10"假通过。
//   - 下界 HP≥3（掉≤7）排除异常重伤/摔死。
// 与 jump_boost_iii_reduces_fall_damage 交叉验证：无效果掉≥5 vs 带效果掉≤4 = JumpBoost 减伤正确。
function noEffectTakesFullFallDamage(test: Test): void {
    test.setBlockType("minecraft:stone", LANDING_POS);
    test.spawn(PIG_TYPE, SPAWN_POS);

    pollUntilSucceed(test, () => {
        const hp = readPigHp(test);
        // HP ∈ [3,5] 证明承受了 ~6 完整摔落伤害（猪 10→4，Cubium fallDistance≈9）。
        return hp >= 3 && hp <= 5;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `no-effect pig should take full fall damage from 9 blocks (HP 10→~4), `
            + `but pig HP=${readPigHp(test)} (if HP=10 fall damage chain broken [causeFallDamage not triggered]; `
            + `if HP~4 correct)`),
    });
}

// JumpBoost III 猪从 9 格高处摔落到圆石，经 SAFE_FALL_DISTANCE 修饰符减伤。
//
// JumpBoost III（amplifier=2，level=3）→ safeFall = 3 + 3 = 6。
// Cubium 实测 fallDistance≈9（同无效果测试，任务 #264 修复后），伤害 floor(9-6)=3（HP 10→7）。
// 断言 HP ∈ [6,8]（掉 2-4），与无效果测试 HP ∈ [3,5] 区间不重叠，确保交叉验证成立。
//   - 下界 HP≥6（掉≤4）证明 JumpBoost 减伤生效：若 SAFE_FALL_DISTANCE 修饰符未接通（safeFall=3），
//     伤害 floor(9-3)=6（HP=4<6）→ 超时 FAIL，暴露 EffectAttributeModifiers 缺陷。
//   - 上界 HP≤8（掉≥2）排除"完全免疫掉 0/1"假通过（HP≥9 不满足）。
// 与 no_effect_takes_full_fall_damage 交叉验证：带效果掉≤4 vs 无效果掉≥5，HP 区间 [6,8] vs [3,5]
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
        // HP ∈ [6,8] 证明 JumpBoost III 减伤至 ~3（猪 10→7，Cubium fallDistance≈9，safeFall=6）。
        return hp >= 6 && hp <= 8;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `JumpBoost III pig should take reduced fall damage from 10 blocks (safeFall 3→6, HP 10→~7), `
            + `but pig HP=${readPigHp(test)} (if HP~4 SAFE_FALL_DISTANCE modifier not applied [EffectAttributeModifiers defect]; `
            + `if HP~7 correct; if HP=10 fall damage chain broken)`),
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
