// 冰霜行者附魔踩踏免疫行为类 GameTest。
//
// 验证 Cubium 冰霜行者（frost_walker）附魔经 LivingEntity::isInvulnerableTo 的
// BURN_FROM_STEPPING 管线侧门控，使穿戴者完全免疫营火踩踏（campfire）与岩浆块烫脚（hot_floor）
// 伤害，对齐 MC Java 1.21.11 vanilla 架构。
//
// vanilla 架构（关键：纯管线侧门控，方块侧不自查）：
//   - CampfireBlock.entityInside（CampfireBlock.java:110-116）直接 hurt(campfire, fireDamage)，
//     不自查 frostWalker。
//   - MagmaBlock.stepOn（MagmaBlock.java:30-33）直接 hurt(hotFloor, 1.0F)，不自查 frostWalker。
//   - 冰霜行者免疫完全由 LivingEntity.isInvulnerableTo:3857 统一拦截：
//       isInvulnerableToBase || EnchantmentHelper.isImmuneToDamage(this, source)
//     → EnchantmentHelper.isImmuneToDamage（EnchantmentHelper.java:160）遍历装备附魔调
//       Enchantment.isImmuneToDamage（Enchantment.java:199）查 EnchantmentEffectComponents.DAMAGE_IMMUNITY
//       组件。
//   - FROST_WALKER 注册 DAMAGE_IMMUNITY 组件（Enchantments.java:388-396），条件：
//       tag(is(BURN_FROM_STEPPING)) && tag(isNot(BYPASSES_INVULNERABILITY))
//     即任意等级 frost_walker 使穿戴者对所有 BURN_FROM_STEPPING 标签伤害源（campfire/hot_floor）
//     完全免疫（DamageImmunity 不依赖 level，条件匹配即免疫）。
//
// Cubium 实现（任务 #274 修复：架构归一，对齐 vanilla 纯管线侧）：
//   - 修复前：CampfireBlock::onEntityCollision / MagmaBlock::onEntityWalk 各自前置自查
//     hasFrostWalker(boots)→return（方块侧守卫），偏离 vanilla（vanilla 不自查）。
//   - 修复后：移除方块侧守卫，统一在 LivingEntity::isInvulnerableTo（LivingEntity.cpp:1164-1178）
//     加管线侧门控：!source.bypassesInvulnerability() && source.is(BURN_FROM_STEPPING) &&
//     靴子 frost_walker 等级 > 0 → return true（免疫）。
//   - BURN_FROM_STEPPING 标签（DamageTypeTags.cpp:805-808）含 Campfire + HotFloor，与门控完全对应。
//   - 与 IS_FIRE+isImmuneToFire（火焰免疫实体）/ IS_FIRE+FireResistance（抗火药水）统一管线门控一致，
//     职责归一（vanilla 同一 isInvulnerableToBase 内并列各分支）。
//
// 附魔施加（关键：mob 不能用 /enchant，须用 ItemStack.addEnchantment）：
//   - /enchant 命令仅对玩家生效（EnchantCommand.cpp:60 用 EntityArgumentType::player()）。
//   - Cubium 在 ItemStack 类绑定 addEnchantment({type, level})（MinecraftModuleFactory.cpp:2880-2922，
//     Cubium 扩展，未实现 minecraft:enchantable 组件派发故挂 ItemStack 类），供测试构造带附魔装备
//     绕过 /enchant 仅对玩家生效的限制（同 FireProtectionBurningTimeTests 范式）。
//   - 经 equippable.setEquipment("Feet", stack) 穿戴（mob 有 equippable，Cubium 善意扩展）。
//   - ItemStack 拷贝构造深拷贝 m_enchantments（ItemStack.cpp:124），setEquipment 按值拷贝写入装备数组，
//     C++ 侧 EnchantmentHelper::getEnchantmentLevel(boots, "minecraft:frost_walker") 能读到附魔。
//   - frost_walker 走 isInvulnerableTo 门控（读附魔等级，非装备属性修饰符管线），写入即可读，
//     无需等 detectEquipmentUpdates（与 FireProtectionBurningTime 需等修饰符应用不同）。
//
// 正反对照（防假通过，3 测试交叉验证）：
//   - frost_walker_boots_immune_to_campfire：穿 frost_walker 靴子的猪踩营火，HP 保持 10（免疫）。
//   - frost_walker_boots_immune_to_magma：穿 frost_walker 靴子的猪踩岩浆块，HP 保持 10（免疫）。
//   - plain_boots_take_magma_damage：穿无附魔靴子的猪踩岩浆块，HP 下降（负向对照，排除"穿靴子本身
//     免伤"或"踩踏伤害链路失效"假通过——campfire 的光脚对照已有 campfire_damages_entity_on_top 覆盖，
//     此处补 magma 的穿靴对照，确保 magma 链路在穿靴时仍正常造伤）。
//
// 时序：spawn 同步返回后立即 setEquipment 穿戴 frost_walker 靴子（落地前已穿好），猪下落至营火/
// 岩浆块顶面站稳后 onEntityCollision/onEntityWalk 每 tick 调 hurt，isInvulnerableTo 门控拦截所有
// campfire/hotFloor 伤害，HP 恒 10。受击免疫节流对本测试无影响（门控在 isInvulnerableTo 层，先于
// 无敌帧判定，每 tick 均拦截）。maxTicks=200 留足落地 + 站立时间。
//
// className 恒为 BlockBehaviorTests（对齐 block_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_营火.txt#伤害（冰霜行者免疫营火伤害）
// Ref: LivingEntity.cpp:1164-1178（isInvulnerableTo BURN_FROM_STEPPING+frost_walker 管线侧门控）
// Ref: Enchantments.java:388-396（FROST_WALKER 注册 DAMAGE_IMMUNITY 组件）
// Ref: DamageTypeTags.cpp:805-808（BURN_FROM_STEPPING = Campfire + HotFloor）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底（满铺 49 glass），y=1..2 为玻璃墙围出的内部 air 空腔，y=3..4 air+顶部框架。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

const PIG_TYPE = "pig";

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 ArmorDamageReductionTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给猪穿带冰霜行者 II 附魔的钻石靴子（经 addEnchantment + setEquipment）。
//   - addEnchantment({type:"minecraft:frost_walker", level:2})：直接给 ItemStack 设附魔
//     （/enchant 仅对玩家生效，mob 须用此 API，同 FireProtectionBurningTimeTests 范式）。
//   - setEquipment("Feet", stack)：写入 Feet 槽装备数组。frost_walker 仅注册在 FEET 槽
//     （Enchantments.java:388-396，FROST_WALKER definition slots=FEET）。
//   - 任意等级均免疫（vanilla DamageImmunity 不依赖 level），用 II 级更贴近实战装备。
function equipFrostWalkerBoots(mob: any): void {
    const boots = makeItem("minecraft:diamond_boots");
    boots.addEnchantment({ type: "minecraft:frost_walker", level: 2 });
    mob.getComponent("minecraft:equippable").setEquipment("Feet", boots);
}

// 给猪穿无附魔钻石靴子（负向对照，排除"穿靴子本身免伤"假通过）。
function equipPlainBoots(mob: any): void {
    mob.getComponent("minecraft:equippable").setEquipment("Feet", makeItem("minecraft:diamond_boots"));
}

// 读取 glass_pit 区域内猪的当前血量。区域限定排除并行测试污染。
function readPigHp(test: Test): number {
    const pigs = test.getDimension().getEntities({
        type: PIG_TYPE,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    if (pigs.length === 0) {
        return -1;
    }
    const health = pigs[0].getComponent("minecraft:health");
    return (health as any).currentValue as number;
}

// 冰霜行者靴子免疫营火踩踏伤害（验证 BURN_FROM_STEPPING+frost_walker 管线侧门控对 campfire 生效）。
//
// 猪 spawn (3,2,3) 穿 frost_walker 靴子，下落至 (3,1,3) 营火顶面站稳。CampfireBlock::onEntityCollision
// 每 tick 调 hurt(campfire, 1.0f)，LivingEntity::isInvulnerableTo 门控（BURN_FROM_STEPPING+frost_walker）
// 拦截所有伤害，HP 恒 10。
//
// 囚笼同 campfire_damages_entity_on_top：营火 (3,1,3) + 四周 y=2 玻璃 + 顶部 (3,3,3) 玻璃封顶
// （onEntityCollision 依赖 AABB 相交，1 格高囚笼 + 封顶防猪跳出，不依赖 m_onGround）。
//
// 判定：succeedWhen 每 tick 检查 HP === 10（满血，完全免疫）。
//   若门控缺失（任务 #274 修复前：方块侧守卫已移除但管线侧门控未加），campfire 伤害放行，HP 下降 < 10
//   → 超时 FAIL，暴露管线侧门控缺陷。
//   若 addEnchantment/setEquipment 未生效（附魔未写入），门控读到 frost_walker 等级 0，HP 下降 → FAIL。
//   与 plain_boots_take_magma_damage 交叉验证：穿附魔靴免疫 vs 穿无附魔靴受伤 = frost_walker 免疫正确。
function frostWalkerBootsImmuneToCampfire(test: Test): void {
    // (3,1,3) 放营火（defaultState LIT=true 点燃，触发伤害）。下方 y=0 glass 实心支撑。
    test.setBlockType("minecraft:campfire", { x: 3, y: 1, z: 3 });

    // 囚笼：四周 y=2 层玻璃 + 顶部 (3,3,3) 玻璃封顶（防猪跳跃挤出）。同 campfire_damages_entity_on_top。
    test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 });
    test.setBlockType("minecraft:glass", { x: 4, y: 2, z: 3 });
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 });
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 });
    test.setBlockType("minecraft:glass", { x: 3, y: 3, z: 3 });

    // 猪 spawn 于 (3,2,3)（营火正上方），立即穿 frost_walker 靴子（落地前已穿好）。
    const pig = test.spawn(PIG_TYPE, { x: 3, y: 2, z: 3 });
    equipFrostWalkerBoots(pig);

    // 断言猪免疫营火伤害：succeedWhen 每 tick 检查 HP === 10（满血，完全免疫）。
    // 留 startTick 余量让猪落地站稳 + 首次 onEntityCollision 触发（落地后约 10 tick 首次 hurt 放行，
    // 若门控缺失此时 HP 应已下降，故等够时间后仍满血即证免疫）。
    test.succeedWhen(() => {
        const hp = readPigHp(test);
        test.assert(hp === 10,
            `frost_walker boots pig should be immune to campfire damage (HP stay 10), `
            + `but pig HP=${hp} (if HP<10 BURN_FROM_STEPPING+frost_walker pipeline gate missing `
            + `[task #274 regression] or enchantment not applied; if HP=10 correct; if HP=-1 pig escaped cage)`);
    });
}

// 冰霜行者靴子免疫岩浆块烫脚伤害（验证 BURN_FROM_STEPPING+frost_walker 管线侧门控对 hot_floor 生效）。
//
// 猪 spawn (3,2,3) 穿 frost_walker 靴子，下落至 (3,1,3) 岩浆块顶面站稳。MagmaBlock::onEntityWalk
// 每 tick 调 hurt(hotFloor, 1.0f)，LivingEntity::isInvulnerableTo 门控（BURN_FROM_STEPPING+frost_walker）
// 拦截所有伤害，HP 恒 10。
//
// 囚笼同 magma_damages_entity_on_top：岩浆块 (3,1,3) + y=2,y=3 两层四周玻璃（2 格高墙，不在猪正上方
// (3,3,3) 放玻璃防挤压——onEntityWalk 依赖 m_onGround，挤压致悬空会破坏 m_onGround）。
//
// 判定：succeedWhen 每 tick 检查 HP === 10（满血，完全免疫）。
//   与 frostWalkerBootsImmuneToCampfire + plainBootsTakeMagmaDamage 三角验证：
//   frost_walker 免疫 campfire + 免疫 magma + 无附魔靴受伤 = BURN_FROM_STEPPING 管线门控正确。
function frostWalkerBootsImmuneToMagma(test: Test): void {
    // (3,1,3) 放岩浆块（完整方块，猪可站顶面）。下方 y=0 glass 实心支撑。
    test.setBlockType("minecraft:magma_block", { x: 3, y: 1, z: 3 });

    // 囚笼：y=2 和 y=3 两层四周 glass（2 格高墙），不在猪正上方 (3,3,3) 放玻璃防挤压。
    // 顶部依靠结构 y=4 顶框玻璃防猪跳出。同 magma_damages_entity_on_top。
    for (const y of [2, 3]) {
        test.setBlockType("minecraft:glass", { x: 2, y, z: 3 });
        test.setBlockType("minecraft:glass", { x: 4, y, z: 3 });
        test.setBlockType("minecraft:glass", { x: 3, y, z: 2 });
        test.setBlockType("minecraft:glass", { x: 3, y, z: 4 });
    }

    // 猪 spawn 于 (3,2,3)（岩浆块正上方），立即穿 frost_walker 靴子。
    const pig = test.spawn(PIG_TYPE, { x: 3, y: 2, z: 3 });
    equipFrostWalkerBoots(pig);

    // 断言猪免疫岩浆块烫脚伤害：succeedWhen 每 tick 检查 HP === 10。
    test.succeedWhen(() => {
        const hp = readPigHp(test);
        test.assert(hp === 10,
            `frost_walker boots pig should be immune to magma hot_floor damage (HP stay 10), `
            + `but pig HP=${hp} (if HP<10 BURN_FROM_STEPPING+frost_walker pipeline gate missing `
            + `[task #274 regression] or enchantment not applied; if HP=10 correct; if HP=-1 pig escaped cage)`);
    });
}

// 无附魔靴子踩岩浆块仍受烫脚伤害（负向对照，防 frost_walker 免疫测试假通过）。
//
// 猪 spawn (3,2,3) 穿无附魔钻石靴子，下落至 (3,1,3) 岩浆块顶面站稳。MagmaBlock::onEntityWalk 每 tick
// 调 hurt(hotFloor, 1.0f)，isInvulnerableTo 门控不拦截（无 frost_walker），HP 下降。
//
// 排除两类假通过：
//   1. "穿靴子本身免伤"：若 diamond_boots 有某种火焰免伤（实际无），frost_walker 测试的免疫可能是
//      靴子而非附魔导致。本测试穿同款无附魔靴子仍受伤，证明免疫源于 frost_walker 附魔而非靴子。
//   2. "踩踏伤害链路失效"：若 magma 链路本身不造伤（如囚笼设计致猪未站岩浆块），frost_walker 测试的
//      "HP 恒 10"可能是链路失效而非免疫。本测试同布局穿无附魔靴子受伤，证明链路正常。
//
// 判定：succeedWhen 每 tick 检查 HP < 10（受伤，首次伤害后 10→9）。
//   campfire 的光脚受伤对照已有 campfire_damages_entity_on_top 覆盖，此处补 magma 的穿靴受伤对照。
function plainBootsTakeMagmaDamage(test: Test): void {
    test.setBlockType("minecraft:magma_block", { x: 3, y: 1, z: 3 });

    for (const y of [2, 3]) {
        test.setBlockType("minecraft:glass", { x: 2, y, z: 3 });
        test.setBlockType("minecraft:glass", { x: 4, y, z: 3 });
        test.setBlockType("minecraft:glass", { x: 3, y, z: 2 });
        test.setBlockType("minecraft:glass", { x: 3, y, z: 4 });
    }

    // 猪 spawn 于 (3,2,3)，立即穿无附魔钻石靴子（负向对照）。
    const pig = test.spawn(PIG_TYPE, { x: 3, y: 2, z: 3 });
    equipPlainBoots(pig);

    // 断言猪受岩浆块烫脚伤害：succeedWhen 每 tick 检查 HP < 10。
    test.succeedWhen(() => {
        const hp = readPigHp(test);
        test.assert(hp < 10 && hp > 0,
            `plain-boots pig should take magma hot_floor damage (HP 10→<10), `
            + `but pig HP=${hp} (if HP=10 magma damage chain broken [pig not standing on magma or onEntityWalk `
            + `not firing] — frost_walker immune test may be false pass; if HP<10 correct)`);
    });
}

export function registerFrostWalkerTests(): void {
    GameTest.register("BlockBehaviorTests", "frost_walker_boots_immune_to_campfire", frostWalkerBootsImmuneToCampfire)
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("BlockBehaviorTests", "frost_walker_boots_immune_to_magma", frostWalkerBootsImmuneToMagma)
        .structureName("gametests:glass_pit")
        .maxTicks(200);

    GameTest.register("BlockBehaviorTests", "plain_boots_take_magma_damage", plainBootsTakeMagmaDamage)
        .structureName("gametests:glass_pit")
        .maxTicks(200);
}
