// 轻灵附魔（Feather Falling）摔落伤害减免行为类 GameTest。
//
// 验证 Cubium 轻灵附魔经 applyPotionDamageCalculations→getTotalArmorProtection→
// getDamageProtection（Fall+FALL flag = level*3）→getDamageAfterMagicAbsorb（damage*(1-epf/25)）
// 链路对齐 vanilla 1.21.11 摔落保护附魔减伤（ProtectionEnchantment#getDamageProtection）。
//
// 机制（对齐 vanilla ProtectionEnchantment.getDamageProtection + CombatRules.getDamageAfterMagicAbsorb）：
//   - FeatherFallingEnchantment 继承 ProtectionEnchantment(Type::Fall)，未重写 getDamageProtection。
//   - ProtectionEnchantment::getDamageProtection（ProtectionEnchantment.cpp:101-106）：Type::Fall 且
//     damageType 含 DamageFlags::FALL 位 → 返回 level*3（每级 EPF=3）。对齐 vanilla
//     ProtectionEnchantment#getDamageProtection（fall 源 → level*3）。
//   - EnchantmentHelper::getTotalArmorProtection（EnchantmentHelper.cpp:290）：遍历 4 护甲槽调
//     getProtectionFactor 累加 EPF，封顶 20（EPF_MAX）。轻灵仅能附于靴子（Feet 槽），单件 IV 级 EPF=4*3=12。
//   - applyPotionDamageCalculations（LivingEntity.cpp:552-561）：source.isFall() 设 DamageFlags::FALL
//     位，调 getTotalArmorProtection 算 EPF，再调 getDamageAfterMagicAbsorb 减伤。
//     门控：!source.isDamageAbsolute() && !source.is(DamageTypeTags::BYPASSES_ENCHANTMENTS)。
//   - getDamageAfterMagicAbsorb（CombatRules.cpp:52-67）：final = damage * (1 - clamp(epf,0,20)/25)。
//     单件轻灵 IV EPF=12 → 减伤 12/25=48% → final = damage * 0.52。
//
// 关键：摔落伤害 DamageType::Fall bypassesArmor=true（DamageSource.hpp:335 EnvironmentalDamage::
//   bypassesArmor 列表含 Fall），故 applyArmorCalculations 被跳过——钻石靴子的护甲值不参与摔落减伤，
//   仅有轻灵附魔 EPF 减伤。这与 vanilla 一致（fall 绕过护甲但受附魔保护减伤）。
//
// 数值（落差 10 格，Cubium 实测 fallDistance≈9，同 FallDamageTests，任务 #264 修复后）：
//   - 无附魔靴子：伤害 floor(9-3)=6（villager HP 20→14.0）。
//   - 轻灵 IV 钻石靴子：伤害 6.0 → EPF=12 → 6.0*(1-12/25)=6.0*0.52=3.12 → HP 20→16.88。
//   断言区间不重叠：无附魔 HP∈[12.5,14.5] vs 轻灵 IV HP∈[16.0,17.8]，交叉验证减伤生效。
//
// villager（HP 20）而非猪（HP 10）：HP 上限更高，减伤前后 HP 区分度更大（无附魔 14.0 vs 轻灵 16.88，
// 差 2.88；猪则 4.0 vs 6.88，差 2.88 但绝对值小容差紧）。villager 默认 SAFE_FALL_DISTANCE=3、
// FALL_DAMAGE_MULTIPLIER=1.0，与猪一致，fallDistance 累积机制相同。
// villager 是 Mob（被动不反击），可经 equippable.setEquipment("Feet", stack) 穿靴子（Cubium 善意扩展，
// 同 ArmorDamageReductionTests 范式）。
//
// 装备同步（关键）：equippable.setEquipment("Feet", stack) 写入装备数组。getTotalArmorProtection
// 直接读 ItemStack 的 EnchantmentContainer（getEnchantments(stack)→stack.getEnchantments().getAll()），
// 不依赖 detectEquipmentUpdates 应用附魔属性修饰符——故 setEquipment 后立即生效，无须等待 tick。
// 但仍留若干 tick 让实体落下（落体时间）。
//
// 附魔施加（受害者靴子）：脚本 ItemStack.addEnchantment({type:"minecraft:feather_falling", level:4})
//（Cubium 扩展，直接挂 ItemStack 类，对应基岩 ItemEnchantableComponent.addEnchantment，同
// FireProtectionBurningTimeTests 范式）。/enchant 仅对玩家生效，mob 须用 addEnchantment 直接给
// ItemStack 设附魔后经 setEquipment 穿戴。
//
// 防假通过设计（正反对照）：
//   - feather_falling_iv_reduces_fall_damage：穿轻灵 IV 钻石靴子 villager 摔 10 格，HP∈[16.0,17.8]
//     （掉 2.2-4）。若 getDamageProtection 未识别 FALL flag（EPF=0），伤害 6.0（HP=14.0<16.0）→ 超时 FAIL。
//     若 FeatherFallingEnchantment 未注册为 Type::Fall（getDamageProtection 走 default 返 0），同上 FAIL。
//     若 applyPotionDamageCalculations 未设 DamageFlags::FALL 位，同上 FAIL。
//   - no_feather_falling_takes_full_fall_damage：穿无附魔钻石靴子 villager 摔 10 格，HP∈[12.5,14.5]
//     （掉 5.5-7.5）。若摔落链路本身失效（HP=20），HP∈[12.5,14.5] 不满足→超时 FAIL，暴露轻灵测试假通过
//     风险（"高 HP"与轻灵减伤无关）。两测试交叉验证：无附魔掉≥5.5 vs 轻灵掉≤4 = 轻灵减伤正确。
//
// 注：fall_tower 1×1 玻璃管囚禁 villager 垂直自由落体。villager 体积（宽 0.6 高 1.95）略大于猪，
// 但 1×1 玻璃管内部空间 1.0×1.0 仍可容纳（玻璃管壁 glass 在 x=2/x=4、z=2/z=4，中心列 x=3,z=3 是 air）。
// villager spawn (3,11,3) 沿玻璃管落下到 (3,0,3) stone 顶面 y=1.0，落差 10 格。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_摔落.txt（摔落伤害公式 + 附魔减伤）
// Ref: ProtectionEnchantment.cpp:101-106（getDamageProtection：Fall+FALL → level*3）
// Ref: EnchantmentHelper.cpp:290-303（getTotalArmorProtection：遍历护甲槽累加 EPF 封顶 20）
// Ref: LivingEntity.cpp:552-561（applyPotionDamageCalculations：isFall 设 FALL 位 + getTotalArmorProtection）
// Ref: CombatRules.cpp:52-67（getDamageAfterMagicAbsorb：damage*(1-clamp(epf,0,20)/25)）
// Ref: DamageSource.hpp:335（EnvironmentalDamage::bypassesArmor 含 Fall——摔落绕过护甲仅受附魔减伤）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// fall_tower 结构尺寸 7×16×7（helper 相对坐标 x,z∈[0,6], y∈[0,15]）。
// 中心 (3,*,3) 为 1×1 垂直玻璃管落管：y=0 满铺 cobblestone 底（中心格覆盖为 stone），
// y=1..14 中心柱 air（下落通道），四周管壁 glass（y=1..15），y=15 顶部封顶。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const TOWER_FROM = { x: 0, y: 0, z: 0 };
const TOWER_VOLUME = { x: 7, y: 16, z: 7 };

const VILLAGER_TYPE = "villager";

// spawn 位置：villager 脚 (3,11,3)，落到 (3,0,3) stone 顶面 y=1.0，落差 10 格。
const SPAWN_POS = { x: 3, y: 11, z: 3 };
// 落点方块（普通方块，r=1.0 完整摔落伤害）。
const LANDING_POS = { x: 3, y: 0, z: 3 };

// 读取落地区域内 villager 的当前血量。区域限定排除并行测试污染。
function readVillagerHp(test: Test): number {
    const villagers = test.getDimension().getEntities({
        type: VILLAGER_TYPE,
        location: test.worldLocation(TOWER_FROM),
        volume: TOWER_VOLUME,
    });
    if (villagers.length === 0) {
        return -1;
    }
    const health = villagers[0].getComponent("minecraft:health");
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给 villager 穿轻灵 IV 钻石靴子（经 addEnchantment + setEquipment）。
//   - addEnchantment({type:"minecraft:feather_falling", level:4})：直接给 ItemStack 设附魔
//     （/enchant 仅对玩家生效，mob 须用此 API，同 FireProtectionBurningTimeTests 范式）。
//   - setEquipment("Feet", stack)：写入装备数组。getTotalArmorProtection 直接读 ItemStack 的
//     EnchantmentContainer（不依赖 detectEquipmentUpdates），故 setEquipment 后立即生效。
// 单件轻灵 IV EPF = 4*3 = 12，减伤 12/25 = 48%。
function equipFeatherFallingBoots(victim: any): void {
    const boots = makeItem("minecraft:diamond_boots");
    boots.addEnchantment({ type: "minecraft:feather_falling", level: 4 });
    victim.getComponent("minecraft:equippable").setEquipment("Feet", boots);
}

// 给 villager 穿无附魔钻石靴子（反向对照，验证摔落链路本身造成完整伤害，非"摔落失效 HP 高"假通过）。
function equipPlainBoots(victim: any): void {
    victim.getComponent("minecraft:equippable").setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 无附魔靴子 villager 从 10 格高处摔落到石头，承受完整摔落伤害（负向对照，防假通过）。
//
// 几何落差 10 格（spawn y=11 → stone 顶面 y=1）。Cubium 实测 fallDistance≈9（同 FallDamageTests，
// 任务 #264 修复 updateFallDistance 对齐 vanilla checkFallDamage 后；剩余 onGround 接触探测提前判定
// 偏差见任务 #273），伤害 floor(9-3)=6（villager HP 20→14.0）。
// 摔落绕过护甲（DamageType::Fall bypassesArmor=true），故钻石靴子护甲值不减伤，伤害纯摔落 6.0。
//
// 断言 HP ∈ [12.5, 14.5]（掉 5.5-7.5），容差覆盖 fallDistance 波动，且与轻灵 IV 测试
// HP ∈ [16.0, 17.8] 区间不重叠，确保交叉验证成立。
//   - 上界 HP≤14.5（掉≥5.5）证明承受了显著摔落伤害，排除"摔落链路失效 HP=20"假通过。
//   - 下界 HP≥12.5（掉≤7.5）排除异常重伤/摔死。
// 与 feather_falling_iv_reduces_fall_damage 交叉验证：无附魔掉≥5.5 vs 轻灵掉≤4 = 轻灵减伤正确。
function noFeatherFallingTakesFullFallDamage(test: Test): void {
    test.setBlockType("minecraft:stone", LANDING_POS);
    const villager = test.spawn(VILLAGER_TYPE, SPAWN_POS);
    equipPlainBoots(villager);

    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        // HP ∈ [12.5, 14.5] 证明承受了 ~6 完整摔落伤害（villager 20→14，Cubium fallDistance≈9）。
        // 摔落绕过护甲故钻石靴子不减伤。
        return hp >= 12.5 && hp <= 14.5;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `no-feather-falling villager should take full fall damage from 10 blocks (HP 20→~14), `
            + `but villager HP=${readVillagerHp(test)} (if HP=20 fall damage chain broken [causeFallDamage not triggered]; `
            + `if HP~14 correct; if HP<12.5 abnormal overkill)`),
    });
}

// 轻灵 IV 钻石靴子 villager 从 10 格高处摔落到石头，经附魔保护 EPF 减伤。
//
// 单件轻灵 IV（Feet 槽）EPF = 4*3 = 12，减伤 12/25 = 48%。
// Cubium 实测 fallDistance≈9（同无附魔测试），基础伤害 floor(9-3)=6.0，经 EPF 减伤后
// 6.0 * (1 - 12/25) = 6.0 * 0.52 = 3.12，villager HP 20→16.88。
//
// 断言 HP ∈ [16.0, 17.8]（掉 2.2-4），与无附魔测试 HP ∈ [12.5, 14.5] 区间不重叠，确保交叉验证成立。
//   - 下界 HP≥16.0（掉≤4）证明轻灵减伤生效：若 getDamageProtection 未识别 FALL flag（EPF=0），
//     伤害 6.0（HP=14.0<16.0）→ 超时 FAIL，暴露 ProtectionEnchantment::getDamageProtection 缺陷。
//     若 FeatherFallingEnchantment 未注册为 Type::Fall（getDamageProtection 走 default 返 0），同上 FAIL。
//     若 applyPotionDamageCalculations 未设 DamageFlags::FALL 位（getTotalArmorProtection 收到 flags=0），
//     getDamageProtection(Fall) 的 (damageType & FALL)=0 → 返 0 EPF → 同上 FAIL。
//   - 上界 HP≤17.8（掉≥2.2）排除"完全免疫掉 0/1"假通过（HP≥18.8 不满足）。
// 与 no_feather_falling_takes_full_fall_damage 交叉验证：带轻灵掉≤4 vs 无附魔掉≥5.5，
// HP 区间 [16.0,17.8] vs [12.5,14.5] 不重叠，确证轻灵 IV 经 EPF=12 减伤 48% 生效。
function featherFallingIvReducesFallDamage(test: Test): void {
    test.setBlockType("minecraft:stone", LANDING_POS);
    const villager = test.spawn(VILLAGER_TYPE, SPAWN_POS);
    equipFeatherFallingBoots(villager);

    pollUntilSucceed(test, () => {
        const hp = readVillagerHp(test);
        // HP ∈ [16.0, 17.8] 证明轻灵 IV 减伤至 ~3.12（villager 20→16.88，EPF=12 减伤 48%）。
        return hp >= 16.0 && hp <= 17.8;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 200,
        onTimeout: () => test.assert(false,
            `feather falling IV boots should reduce fall damage by 48% (EPF=12, HP 20→~16.9), `
            + `but villager HP=${readVillagerHp(test)} (if HP~14 feather falling EPF not applied `
            + `[getDamageProtection FALL flag / Type::Fall / DamageFlags::FALL defect]; `
            + `if HP~16.9 correct; if HP=20 fall damage chain broken)`),
    });
}

export function registerFeatherFallingTests(): void {
    GameTest.register("MobBehaviorTests", "no_feather_falling_takes_full_fall_damage", noFeatherFallingTakesFullFallDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(220);

    GameTest.register("MobBehaviorTests", "feather_falling_iv_reduces_fall_damage", featherFallingIvReducesFallDamage)
        .structureName("gametests:fall_tower")
        .maxTicks(220);
}
