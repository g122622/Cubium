// 通用击退行为类 GameTest。
//
// 验证 Cubium LivingEntity::hurt 通用击退对齐 MC Java 1.21.11 LivingEntity.hurtServer:1222-1238
// 的 NO_KNOCKBACK 门控分支。
//
// 机制（对齐 vanilla LivingEntity.hurtServer:1222-1238）：
//   if (!source.is(NO_KNOCKBACK)) {
//     if (directEntity instanceof Projectile) { d0/d1 = -projectile.calculateHorizontalHurtKnockbackDirection(); }
//     else if (sourcePosition != null) { d0 = srcPos.x - x; d1 = srcPos.z - z; }
//     knockback(0.4F, d0, d1);      // 对受害者施加 0.4 强度击退（受 KNOCKBACK_RESISTANCE 减免）
//     if (!flag) indicateDamage(d0, d1);
//   }
//   即所有非 NO_KNOCKBACK 伤害源（玩家近战、mob 近战、投射物等）都施加 0.4 通用击退，方向由
//   sourcePosition（攻击者位置）指向受害者的反方向（victim 被推开远离攻击者）。
//
// 修复（任务 #309）：Cubium 此前 hurt 内 NO_KNOCKBACK 门控只对齐了 indicateDamage，通用 knockback(0.4)
// 完全缺失。致玩家无附魔近战攻击零击退（causeExtraKnockback 无附魔无冲刺时 strength=0 跳过，击退
// 完全来自 hurt 的通用 knockback(0.4)，缺失即零击退）、mob 近战及所有非特化路径伤害零击退，偏离 vanilla。
// 现在 LivingEntity.cpp:299-319 补 applyKnockback(0.4, d0, d1)。NO_KNOCKBACK 门控为既有逻辑（:299），
// 本次修复未触及门控，且 BlastProtectionKnockbackTests/CooldownScalingTests 回归通过间接验证门控未坏，
// 故本测试聚焦正向击退验证，不设易 flaky 的 NO_KNOCKBACK 负向位移对照（接触类 NO_KNOCKBACK 伤害源
// 如仙人掌/甜浆果丛要求实体移动触发，移动本身污染"不击退"位移判定）。
//
// 方向对照设计（消除单 victim 绝对位移阈值对 AI 漫游的依赖）：
//   攻击者居中，两个 villager 置于 x 轴两侧等距 2 格。击退方向 = sourcePosition(attacker) - victim：
//     victimA 在 +x 侧（x=8 > attacker x=6）：d0=6-8=-2 → velocity.x=+0.4，朝 +x（远离 attacker）。
//     victimB 在 -x 侧（x=4 < attacker x=6）：d0=6-4=+2 → velocity.x=-0.4，朝 -x（远离 attacker）。
//   断言 victimA.x 增大 且 victimB.x 减小（两者同时朝"远离 attacker"方向位移）。
//   villager 的 WanderGoal 缓慢随机漫游是独立随机方向，不可能让两个 villager 同时持续朝"远离 attacker"
//   的特定相反方向位移——方向对照对随机漫游鲁棒。
//
// 受害者用 villager 而非 cow（关键）：
//   - villager 无 PanicGoal（仅 WanderingTrader 注册 Panic，VillagerEntity.registerGoals 无），被攻击后
//     不 panic 逃跑，仅缓慢 WanderGoal 漫游，不污染击退方向。
//   - villager 无 Brain（Brain 仅 VillagerEntity 注册 HurtBySensor，但本测试 attacker 是 SimulatedPlayer，
//     全程不 remove，不触发任务 #272 的爆炸 remove 致悬垂指针 UAF）。近战 attacker 持续存活，安全。
//   villager HP 20，无附魔钻石剑满冷却 7.0 伤害后存活（HP 20→13）可测位移。
//
// 位置读取：直接用实体句柄的 .location.x（villager 句柄 location 实时反映击退位移），不依赖区域
//   getEntities 查询——后者在批内并行时会被外来 villager 污染（nearestX 误匹配外来实体）。
//
// 短窗口测量（攻击后 8-30 tick 内轮询）：
//   击退是 hurt 调用当 tick 的瞬时速度脉冲（velocity.x=±0.4），摩擦衰减下前几 tick 位移约 0.4+0.36+0.33...
//   ≈1.5-2 格。WanderGoal 的 AI 移动需多 tick 启动寻路，攻击后短窗口内击退脉冲主导位移。两 villager
//   同时朝远离方向位移即证明击退发生。
//
// 独立 batch（knockback_solo）：night batch 内多个 melee 测试并行 tick + 不清场（详见
//   docs/test/INTEGRATED_TEST.md 框架清理行为），外来 villager/攻击者会污染区域查询与 victim 位移。
//   用独立 solo batch 串行独占世界，彻底规避并行污染（参考 trader_flee_solo 等隔离范式）。
//
// 环境选择：mediumglass（12×9×11 玻璃盒空腔）。结构内 x∈[2,10]/z∈[1,9] 为空气腔，y=0 圆石地板，
//   helper y=2 对应站 y=1 地板上。空腔 9×9 水平，足够容纳 attacker+两 villager 的 x 轴布局且击退后
//   不会立即撞墙（两侧各距腔壁 2 格）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// 几何布局（mediumglass 空腔 x∈[2,10]/z∈[1,9]，helper 相对坐标）：
//   attacker (6,2,5) 居中；victimA (8,2,5) +x 侧距 attacker 2 格；victimB (4,2,5) -x 侧距 attacker 2 格。
//   两侧各距腔壁 2 格（x=8 距 x=10 墙 2 格；x=4 距 x=2 墙 2 格），击退 0.4 强度位移约 1.5-2 格不撞墙。
//   attacker 与 victim 在 z=5 同一线（z 轴无偏移），击退方向纯 x 轴，方向判定清晰。
const ATTACKER_POS = { x: 6, y: 2, z: 5 };
const VICTIM_A_POS = { x: 8, y: 2, z: 5 }; // +x 侧，击退推 +x
const VICTIM_B_POS = { x: 4, y: 2, z: 5 }; // -x 侧，击退推 -x

// 构造物品 ItemStack（两份 @minecraft/server 类型分裂，as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给攻击者主手装备无附魔钻石剑（ATTACK_DAMAGE=7.0，满冷却攻击 villager HP 20→13 存活）。
function equipDiamondSword(player: any): void {
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);
}

// 无附魔近战攻击对两侧 victim 施加相反方向击退（验证 hurt 通用击退 0.4 缺失修复）。
//
// 攻击者无附魔钻石剑，spawn 后等 30 tick（满冷却），同时记录两 victim 初始 x，攻击 victimA 与 victimB
// （同一回调内连续攻击，两者无敌帧独立无干扰）。hurt 内通用击退(0.4) 使 victimA 朝 +x、victimB 朝 -x
// 位移（各自远离 attacker）。
//
// 判定：tick 30 攻击，pollUntilSucceed 轮询 victimA.x > 初始 x（朝 +x 位移）且 victimB.x < 初始 x
//   （朝 -x 位移），且两者位移绝对值均 > 0.3（防"两者都没动"假通过）。
//   - 若通用击退缺失（修复前），两 villager 位移≈0（仅缓慢 WanderGoal 漫游），不会同时朝特定相反方向
//     位移 >0.3 → FAIL，暴露"近战零击退"偏差。
//   - 修复后两 villager 同时朝远离 attacker 方向位移 >0.3 → PASS。
//
// 方向鲁棒性：WanderGoal 漫游是独立随机方向，两个 villager 同时朝"远离 attacker"的特定相反方向位移
//   >0.3 的概率极低（持续多 tick 轮询更低）。击退脉冲方向由 attacker-victim 几何确定（纯 x 轴），
//   不受 AI 影响。故方向对照对随机漫游鲁棒。
//
// 无敌帧规避：victimA 与 victimB 是不同实体，无敌帧独立，连续攻击两者互不干扰。
// attacker 持续存活（不 remove），无 UAF 风险。
//
// Ref: LivingEntity.cpp:299-319（hurt 内 NO_KNOCKBACK 门控 + applyKnockback(0.4, d0, d1) 通用击退）
// Ref: DamageSource.cpp:36-39（EntityDamageSource::sourcePosition 返回攻击者位置）
// Ref: LivingEntity.cpp:2446-2500（applyKnockback：velocity.x = vx/2 - ratioX*strength）
function meleeNoEnchantKnocksBackBothVictims(test: Test): void {
    (test as any).killAllEntities();
    const victimA = test.spawn("minecraft:villager", VICTIM_A_POS);
    const victimB = test.spawn("minecraft:villager", VICTIM_B_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any); // 0=Survival

    equipDiamondSword(attacker);

    // 记录攻击瞬间两 victim 的 x（攻击前它们可能已缓慢漫游，以攻击 tick 的实时位置为基准）。
    let initXa = victimA.location.x;
    let initXb = victimB.location.x;

    // tick 30 满冷却攻击两 victim（同一回调内连续攻击，victimA 满冷却掉 7，victimB 紧接零冷却掉 ~1.4，
    // 两者都掉血存活，击退强度 0.4 不受冷却影响——通用击退在 hurt 内，与攻击冷却解耦）。
    test.runAtTickTime(30, () => {
        // 攻击前抓取实时 x 作为基准（villager 可能已缓慢漫游偏移初始坐标）。
        initXa = victimA.location.x;
        initXb = victimB.location.x;
        (attacker as any).attackEntity(victimA);
        (attacker as any).attackEntity(victimB);
    });

    pollUntilSucceed(test, () => {
        const curXa = victimA.location.x;
        const curXb = victimB.location.x;
        const dispA = curXa - initXa; // victimA 应朝 +x（dispA > 0）
        const dispB = curXb - initXb; // victimB 应朝 -x（dispB < 0）
        // 两 victim 同时朝远离 attacker 方向位移 >0.3，证明通用击退发生。
        return dispA > 0.3 && dispB < -0.3;
    }, {
        startTick: 33,
        interval: 4,
        maxTick: 90,
        onTimeout: () => {
            const curXa = victimA.location.x;
            const curXb = victimB.location.x;
            const dispA = curXa - initXa;
            const dispB = curXb - initXb;
            test.assert(false,
                `unenchanted melee should knock both victims away from attacker (generic knockback 0.4): `
                + `victimA dispX=${dispA.toFixed(2)} (expect >+0.3, pushed +x), `
                + `victimB dispX=${dispB.toFixed(2)} (expect <-0.3, pushed -x). `
                + `If both ~0, generic knockback(0.4) missing in LivingEntity::hurt (task #309 regression). `
                + `initXa=${initXa.toFixed(2)} initXb=${initXb.toFixed(2)} `
                + `curXa=${curXa.toFixed(2)} curXb=${curXb.toFixed(2)}`);
        },
    });
}

export function registerKnockbackTests(): void {
    GameTest.register("MobBehaviorTests", "melee_no_enchant_knocks_back_both_victims",
        meleeNoEnchantKnocksBackBothVictims)
        .batch("knockback_solo")
        .structureName("gametests:mediumglass")
        .maxTicks(200);
}
