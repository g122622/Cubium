// 盾牌格挡耐久消耗行为类 GameTest（验证 damageShield → hurtAndBreak 链路对齐 vanilla）。
//
// 验证 Cubium 盾牌格挡成功后消耗耐久度对齐 MC Java 1.21.11 BlocksAttacks.hurtBlockingItem：
// 玩家举盾格挡近战伤害时，盾牌耐久按伤害值下降（Cubium ceil(amount) 点）。
//
// C++ 链路：
//   attacker.attackEntity(victim)（ScriptSimulatedPlayer.cpp，转发 Player::attack）
//     → Player::attack（Player.cpp:2611）：baseDamage=ATTACK_DAMAGE（钻石剑 7.0），
//       满冷却 quadraticCooldown=1.0 → damage=7.0，totalDamage=7.0（无附魔）
//       → livingTarget->hurt(damageSource, totalDamage)
//     → LivingEntity::hurt → actuallyHurt（LivingEntity.cpp:342）
//       → canBlockDamageSource（Player.cpp:1570）：举盾+盾牌+BYPASSES_SHIELD 门控 → true
//       → damageShield(amount)（LivingEntity.cpp:353，amount=原始伤害 7.0）
//     → Player::damageShield（Player.cpp:1595）：durabilityCost=ceil(7.0)=7，
//       hurtAndBreak(shieldStack, 7, ...) 消耗装备槽盾牌耐久 7 点
//
// vanilla 对齐（BlocksAttacks.java:114-125 + ItemDamageFunction:181-204）：
//   hurtBlockingItem(level, stack, entity, hand, f=resolveBlockedDamage 结果)
//     → i = itemDamage.apply(f) = (f < threshold ? 0 : floor(base + factor*f))
//   shield 用 ItemDamageFunction DEFAULT=(threshold=1.0, base=0.0, factor=1.0)：
//     f>=1.0 → floor(0 + 1.0*f) = floor(f)
//   注：vanilla 的 f 是 resolveBlockedDamage 结果（经 DamageReduction 公式的被格挡伤害），
//   Cubium 简化格挡模型（canBlockDamageSource 命中即全格挡）传原始 amount，无 DamageReduction 体系。
//   对整数伤害（钻石剑 7.0）：Cubium ceil(7.0)=7 == vanilla floor(7.0)=7，无偏差。
//   差异仅在小数伤害：Cubium ceil 比 vanilla floor 多 1（已知简化偏差，待 BlocksAttacks 组件体系接入后对齐，
//   TODO: 见 Player.cpp:1600 注释）。
//
// 已知架构差异（非本测试修复范围，记录供后续对齐）：
//   1. Cubium 无 DataComponents.BLOCKS_ATTACKS 体系，格挡判定硬编码（canBlockDamageSource 无方向夹角校验，
//      Player.cpp:1580 TODO）。
//   2. Cubium 传原始 amount，vanilla 传 resolveBlockedDamage 结果（DamageReduction 数据驱动减伤后的被格挡值）。
//   3. Cubium 用 ceil(amount)，vanilla 用 floor(base+factor*f) + threshold<1.0 返 0 门控。
//   本测试用整数伤害场景（钻石剑 7.0）避开 ceil/floor 与 threshold 差异，专注验证"格挡消耗耐久"链路本身。
//
// 耐久读取（任务 #322 新增）：ItemStack.getComponent("minecraft:durability") 返回 { damage, maxDurability }。
//   - damage：已损耗耐久（ItemStack::getDamage 即 m_damage，0=满耐久）。
//   - maxDurability：最大耐久（盾牌 336，Items.cpp:2813 .maxDamage(336)）。
//   非可损坏物品返 undefined（对齐基岩语义）。本绑定让测试可直接断言耐久下降，此前所有耐久测试只能用
//   "数量不变"间接验证（盾牌数量恒 1 无法区分耐久是否消耗）。
//
// 防假通过设计（正反对照）：
//   - shield_block_consumes_durability：举盾格挡钻石剑近战 → 盾牌 damage 上升至 ~7。
//     断言 damage∈[5,8]（满冷却=7，容忍冷却未满略低；上界 8 防异常）。
//     若 damageShield 链路失效（hurtAndBreak 未接入/作用于副本未回写装备槽），damage=0 → FAIL。
//   - no_block_no_durability_loss：主手持盾但【不举盾】（不 useItem），同样被钻石剑近战 → 盾牌不格挡，
//     damage 保持 0，且 victim HP 下降（证明攻击确实命中，排除"没攻击到"假通过）。
//     若 damageShield 误对"未举盾"也消耗耐久（canBlockDamageSource 未查 isUsingItem），damage>0 → FAIL。
//   两测试交叉验证：举盾格挡消耗耐久 vs 不举盾不消耗 = isUsingItem 门控 + hurtAndBreak 链路对齐。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。两 SimulatedPlayer 互攻不涉及亡灵燃烧、
// 自然刷怪干扰。killAllEntities 清场防区域残留实体污染句柄。
//
// 时序：
//   - tick 0：victim 装盾主手 + useItem 举盾（测试1）/ 仅装盾不举盾（测试2）；attacker 装钻石剑主手。
//   - tick 20：attacker.attackEntity(victim)（留 20 tick 让两玩家 spawn 注册 + 就位 + 满攻击冷却，
//     钻石剑 ATTACK_SPEED=1.6，冷却周期 ~12.5 tick，20 tick 足够满冷却）。
//   - tick 30：断言盾牌耐久（攻击后 10 tick，确保 hurt→actuallyHurt→damageShield 链路完成）。
//
// 实体身份隔离：闭包持有 test.spawnSimulatedPlayer 返回的句柄，直接读主手槽盾牌耐久，
// 不按 type 区域查询（两玩家同 type 无法区分）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: Player.cpp:1595-1627（damageShield：ceil(amount) → hurtAndBreak 装备槽原件）
// Ref: LivingEntity.cpp:342-353（actuallyHurt canBlockDamageSource 命中 → damageShield(amount)）
// Ref: Player.cpp:1570-1593（canBlockDamageSource：isUsingItem + ShieldItem::isShield + BYPASSES_SHIELD）
// Ref: BlocksAttacks.java:114-125（hurtBlockingItem）+ :181-204（ItemDamageFunction DEFAULT floor(f)）
// Ref: Items.cpp:2813（盾牌 maxDamage(336)）
// Ref: MinecraftModuleFactory.cpp ItemStack.getComponent("minecraft:durability") 绑定（任务 #322）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑。两玩家距 2 格（Player::attack 攻击距离 ~3 格内）。
const VICTIM_POS = { x: 3, y: 2, z: 3 };
const ATTACKER_POS = { x: 3, y: 2, z: 5 };

// 构造盾牌 ItemStack（两份 @minecraft/server 类型分裂，as any 绕过，同 ShieldDisableTests 范式）。
function makeShield(): any {
    return new ItemStack("minecraft:shield", 1);
}

// 构造钻石剑 ItemStack（ATTACK_DAMAGE=7.0，无附魔，整数伤害避开 ceil/floor 差异）。
function makeDiamondSword(): any {
    return new ItemStack("minecraft:diamond_sword", 1);
}

// 受害者主手装盾并举盾（setActiveHand 进入持续格挡状态）。
// setItem(stack, 0, true)：slot=0 主手，selectSlot=true 同步选中。
// useItem(shield)：ShieldItem::onItemRightClick→setActiveHand(MainHand)，盾 getUseDuration=72000 持续极久。
function equipAndRaiseShield(victim: any): void {
    const shield = makeShield();
    victim.setItem(shield as unknown as Parameters<typeof victim.setItem>[0], 0, true);
    // 必须先 setItem 放盾主手再 useItem（onItemRightClick 读 getHeldItem(MainHand)，空手早返回）。
    (victim as any).useItem(shield as unknown as Parameters<typeof victim.setItem>[0]);
}

// 受害者主手装盾但不举盾（不 useItem，盾牌在主手槽但未进入格挡状态）。
function equipShieldOnly(victim: any): void {
    const shield = makeShield();
    victim.setItem(shield as unknown as Parameters<typeof victim.setItem>[0], 0, true);
}

// 攻击者主手装钻石剑。
function equipSword(attacker: any): void {
    attacker.setItem(makeDiamondSword() as unknown as Parameters<typeof attacker.setItem>[0], 0, true);
}

// 读取 victim 主手槽（slot 0）盾牌的已损耗耐久（damage）。无盾牌或非可损坏返 -1。
// container.getItem(0) 返回 owned 拷贝（拷贝时刻含当前 damage），getComponent("minecraft:durability")
// 读拷贝的 damage（任务 #322 绑定）。damageShield 作用于装备槽原件（getMutableEquipment），getItem 拷贝
// 反映更新后的耐久。
function readShieldDamage(player: any): number {
    const inv = player.getComponent("minecraft:inventory") as any;
    const stack = inv?.container?.getItem?.(0) as any;
    if (stack == null) {
        return -1;
    }
    const durability = (stack as any).getComponent?.("minecraft:durability") as
        | { damage?: number; maxDurability?: number }
        | undefined;
    if (durability == null) {
        return -1;
    }
    return durability.damage ?? -1;
}

// 读取玩家当前 HP（用于负向对照证明攻击命中）。
function readHp(player: any): number {
    const health = player?.getComponent?.("minecraft:health") as { currentValue?: number } | undefined;
    return health?.currentValue ?? NaN;
}

// 举盾格挡钻石剑近战 → 盾牌耐久下降 ~7 点（验证 damageShield → hurtAndBreak 链路）。
//
// victim 举盾（isUsingItem=true），attacker 钻石剑满冷却近战（totalDamage=7.0）。
// actuallyHurt canBlockDamageSource 命中（举盾+盾牌+BYPASSES_SHIELD 门控）→ damageShield(7.0)
// → ceil(7.0)=7 → hurtAndBreak 装备槽盾牌 7 点耐久。
//
// 判定：tick 20 attacker.attackEntity(victim)，pollUntilSucceed 轮询盾牌 damage∈[5,8]：
//   - 满冷却 damage=7（钻石剑 ATTACK_DAMAGE=7.0，ceil(7.0)=7）。
//   - 下界 5：证明格挡消耗了显著耐久（≥5），排除"damageShield 链路失效 damage=0"假通过。
//     容忍冷却未满略低（quadraticCooldown<1.0 时 damage<7，但仍应≥5 因 20 tick 足够满冷却）。
//   - 上界 8：防异常重伤（如暴击 1.5x 致 damage=ceil(10.5)=11，或多次格挡累加）。
//     暴击需 victim 落地/疾跑等条件，本场景 victim 站立不触发暴击，damage 应恰为 7。
//
// Ref: Player.cpp:1595-1627（damageShield ceil(amount)→hurtAndBreak）
// Ref: LivingEntity.cpp:352-353（canBlockDamageSource 命中→damageShield(amount)）
function shieldBlockConsumesDurability(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawnSimulatedPlayer(VICTIM_POS, "victim", 0 as any);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    equipAndRaiseShield(victim);
    equipSword(attacker);

    test.runAtTickTime(20, () => {
        (attacker as any).attackEntity(victim);
    });

    pollUntilSucceed(test, () => {
        const damage = readShieldDamage(victim);
        // damage∈[5,8] 证明格挡消耗了 ~7 耐久（钻石剑满冷却 7.0，ceil=7）。
        return damage >= 5 && damage <= 8;
    }, {
        startTick: 30,
        interval: 5,
        maxTick: 80,
        onTimeout: () => {
            const damage = readShieldDamage(victim);
            test.assert(false,
                `shield_block_consumes_durability: failed: shield damage=${damage} `
                + `(expected 5..8 = ceil(7.0) diamond_sword full-cooldown blocked damage; `
                + `if damage=0 damageShield→hurtAndBreak link broken or not writing to equipment slot; `
                + `if damage>8 abnormal [crit 1.5x or multi-block accumulated])`);
        },
    });
}

// 不举盾被钻石剑近战 → 盾牌不格挡，耐久保持 0，且 victim 掉血（负向对照，防假通过）。
//
// victim 主手持盾但【不 useItem 举盾】（isUsingItem=false），attacker 钻石剑满冷却近战。
// canBlockDamageSource 因 isUsingItem=false 返 false → 不进 damageShield 分支 → 盾牌耐久不变（damage=0）。
// 同时 victim 未格挡，actuallyHurt 走正常扣血 → HP 下降。
//
// 判定（tick 30，攻击后 10 tick）：
//   - 盾牌 damage===0（未格挡不消耗耐久）。
//   - victim HP < 20（攻击命中掉血，证明攻击确实发生，排除"没攻击到致 damage=0"假通过）。
// 交叉验证：damage=0 + HP 下降 = 盾牌耐久消耗确由"格挡"触发（isUsingItem 门控），非任何攻击误消耗。
//   - 若 canBlockDamageSource 漏 isUsingItem 门控（持盾即格挡）：未举盾也格挡，damage>0 → FAIL。
//   - 若攻击未命中（距离/朝向问题）：damage=0 但 HP=20（未掉血）→ HP<20 断言 FAIL，暴露测试环境问题。
//
// Ref: Player.cpp:1581-1583（canBlockDamageSource 首查 isUsingItem，未举盾返 false）
function noBlockNoDurabilityLoss(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawnSimulatedPlayer(VICTIM_POS, "victim", 0 as any);
    const attacker = test.spawnSimulatedPlayer(ATTACKER_POS, "attacker", 0 as any);

    // 仅装盾主手，不 useItem 举盾（盾牌在 slot 0 但未进入格挡状态）。
    equipShieldOnly(victim);
    equipSword(attacker);

    test.runAtTickTime(20, () => {
        (attacker as any).attackEntity(victim);
    });

    pollUntilSucceed(test, () => {
        const damage = readShieldDamage(victim);
        const hp = readHp(victim);
        // 盾牌耐久保持 0（未格挡）+ victim 掉血（攻击命中）。
        return damage === 0 && !Number.isNaN(hp) && hp < 20;
    }, {
        startTick: 30,
        interval: 5,
        maxTick: 80,
        onTimeout: () => {
            const damage = readShieldDamage(victim);
            const hp = readHp(victim);
            test.assert(false,
                `no_block_no_durability_loss: failed: shield damage=${damage}, victim hp=${hp} `
                + `(expected damage=0 [not blocking] & hp<20 [attack landed]; `
                + `if damage>0 canBlockDamageSource missing isUsingItem gate [blocking without raising shield]; `
                + `if hp=20 attack did not land [distance/facing issue])`);
        },
    });
}

export function registerShieldDurabilityTests(): void {
    GameTest.register("MobBehaviorTests", "shield_block_consumes_durability", shieldBlockConsumesDurability)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "no_block_no_durability_loss", noBlockNoDurabilityLoss)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);
}
