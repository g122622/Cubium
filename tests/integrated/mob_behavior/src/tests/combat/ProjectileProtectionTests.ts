// 弹射物保护附魔（Projectile Protection）EPF 减伤对齐测试。
//
// 验证 Cubium 弹射物保护附魔经 applyPotionDamageCalculations→getTotalArmorProtection→
// getDamageProtection（Projectile+PROJECTILE flag = level*2）→getDamageAfterMagicAbsorb
// （damage*(1-epf/25)）链路对齐 vanilla 1.21.11 projectile_protection 减伤。
//
// 机制（对齐 vanilla projectile_protection.json + ProtectionEnchantment.getDamageProtection）：
//   - ProjectileProtectionEnchantment 继承 ProtectionEnchantment(Type::Projectile)，未重写 getDamageProtection。
//   - ProtectionEnchantment::getDamageProtection（ProtectionEnchantment.cpp:115-120）：Type::Projectile 且
//     damageType 含 DamageFlags::PROJECTILE 位 → 返回 level*2（每级 EPF=2）。对齐 vanilla
//     projectile_protection.json effects.damage_protection（linear base=2.0 per_level_above_first=2.0，
//     level 4 = 2+2*3 = 8.0）。
//   - EnchantmentHelper::getTotalArmorProtection（EnchantmentHelper.cpp:290）：遍历 4 护甲槽调
//     getProtectionFactor 累加 EPF，封顶 20（EPF_MAX）。4 件 PP IV EPF = 4*8 = 32 → 封顶 20。
//   - applyPotionDamageCalculations（LivingEntity.cpp:625-637）：source.isProjectile() 设
//     DamageFlags::PROJECTILE 位，调 getTotalArmorProtection 算 EPF，再调 getDamageAfterMagicAbsorb 减伤。
//     门控：!source.isDamageAbsolute() && !source.is(DamageTypeTags::BYPASSES_ENCHANTMENTS)。
//   - getDamageAfterMagicAbsorb（CombatRules.cpp:82-97）：final = damage * (1 - clamp(epf,0,20)/25)。
//     4 件 PP IV EPF=20（封顶）→ 减伤 20/25=80% → final = damage * 0.20。
//
// 关键：投射物伤害不绕过护甲（DamageType::Projectile bypassesArmor=false，与 Fall 不同），故 villager
//   穿护甲时既有护甲减伤又有 EPF 减伤。为纯隔离 EPF，本测试用**同护甲值对照**：4 件无附魔皮革套 vs
//   4 件 PP IV 皮革套——两者护甲减伤完全相同（皮革套护甲值 7：helmet1+chest3+legs2+boots1），HP 差异
//   纯粹来自 PP IV 的 EPF 减伤（80%）。皮革套而非钻石套：钻石套护甲 20 韧性 8 减伤过强（无附魔弓 6-10
//   经钻石护甲后仅 ~1.6-2.7，HP 差异小难分离 EPF）；皮革套护甲适中，留足伤害空间让 EPF 80% 减伤显效。
//
// 数值（满弓无附魔弓，箭矢基础伤害 6 + 暴击 0-4 = 6~10，speed=3.0 近距离衰减极小）：
//   - 4 件无附魔皮革套（护甲 7 韧性 0）：
//       effectiveArmor = clamp(7 - dmg/(2+0), 7*0.2=1.4, 20)；dmg=8 时 = clamp(3,1.4,20)=3
//       finalArmor = dmg*(1-3/25) = dmg*0.88 ≈ 5.3~8.8（dmg 6~10）
//       villager HP 20 → 11.2~14.7
//   - 4 件 PP IV 皮革套（护甲 7 韧性 0，EPF=20 封顶）：
//       finalArmor 同上 ≈ 5.3~8.8 → EPF 减伤 80% → final ≈ 1.1~1.8
//       villager HP 20 → 18.2~18.9
//   两区间 [11.2,14.7] vs [18.2,18.9] 完全不重叠，交叉验证 PP IV EPF 减伤生效。
//
// villager（HP 20，Mob 被动不反击）经 equippable.setEquipment 穿 4 件皮革套（Cubium 善意扩展，
//   同 ArmorDamageReductionTests 范式）。攻击者 SimulatedPlayer 满弓无附魔弓（同 BowArrowDamageTests 范式，
//   tick5 拉弓 tick25 释放满弓 20 tick，speed=3.0，箭矢 1 tick 命中 1 格外 villager）。
//
// 装备同步：护甲 ARMOR modifier 走 detectEquipmentUpdates（首 tick 后应用），EPF 直接读 ItemStack 的
//   EnchantmentContainer（不依赖 modifier），故 setEquipment 后须留 tick 让护甲 modifier 应用。两个测试
//   均穿皮革套（护甲 modifier 均应用），对照公平。
//
// 附魔施加（受害者 4 件皮革套）：stack.addEnchantment({type:"minecraft:projectile_protection", level:4})
//   （Cubium 扩展，同 FeatherFallingTests 范式）。/enchant 仅对玩家生效，mob 须用 addEnchantment。
//
// 防假通过设计（正反对照）：
//   - no_projectile_protection_takes_full_arrow_damage：穿无附魔皮革套 villager 中箭，HP∈[10,15]
//     （掉 5-10）。若弓箭链路失效（HP=20），HP∈[10,15] 不满足→超时 FAIL，暴露 PP IV 测试假通过风险
//     （"高 HP"与 PP IV 减伤无关）。若 getDamageProtection 未识别 PROJECTILE flag，PP IV 测试 HP 与本测试
//     相同（无减伤），两区间重叠→PP IV 测试 FAIL。
//   - projectile_protection_iv_reduces_arrow_damage：穿 PP IV 皮革套 villager 中箭，HP∈[17.5,19.5]
//     （掉 0.5-2.5）。若 getDamageProtection 未识别 PROJECTILE flag（EPF=0），伤害同无附魔（HP~13<17.5）→
//     超时 FAIL。若 ProjectileProtectionEnchantment 未注册为 Type::Projectile（getDamageProtection 走 default
//     返 0），同上 FAIL。若 applyPotionDamageCalculations 未设 DamageFlags::PROJECTILE 位，同上 FAIL。
//   两测试交叉验证：无附魔掉 5-10 vs PP IV 掉 0.5-2.5 = PP IV 经 EPF=20 减伤 80% 生效。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: projectile_protection.json（linear base=2.0 per_level_above_first=2.0，is_projectile 门控）
// Ref: ProtectionEnchantment.cpp:115-120（getDamageProtection：Projectile+PROJECTILE → level*2）
// Ref: EnchantmentHelper.cpp:290-303（getTotalArmorProtection：遍历护甲槽累加 EPF 封顶 20）
// Ref: LivingEntity.cpp:625-637（applyPotionDamageCalculations：isProjectile 设 PROJECTILE 位 + EPF 减伤）
// Ref: CombatRules.cpp:82-97（getDamageAfterMagicAbsorb：damage*(1-clamp(epf,0,20)/25)）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const BOW = "minecraft:bow";
const ARROW = "minecraft:arrow";
const VILLAGER_TYPE = "minecraft:villager";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 攻击者 (3,2,3) 默认 yaw=0 朝 +Z。受害者 (3,2,4) 距 1 格正前方（箭矢 1 tick 命中）。
const ARCHER_POS = { x: 3, y: 2, z: 3 };
const VICTIM_POS = { x: 3, y: 2, z: 4 };

// 读取 pit 区域内 villager 的当前血量。区域限定排除并行测试污染。
function readVillagerHp(test: Test): number {
    const villagers = test.getDimension().getEntities({
        type: VILLAGER_TYPE,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    if (villagers.length === 0) {
        return -1;
    }
    const health = villagers[0].getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给 villager 穿完整皮革套（4 件，护甲值 7 韧性 0）。经 equippable 组件 setEquipment
// （Cubium 善意扩展，mob 有 equippable，同 ArmorDamageReductionTests 范式）。
// withEnchant=true 时给 4 件均附 PP IV（每件 EPF=8，4 件 EPF=32 封顶 20，减伤 80%）。
// 4 件均附魔而非单件：单件 PP IV EPF=8 减伤仅 32%，区分度不足；4 件封顶 20 减伤 80% 区分度大。
function equipLeatherArmor(victim: any, withProjectileProtection: boolean): void {
    const eq = victim.getComponent("minecraft:equippable");
    const pieces: Array<{ slot: string; item: string }> = [
        { slot: "Head", item: "minecraft:leather_helmet" },
        { slot: "Chest", item: "minecraft:leather_chestplate" },
        { slot: "Legs", item: "minecraft:leather_leggings" },
        { slot: "Feet", item: "minecraft:leather_boots" },
    ];
    for (const piece of pieces) {
        const stack = makeItem(piece.item);
        if (withProjectileProtection) {
            stack.addEnchantment({ type: "minecraft:projectile_protection", level: 4 });
        }
        eq.setEquipment(piece.slot, stack);
    }
}

// 给攻击者主手弓 + 副手箭，tick 5 拉弓 tick 25 释放满弓（20 tick，speed=3.0）。
// 同 BowArrowDamageTests setupArcherAndVictim 范式，但攻击对象由调用方传入。
function setupArcher(test: Test): any {
    const player = test.spawnSimulatedPlayer(ARCHER_POS, "archer", 0 as any); // 0=Survival
    player.setItem(makeItem(BOW), 0, true); // 主手弓 slot 0
    const arrow = new ItemStack(ARROW, 5);
    player.setItem(arrow as unknown as Parameters<typeof player.setItem>[0], 40, false); // 副手 slot 40

    test.runAtTickTime(5, () => {
        (player as any).useItem(makeItem(BOW) as unknown as Parameters<typeof player.useItem>[0]);
    });
    test.runAtTickTime(25, () => {
        (player as any).stopUsingItem();
    });
    return player;
}

// 无弹射物保护皮革套 villager 承受完整箭矢伤害（正向对照，防 PP IV 测试假通过）。
//
// 满弓无附魔弓基础伤害 6 + 暴击 0-4 = 6~10。4 件无附魔皮革套护甲 7 韧性 0 减伤后 ~5.3~8.8，
// villager HP 20 → 11.2~14.7。断言 HP ∈ [10, 15]（掉 5-10）。
//   - 上界 HP≤15（掉≥5）证明承受了显著箭矢伤害，排除"弓箭链路失效 HP=20"假通过。
//   - 下界 HP≥10（掉≤10）排除异常重伤/致死。
// 与 projectile_protection_iv_reduces_arrow_damage 交叉验证：无附魔掉 5-10 vs PP IV 掉 0.5-2.5 =
//   PP IV EPF 减伤生效。
function noProjectileProtectionTakesFullArrowDamage(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);
    equipLeatherArmor(victim, false);
    setupArcher(test);

    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        return hp >= 10 && hp <= 15;
    }, {
        startTick: 30,
        interval: 5,
        maxTick: 100,
        onTimeout: () => test.assert(false,
            `no-projectile-protection villager should take full arrow dmg (leather armor, HP 20→~13), `
            + `but villager HP=${readVillagerHp(test)} (if HP=20 bow chain broken [arrow not fired/missed]; `
            + `if HP~13 correct; if HP<10 abnormal overkill/crit)`),
    });
}

// 弹射物保护 IV 皮革套 villager 经 EPF 减伤承受少量箭矢伤害。
//
// 满弓无附魔弓伤害 6~10，4 件 PP IV 皮革套：护甲减伤后 ~5.3~8.8 → EPF=20（4*8 封顶）减伤 80% →
// ~1.1~1.8，villager HP 20 → 18.2~18.9。断言 HP ∈ [17.5, 19.5]（掉 0.5-2.5）。
//   - 下界 HP≥17.5（掉≤2.5）证明 PP IV 减伤生效：若 getDamageProtection 未识别 PROJECTILE flag（EPF=0），
//     伤害同无附魔（HP~13<17.5）→ 超时 FAIL，暴露 ProtectionEnchantment::getDamageProtection PROJECTILE
//     分支缺陷。若 ProjectileProtectionEnchantment 未注册为 Type::Projectile，同上 FAIL。若
//     applyPotionDamageCalculations 未设 DamageFlags::PROJECTILE 位，同上 FAIL。
//   - 上界 HP≤19.5（掉≥0.5）排除"箭矢未命中 HP=20"假通过（HP≥19.5 不满足，需掉≥0.5）。
// 与 no_projectile_protection_takes_full_arrow_damage 交叉验证：PP IV 掉 0.5-2.5 vs 无附魔掉 5-10，
//   HP 区间 [17.5,19.5] vs [10,15] 不重叠，确证 PP IV 经 EPF=20 减伤 80% 生效。
function projectileProtectionIvReducesArrowDamage(test: Test): void {
    (test as any).killAllEntities();
    const victim = test.spawn(VILLAGER_TYPE, VICTIM_POS);
    equipLeatherArmor(victim, true);
    setupArcher(test);

    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        return hp >= 17.5 && hp <= 19.5;
    }, {
        startTick: 30,
        interval: 5,
        maxTick: 100,
        onTimeout: () => test.assert(false,
            `projectile protection IV villager should take 80% reduced arrow dmg (EPF=20, HP 20→~18.5), `
            + `but villager HP=${readVillagerHp(test)} (if HP~13 PP EPF not applied `
            + `[getDamageProtection PROJECTILE flag / Type::Projectile / DamageFlags::PROJECTILE defect]; `
            + `if HP~18.5 correct; if HP=20 bow chain broken [arrow missed])`),
    });
}

export function registerProjectileProtectionTests(): void {
    GameTest.register("MobBehaviorTests", "no_projectile_protection_takes_full_arrow_damage", noProjectileProtectionTakesFullArrowDamage)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "projectile_protection_iv_reduces_arrow_damage", projectileProtectionIvReducesArrowDamage)
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
}
