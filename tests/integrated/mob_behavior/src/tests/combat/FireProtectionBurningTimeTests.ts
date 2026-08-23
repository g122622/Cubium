// 火焰保护附魔燃烧时间缩减行为类 GameTest。
//
// 验证 Cubium 火焰保护附魔经 BURNING_TIME 属性缩减被点燃后的燃烧时间
//（LivingEntity.igniteForTicks override：ceil(ticks * getAttributeValue(BURNING_TIME))）。
//
// 机制：
//   - BURNING_TIME 属性默认 1.0（满额燃烧时间）。
//   - 火焰保护附魔经 enchantment.fire_protection 修饰符（Op1 ADD_MULTIPLIED_BASE，每级 -0.15，
//     4 个盔甲槽位）缩减 BURNING_TIME：单件 IV 级 → 1.0 + 1.0×(4×-0.15) = 0.4。
//   - LivingEntity::igniteForTicks override 在设置火焰计时器前，先将传入 tick 数乘以 BURNING_TIME
//     并向上取整（ceil(ticks * BURNING_TIME)）。
//   - 火焰附加 II 近战攻击设燃烧时间 level×4 秒 = 160 tick（Player.cpp:2726 igniteForSeconds(8)）。
//     穿火焰保护 IV 胸甲受害者：ceil(160 × 0.4) = 64 tick；无附魔对照：160 tick。
//
// C++ 链路：
//   脚本 player.attackEntity(victim)（ScriptSimulatedPlayer.cpp，转发 Player::attack）
//     → Player.cpp:2617-2620 攻击前 fireAspectLevel>0 时 igniteForTicks(20)（wasBurning 判定）
//     → Player.cpp:2644 livingTarget->hurt(damageSource, totalDamage) 应用近战伤害
//     → Player.cpp:2724-2727（attacked 成功分支）：
//         livingTarget->igniteForSeconds(level * 4.0f)  // 即 igniteForTicks(160)
//     → igniteForSeconds 非虚 inline 内部调虚 igniteForTicks，按 victim 实际类型分发到
//       LivingEntity::igniteForTicks override（victim 是 LivingEntity 子类，未进一步 override）
//     → LivingEntity::igniteForTicks：burningTimeMultiplier = attributes().getValue(BURNING_TIME, 1.0)
//         scaledTicks = ceil(160 * burningTimeMultiplier)
//         Entity::igniteForTicks(scaledTicks)  // 直接写 FireComponent.m_fire
//
// 装备同步管线（关键依赖）：
//   equippable.setEquipment("Chest", stack) 写入装备数组 → 下个 tick detectEquipmentUpdates
//   → applyEnchantmentAttributeModifiers(victim, chestStack, Chest)
//   → 遍历 chestStack 附魔调 FireProtectionEnchantment::getAttributeModifiers(4)
//   → 返回 4 槽位 BURNING_TIME 修饰符，按 entry.equipmentSlot==Chest 过滤，应用 1 条
//     MultiplyBase amount=4×-0.15=-0.6 修饰符 → BURNING_TIME = 1.0 + 1.0×(-0.6) = 0.4。
//   故须在 setEquipment 后等待若干 tick（让 detectEquipmentUpdates 应用修饰符）再攻击。
//
// 受害者用 villager mob（HP 20，被动不反击，可经 equippable.setEquipment 穿护甲）：
//   - attackEntity 目标须是 Entity 原型 mob（SimulatedPlayer 因 JS 类未继承 Entity 原型不可作目标，
//     同 ArmorDamageReductionTests 范式）。
//   - villager 非火焰免疫（isImmuneToFire=false），被点燃后正常燃烧，onFireTicksRemaining 可读。
//   - villager HP 20，火焰附加 II 剑掉 7（HP→13），64/160 tick 燃烧最多再掉 3-8，不会在轮询窗口致死。
//
// 附魔施加（受害者胸甲）：
//   脚本侧 ItemStack.addEnchantment({type, level})（Cubium 扩展，直接挂 ItemStack 类，对应基岩
//   ItemEnchantableComponent.addEnchantment）。/enchant 仅对玩家生效，mob 须用 addEnchantment
//   直接给 ItemStack 设附魔后经 setEquipment 穿戴。
//
// 燃烧时间读取：
//   victim.getComponent("minecraft:onfire").onFireTicksRemaining（OnFireComponent readonly 属性，
//   读 Entity::getRemainingFireTicks() = FireComponent.m_fire）。着火时返回 OnFireComponent，
//   未着火返回 undefined。攻击后立即读应为刚设置的燃烧 tick 数（FireTickSystem 每 tick m_fire--，
//   故读到的是攻击后已递减若干 tick 的值，用容差区间覆盖）。
//
// 防假通过设计（正反对照）：
//   - fire_protection_iv_shortens_burn_time：穿火焰保护 IV 胸甲受害者被点燃后 onFireTicks ≤ 100
//     （ceil(160×0.4)=64 附近，容差覆盖 FireTickSystem 递减与取整波动）。
//     若 BURNING_TIME 属性未注册/未消费（igniteForTicks override 缺失或未乘属性），受害者燃烧 160 tick，
//     onFireTicks ≈160 > 100 → 超时 FAIL。
//     若火焰保护修饰符未应用（装备同步管线失效），BURNING_TIME=1.0，燃烧 160 → 同上 FAIL。
//   - no_enchant_full_burn_time：穿无附魔胸甲受害者被点燃后 onFireTicks ≥ 140（160 附近）。
//     若攻击链路本身设短燃烧（fireAspectLevel 判定失效或 igniteForSeconds 参数错误），受害者燃烧 <140
//     → 本测试 FAIL，从而暴露 fire_protection_iv 测试的假通过风险（"短燃烧"与火焰保护无关）。
//     两测试交叉验证：穿甲短燃烧 + 无甲满燃烧 = BURNING_TIME 缩减正确。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch + killAllEntities 清场。
//   - night batch：villager 非亡灵不自燃，但 night 避开自然刷怪追杀 villager 干扰 onFireTicks 读取。
//   - killAllEntities 清场：隔离 night 自然刷怪（zombie 追杀 villager 致 villager 受伤/死亡）。
//   - creeper_pit 无顶无雨，FireTickSystem 雨中扑灭分支（isInRain）不触发，火焰持续自然递减。
//
// 实体身份隔离：闭包持有 test.spawn 返回的 victim 句柄读 getComponent("minecraft:onfire")，
// 不按 type 区域查询，规避 night 自然刷怪污染。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑（helper y=1）。
// 玩家与目标距 2 格（ENTITY_INTERACTION_RANGE 默认 3.0 内）。
const ATTACKER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 5 };

// 读取受害者剩余燃烧 tick 数（onFireTicksRemaining）。未着火返回 -1。
// OnFireComponent 仅在 isOnFire()=true 时返回（getComponent 派发处 isOnFire 守卫），
// 故攻击前/燃烧熄灭后返回 -1。
function readFireTicks(entity: any): number {
    const onfire = entity.getComponent("minecraft:onfire");
    if (onfire === undefined) {
        return -1;
    }
    return (onfire as any).onFireTicksRemaining as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给受害者穿钻石胸甲并施加火焰保护 IV（经 addEnchantment + setEquipment）。
//   - addEnchantment({type:"minecraft:fire_protection", level:4})：直接给 ItemStack 设附魔
//     （/enchant 仅对玩家生效，mob 须用此 API）。
//   - setEquipment("Chest", stack)：写入装备数组，下个 tick detectEquipmentUpdates 应用
//     火焰保护 BURNING_TIME 修饰符（单件 IV → BURNING_TIME=0.4）。
function equipFireProtectionChest(victim: any): void {
    const chest = makeItem("minecraft:diamond_chestplate");
    chest.addEnchantment({ type: "minecraft:fire_protection", level: 4 });
    victim.getComponent("minecraft:equippable").setEquipment("Chest", chest);
}

// 给受害者穿无附魔钻石胸甲（反向对照，验证攻击链路本身设满额 160 tick 燃烧）。
function equipPlainChest(victim: any): void {
    victim.getComponent("minecraft:equippable").setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
}

// 给攻击者主手装备火焰附加 II 钻石剑。
//   - setItem(stack, 0, true)：slot=0 主手，selectSlot=true 同步选中。
//   - /enchant @s fire_aspect 2：permLevel=4（SimulatedPlayer 与 gameMode 解耦）survival 亦可执行。
function equipFireAspectSword(player: any): void {
    player.setItem(makeItem("minecraft:diamond_sword"), 0, true);
    player.chat("/enchant @s fire_aspect 2");
}

// 火焰保护 IV 胸甲缩减被点燃后的燃烧时间（验证 BURNING_TIME 属性注册 + 消费 + 火焰保护修饰符应用）。
//
// 受害者（villager）穿火焰保护 IV 钻石胸甲（BURNING_TIME=0.4），攻击者火焰附加 II 剑近战攻击点燃，
// 燃烧时间 ceil(160×0.4)=64 tick，onFireTicksRemaining 应 ≤100（64 附近，容差覆盖 FireTickSystem
// 每 tick 递减与攻击时序波动）。
//
// 判定：tick 10 setEquipment（留 20 tick 让 detectEquipmentUpdates 应用修饰符）→ tick 30 攻击
//   → pollUntilSucceed 轮询 onFireTicks ∈ [1, 100]。
//   若 BURNING_TIME 属性未注册（LivingEntity::registerAttributes 缺 burningTime）或 igniteForTicks
//   override 未乘属性（LivingEntity::igniteForTicks 缺getValue），受害者燃烧 160 tick，onFireTicks≈160
//   >100 → 超时 FAIL。
//   若火焰保护修饰符未应用（装备同步管线失效 / FireProtectionEnchantment::getAttributeModifiers 缺失），
//   BURNING_TIME=1.0，燃烧 160 → 同上 FAIL。
//
// night batch + killAllEntities：隔离自然刷怪追杀 villager 干扰 onFireTicks 读取。
// Ref: LivingEntity.cpp:igniteForTicks override（ceil(ticks * BURNING_TIME)）
// Ref: FireProtectionEnchantment.hpp:getAttributeModifiers（4 槽位 MultiplyBase 每级 -0.15）
// Ref: Player.cpp:2726（attacked 成功分支 igniteForSeconds(level*4)=igniteForTicks(160)）
function fireProtectionIvShortensBurnTime(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any); // 0=Survival

    equipFireProtectionChest(victim);
    equipFireAspectSword(attacker);

    // tick 30 攻击（留 20+ tick 让 setEquipment 后 detectEquipmentUpdates 应用火焰保护修饰符）。
    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    // 攻击后 victim 着火 ceil(160×0.4)=64 tick。onFireTicks ∈ [1,100] 覆盖递减与取整波动。
    // 下界 ≥1 排除"未着火"假通过（攻击失败/未点燃）；上界 ≤100 排除"满额 160"（BURNING_TIME 未缩减）。
    pollUntilSucceed(test, () => {
        const ticks = readFireTicks(victim);
        return ticks >= 1 && ticks <= 100;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `fire protection IV chest should shorten burn time to ceil(160*0.4)=64 ticks, `
            + `but victim onFireTicks=${readFireTicks(victim)} (if ~160 BURNING_TIME attribute not registered/`
            + `consumed or fire_protection modifier not applied; if -1 attack failed to ignite)`),
    });
}

// 无附魔胸甲受害者承受满额燃烧时间（正向对照，防 fireProtectionIvShortensBurnTime 假通过）。
//
// 受害者（villager）穿无附魔钻石胸甲（BURNING_TIME=1.0），攻击者火焰附加 II 剑近战攻击点燃，
// 燃烧时间 160 tick，onFireTicksRemaining 应 ≥140（160 附近，容差覆盖 FireTickSystem 递减）。
//
// 判定：tick 30 攻击 → pollUntilSucceed 轮询 onFireTicks ∈ [140, 165]。
//   若攻击链路本身设短燃烧（fireAspectLevel 判定失效或 igniteForSeconds 参数错误），受害者燃烧 <140
//   → 本测试 FAIL，从而暴露 fireProtectionIv 测试的"短燃烧"假通过（与火焰保护无关）。
//   上界 ≤165 排除异常超长燃烧。
//
// night batch + killAllEntities：同 fireProtectionIvShortensBurnTime。
// Ref: Player.cpp:2726（igniteForSeconds(8)=igniteForTicks(160)，BURNING_TIME=1.0 不缩减）
function noEnchantFullBurnTime(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn("minecraft:villager", VICTIM_POS);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipPlainChest(victim);
    equipFireAspectSword(attacker);

    test.runAtTickTime(30, () => {
        (attacker as any).attackEntity(victim);
    });

    // 攻击后 victim 着火 160 tick（BURNING_TIME=1.0 不缩减）。onFireTicks ∈ [140,165] 覆盖递减波动。
    // 与 fireProtectionIvShortensBurnTime 的 [1,100] 区间不重叠，交叉验证 BURNING_TIME 缩减生效。
    pollUntilSucceed(test, () => {
        const ticks = readFireTicks(victim);
        return ticks >= 140 && ticks <= 165;
    }, {
        startTick: 35,
        interval: 5,
        maxTick: 120,
        onTimeout: () => test.assert(false,
            `no-enchant chest victim should burn full 160 ticks (BURNING_TIME=1.0), `
            + `but victim onFireTicks=${readFireTicks(victim)} (if <140 fire aspect burn time wrong/short — `
            + `fireProtectionIv test may be false pass; if -1 attack failed to ignite)`),
    });
}

export function registerFireProtectionBurningTimeTests(): void {
    GameTest.register("MobBehaviorTests", "fire_protection_iv_shortens_burn_time", fireProtectionIvShortensBurnTime)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "no_enchant_full_burn_time", noEnchantFullBurnTime)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
