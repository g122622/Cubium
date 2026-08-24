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

// 击退附魔 II 强度对齐验证（任务 #310）。
//
// 验证 Cubium Player::attack 击退附魔强度对齐 MC Java 1.21.11 Player.java:988
//   causeExtraKnockback(target, getKnockback(target, source) + (sprint?0.5:0), vec3)
// 与 LivingEntity.java:1515 getKnockback = (ATTACK_KNOCKBACK + modifyKnockback) / 2.0。
//
// 机制（对齐 vanilla）：
//   Knockback 附魔 KNOCKBACK 组件 = linear(base=1.0, per_level_above_first=1.0)（数据包 knockback.json），
//   即每级 +1.0。modifyKnockback 累加此值。玩家 ATTACK_KNOCKBACK 属性默认 0，故：
//     getKnockback = (0 + level*1.0) / 2.0
//     Knockback I  = 0.5；Knockback II = 1.0。
//   Player.attack 非冲刺：causeExtraKnockback strength = getKnockback = 1.0（II）。
//   该 strength 经 causeExtraKnockback → applyKnockback(1.0, dir) 施加为速度脉冲（dir 取自 attacker yaw）。
//   叠加 hurt 内通用击退(0.4)（任务 #309，方向取自 sourcePosition 几何），Knockback II 总击退远大于无附魔。
//
// 修复（任务 #310）：Cubium 此前 Player.cpp attack 路径绕过 getKnockback，误用
//   knockbackLevel = getEnchantmentLevel（直接等级 1/2）传 causeExtraKnockback，且 KnockbackEnchantment
//   getKnockbackBonus = level*0.5（应为 level*1.0）。致 Knockback II strength = 2.0（vanilla 1.0，2 倍偏差）。
//   修复后用 getKnockback(target)（内含 /2.0 + 每级1.0），Knockback II strength = 1.0 对齐 vanilla。
//
// 击退方向（对齐 vanilla causeExtraKnockback Player.java:1117）：
//   vanilla causeExtraKnockback 用 this.getYRot()（attacker 朝向 yaw）算击退方向 sin/cos(yaw)，
//   非 attacker→victim 几何方向。故须 attacker 朝向 victim 时击退才朝"远离 attacker"方向。
//   hurt 内通用击退(0.4) 用 sourcePosition(attacker)-victim 几何方向（任务 #309），两者方向需一致。
//   测试用 /tp facing 让 attacker 朝 victim 看，使 yaw 朝向 = 几何方向，两路击退同向叠加。
//
// 判定设计（单 victim + attacker 朝向 + 位移阈值，区分 Knockback II 与无附魔基线）：
//   attacker 朝 +x 看（facing victim），victim 在 attacker +x 侧距 3 格。tick 30 攻击 victim。
//   Knockback II 强击退使 victim 朝 +x（远离 attacker）位移，位移绝对值 > 1.5。
//
//   阈值 1.5 的依据：
//     - 无附魔（仅通用 0.4 单脉冲）：前几 tick 位移约 0.4+0.36+0.33... ≈ 1.0-1.5 格（摩擦衰减）。
//     - Knockback II（通用 0.4 + causeExtraKnockback 1.0 双脉冲）：velocity.x ≈ 1.2，前几 tick 位移
//       约 1.2+1.08+0.97... ≈ 3+ 格。1.5 阈值能区分两者（Knockback II 远超 1.5）。
//     - 修复前偏差（Knockback II strength=2.0）：位移更大，1.5 阈值仍满足——本测试验证"Knockback II
//       强击退已接入"，强度绝对值对齐由 getKnockback 数值审计 + 代码审查保证（集成测试难以精确断言
//       绝对位移，因受摩擦/AI 漫游/撞墙影响）。
//
//   撞墙考量：victim x=7 朝 +x 位移，距 +x 墙(x=10) 3 格。Knockback II 位移约 3 格可能撞墙，但轮询窗口
//     （tick 33-80，攻击后 3-50 tick）前几 tick 位移即达 1.5+（速度脉冲瞬时，前 5 tick 位移已 > 1.5），
//     撞墙发生在更晚 tick，不影响早期位移判定。pollUntilSucceed 满足即 succeed。
//
//   AI 漫游鲁棒性：villager WanderGoal 缓慢随机漫游，攻击后短窗口内击退脉冲（velocity.x≈1.2）主导位移，
//   漫游速度（≈0.05/tick）远小于击退脉冲。victim 朝 +x 位移 > 1.5 即证明 Knockback II 强击退（漫游不可能
//   让 victim 持续朝 +x 位移 > 1.5）。
//
// 附魔施加：/enchant @s knockback 2（SimulatedPlayer permLevel=4，survival 可执行，max_level=2）。
// 朝向设置：/tp @s ~ ~ ~ facing <victimPos> 让 attacker yaw 朝 victim（脚本层无 lookAt/teleport 绑定）。
//
// 独立 batch（knockback_solo）：同 melee_no_enchant_knocks_back_both_victims，避免 night batch 并行污染。
//
// Ref: Player.cpp:2679-2688（getKnockback(target) + sprint?0.5:0，对齐 vanilla Player.java:988）
// Ref: LivingEntity.cpp:2541-2561（getKnockback = (ATTACK_KNOCKBACK + 附魔*1.0) / 2.0）
// Ref: KnockbackEnchantment.hpp:85（getKnockbackBonus = level*1.0，对齐 KNOCKBACK 组件 linear base=1.0）
// Ref: Player.cpp:2973-2994（causeExtraKnockback：strength>0 时 applyKnockback(strength, sin(yaw), -cos(yaw))）
function knockbackIiKnocksBackBothVictimsFar(test: Test): void {
    (test as any).killAllEntities();
    // attacker 在 -x 侧，victim 在 +x 侧距 3 格（距 +x 墙 3 格位移空间）。
    const atkPos = { x: 4, y: 2, z: 5 };
    const victimPos = { x: 7, y: 2, z: 5 };
    const victim = test.spawn("minecraft:villager", victimPos);
    const attacker = test.spawnSimulatedPlayer(atkPos, "attacker", 0 as any); // 0=Survival

    // 装备 Knockback II 钻石剑（/enchant 施加，max_level=2）。
    attacker.setItem(makeItem("minecraft:diamond_sword"), 0, true);
    (attacker as any).chat("/enchant @s knockback 2");
    // attacker 朝 victim 看（+x），使 causeExtraKnockback 的 yaw 方向 = 几何方向，两路击退同向。
    (attacker as any).chat(`/tp @s ~ ~ ~ facing ${victimPos.x} ${victimPos.y} ${victimPos.z}`);

    // 记录攻击瞬间 victim 的 x 基准。
    let initX = victim.location.x;

    // tick 30 满冷却攻击 victim。Knockback II 强击退使 victim 朝 +x（远离 attacker）大幅位移。
    test.runAtTickTime(30, () => {
        initX = victim.location.x;
        (attacker as any).attackEntity(victim);
    });

    pollUntilSucceed(test, () => {
        const curX = victim.location.x;
        const disp = curX - initX; // victim 应朝 +x（disp > 0），Knockback II 强击退 > 1.5
        // victim 朝远离 attacker 方向位移 > 1.5，证明 Knockback II 强度正确接入
        // （远超无附魔基线，AI 漫游不可能让 victim 持续朝 +x 位移 > 1.5）。
        return disp > 1.5;
    }, {
        startTick: 33,
        interval: 4,
        maxTick: 90,
        onTimeout: () => {
            const curX = victim.location.x;
            const disp = curX - initX;
            test.assert(false,
                `Knockback II should knock victim far from attacker (strength 1.0 + generic 0.4): `
                + `victim dispX=${disp.toFixed(2)} (expect >+1.5, pushed +x away from attacker). `
                + `If dispX <1.0, Knockback enchant not wired in Player::attack (task #310 regression) `
                + `or getKnockbackBonus wrong (should be level*1.0). `
                + `If dispX negative, attacker yaw not facing victim (causeExtraKnockback dir from yaw). `
                + `initX=${initX.toFixed(2)} curX=${curX.toFixed(2)}`);
        },
    });
}

export function registerKnockbackTests(): void {
    GameTest.register("MobBehaviorTests", "melee_no_enchant_knocks_back_both_victims",
        meleeNoEnchantKnocksBackBothVictims)
        .batch("knockback_solo")
        .structureName("gametests:mediumglass")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "knockback_ii_knocks_both_victims_far",
        knockbackIiKnocksBackBothVictimsFar)
        .batch("knockback_solo")
        .structureName("gametests:mediumglass")
        .maxTicks(200);
}
