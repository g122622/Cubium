// 荆棘附魔反伤行为类 GameTest。
//
// 验证 Cubium 荆棘附魔（THORNS）经受害者护甲在受击时对攻击者反伤（对齐 vanilla 1.21.11
// THORNS POST_ATTACK VICTIM→ATTACKER，DamageEntity(constant 1.0, 5.0)）。
//
// vanilla 1.21.11 THORNS（Enchantments.java:324-347）：
//   POST_ATTACK(VICTIM→ATTACKER)，AllOf.entityEffects(DamageEntity, ChangeItemDamage)，
//   概率门控 LootItemRandomChanceCondition randomChance(perLevel 0.15)：
//     - DamageEntity(constant 1.0F, constant 5.0F, THORNS)：触发时对攻击者造成 [1.0, 5.0) 随机荆棘伤害
//       （Mth.randomBetween，与等级无关，仅触发概率随等级线性增长：I=15% II=30% III=45%）。
//     - ChangeItemDamage(constant 2.0F)：触发时使触发荆棘的那件护甲扣 2 耐久。
//   vanilla 触发点仅在攻击者侧（Player.attack→itemAttackInteraction→doPostAttackEffectsWithItemSource，
//   或 LivingEntity.stabAttack→doPostAttackEffects），hurtServer/actuallyHurt 内无 doPostAttack。
//
// Cubium 链路（受害者侧 actuallyHurt 统一入口，等价简化）：
//   zombie.attackEntityAsMob(player)（MobEntity.cpp:730 target.hurt(mobAttack(zombie))）
//     → player.hurt → LivingEntity::actuallyHurt 步骤 9（LivingEntity.cpp:446，
//       !source.isThornsDamage() && trueSource != nullptr && trueSource != this 门控）
//     → EnchantmentHelper::applyThornsEnchantments(player, zombie)（EnchantmentHelper.cpp:359）
//     → 按 [Head,Chest,Legs,Feet] 遍历 player 护甲，胸甲 thorns III 命中
//     → ThornsEnchantment::onUserHurt(player, zombie, chest, Chest, 3)（ThornsEnchantment.cpp:53）
//     → shouldTrigger(3, rng)：rng seed = player.id() ^ player.ticksExisted()，概率 3*0.15=0.45
//     → 触发后：zombie.hurt(thorns(player), getThornsDamage(rng) ∈ [1.0,5.0))（反伤攻击者）
//                + LivingEntity::hurtAndBreak(chest, 2, player, Chest)（胸甲扣 2 耐久）
//
// 此前缺陷（任务 #276 修复）：
//   1. getThornsDamage 用老版本公式 level>10?level-10:1+nextInt(4)（1-4 整数 + 多余 level>10 分支），
//      偏离 vanilla [1.0,5.0) 随机浮点。修复：改 random.nextFloat(1.0f, 5.0f)。
//   2. onUserHurt 耐久消耗未接入（注释承认未处理但调用方无装备引用）。修复：onUserHurt 签名加
//      ItemStack& enchantedItem + EquipmentSlot slot，触发时调 hurtAndBreak 扣 2 耐久。
//   3. Player::attack 末尾（Player.cpp:2772）与 actuallyHurt 步骤 9 双重调用 applyThornsEnchantments，
//      致玩家近战攻击时荆棘双重触发（反伤+耐久翻倍）。修复：删除 Player.cpp:2772 重复调用，
//      保留 actuallyHurt 统一入口对齐 vanilla 单次触发。
//
// 测试设计（正向 + 负向对照）：
//   - thorns_iii_reflects_damage_to_attacker：穿 thorns III 胸甲的 Survival 玩家被 zombie 近战攻击，
//     荆棘 III 45% 概率反伤 zombie [1.0,5.0)。zombie 主动追击玩家（NearestAttackableTargetGoal 选
//     Survival 玩家），MeleeAttackGoal 贴身后 attackEntityAsMob 攻击玩家触发荆棘。zombie HP 20，
//     多次反伤累积 HP 下降至 <20 即证明荆棘反伤接通。
//   - no_thorns_no_reflection：穿无附魔胸甲的 Survival 玩家被 zombie 攻击，无荆棘护甲不触发反伤，
//     zombie HP 保持 20（全程不掉血）。防 thorns_iii 测试假通过（如 zombie 因其他原因掉血）。
//     两测试交叉验证：穿荆棘 zombie 掉血 + 无荆棘 zombie 不掉血 = 荆棘反伤正确。
//
// 概率与窗口（thorns_iii_reflects_damage_to_attacker）：
//   thorns III 触发概率 45%。zombie MeleeAttackGoal 攻击间隔约 20 tick（攻击冷却），600 tick 窗口
//   约 30 次攻击机会（zombie 需先接近玩家，实际有效攻击约 15-20 次），期望触发 7-9 次，累积反伤
//   约 14-36（[1,5)*7~9）。zombie HP 20 应在窗口内显著下降（多次反伤致死亦可能）。
//   断言 zombie HP < 20（任一次反伤发生即满足，宽松下界避免概率尾巴致 flaky）。
//
// 玩家存活约束：zombie 攻击伤害 3（普通难度）+ 胸甲减伤，玩家 20 HP。600 tick 内 zombie 有效攻击
//   约 15-20 次，玩家承受约 15-20 次攻击（部分被胸甲减伤）。玩家可能掉血较多但荆棘反伤会优先击杀
//   zombie（zombie 20 HP，反伤累积 14-36 致死）。玩家死亡不影哐 zombie HP 断言（zombie 已被反伤掉血）。
//   若玩家先死，zombie 失去目标停止攻击，但此时 zombie HP 应已 <20（反伤已发生）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ night batch。
//   - night batch：zombie 亡灵白天燃烧掉血干扰 HP 断言（燃烧伤害会让 zombie HP 下降与荆棘反伤混淆），
//     night 避开阳光燃烧。creeper_pit 开放坑无顶，night 无阳光 zombie 不燃。
//   - creeper_pit 无围墙，zombie 寻路通畅接近玩家，checkSight 视线无阻挡。
//   - killAllEntities 清场：night batch doMobSpawning 可能自然刷怪干扰，清场后仅本测试 spawn 的
//     zombie + 玩家，区域限定查询排除残余污染。
//
// 实体身份隔离：闭包持有 test.spawn 返回的 zombie 句柄读 getComponent("minecraft:health").currentValue，
// 不按 type 区域查询（虽然也用了区域限定查 zombie 兜底，但优先用句柄）。
//
// 装备同步管线：player.setEquipment("Chest", chest) 写入装备数组 → 下个 tick detectEquipmentUpdates。
// 荆棘不依赖属性修饰符（thorns 在 actuallyHurt 内直接查 getEnchantmentLevel，不经属性管线），故
// setEquipment 后无需等修饰符应用，但留 20 tick 让装备同步 + zombie 选目标稳定。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑（helper y=1）。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 玩家与 zombie 对角放置，距 ~5.7 格。zombie 主动追击玩家（NearestAttackableTargetGoal 选 Survival
// 玩家），MeleeAttackGoal 接近后 attackEntityAsMob 攻击玩家触发荆棘反伤。
const PLAYER_POS = { x: 5, y: 2, z: 5 };
const ZOMBIE_POS = { x: 1, y: 2, z: 1 };

// 读取实体当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。实体死亡移除后返回 -1。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 构造物品 ItemStack（Cubium 的 @minecraft/server 与 server-gametest 依赖的 @minecraft/server
// 是两个独立包实例，ItemStack 类型不兼容，用 as any 绕过，同 MeleeEnchantDamageTests 范式）。
function makeItem(itemId: string): any {
    return new ItemStack(itemId, 1);
}

// 给玩家穿钻石胸甲并施加荆棘 III（经 addEnchantment + setEquipment）。
//   - addEnchantment({type:"minecraft:thorns", level:3})：直接给 ItemStack 设附魔（/enchant 仅对玩家
//     生效但需 permLevel，mob 须用此 API；SimulatedPlayer 亦可用 setEquipment 直接穿戴已附魔物品）。
//   - setEquipment("Chest", stack)：写入装备数组。荆棘在 actuallyHurt 内直接查 getEnchantmentLevel，
//     不经属性管线，故 setEquipment 后无需等修饰符应用。
function equipThornsChest(player: any): void {
    const chest = makeItem("minecraft:diamond_chestplate");
    chest.addEnchantment({ type: "minecraft:thorns", level: 3 });
    player.getComponent("minecraft:equippable").setEquipment("Chest", chest);
}

// 给玩家穿无附魔钻石胸甲（反向对照，验证无荆棘护甲不触发反伤）。
function equipPlainChest(player: any): void {
    player.getComponent("minecraft:equippable").setEquipment("Chest", makeItem("minecraft:diamond_chestplate"));
}

// 穿荆棘 III 胸甲的玩家被僵尸近战攻击后，荆棘反伤僵尸致其掉血（验证 THORNS 反伤端到端链路接通）。
//
// 玩家（Survival）穿 thorns III 钻石胸甲，僵尸主动追击玩家近战攻击 → 玩家 hurt → actuallyHurt 步骤 9
// → applyThornsEnchantments → onUserHurt（thorns III 45% 概率）→ 僵尸 hurt(thorns, [1,5))。
// 僵尸 HP 20，多次反伤累积 HP 下降至 <20 即证明荆棘反伤接通。
//
// 判定：pollUntilSucceed 轮询僵尸 HP < 20（含 -1=被反伤致死移除）。
//   若荆棘反伤链路断裂（actuallyHurt 缺荆棘分支 / onUserHurt 不调 zombie.hurt / getThornsDamage 返 0），
//   僵尸 HP 恒 20 → 超时 FAIL。
//   thorns III 45% 概率，600 tick 窗口期望触发 7-9 次，HP 显著下降。断言 <20 宽松下界避免概率尾巴 flaky。
//
// night batch + killAllEntities：隔离白天燃烧（zombie 亡灵白天燃烧掉血干扰）+ 自然刷怪污染。
// Ref: ThornsEnchantment.cpp:53（onUserHurt 反伤 attacker + 扣耐久）
// Ref: LivingEntity.cpp:446（actuallyHurt 步骤 9 荆棘分支）
// Ref: MobEntity.cpp:730（attackEntityAsMob → target.hurt 触发受害者 actuallyHurt）
function thornsIiiReflectsDamageToAttacker(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", ZOMBIE_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "victim", 0 as any); // 0=Survival

    equipThornsChest(player);

    // 留 20 tick 让装备同步 + 僵尸选目标稳定，然后轮询僵尸 HP。
    // 僵尸需先接近玩家（~5.7 格，速度 ~0.25，约 tick 40 接近），首击约 tick 60+，反伤累积需时间。
    pollUntilSucceed(test, () => {
        const hp = readHp(zombie);
        // 僵尸被反伤致死移除（-1）或掉血（<20）均判定反伤发生。
        return hp < 20;
    }, {
        startTick: 20,
        interval: 10,
        maxTick: 600,
        onTimeout: () => test.assert(false,
            `thorns III chest should reflect damage to attacking zombie (zombie HP should drop <20), `
            + `but zombie HP=${readHp(zombie)} (thorns reflection not wired — `
            + `if HP==20 actuallyHurt step 9 thorns branch missing or onUserHurt not hurting attacker; `
            + `if zombie never attacked player, zombie may not have reached melee range)`),
    });
}

// 无荆棘胸甲玩家被僵尸攻击后，僵尸不掉血（负向对照，防 thornsIiiReflectsDamageToAttacker 假通过）。
//
// 玩家（Survival）穿无附魔钻石胸甲，僵尸近战攻击玩家 → applyThornsEnchantments 遍历护甲无荆棘 →
// 不触发 onUserHurt → 僵尸无反伤（HP 恒 20）。
//
// 判定：pollUntilSucceed 轮询"玩家 HP<20（被 zombie 攻击掉血，证明攻击链路通）且 zombie HP===20（无反伤）"。
//   - 若 applyThornsEnchantments 误对无附魔护甲触发（getEnchantmentLevel 判定失效），zombie HP<20 →
//     condition 不满足 → 超时 FAIL，从而暴露 thornsIii 测试假通过。
//   - 若 zombie 根本没攻击玩家（玩家 HP==20），condition 不满足 → 超时 FAIL（避免"zombie 没掉血但也没攻击"
//     的假通过——必须证明攻击确实发生）。
//   - 只有"玩家被攻击掉血 + zombie 不掉血"同时成立才 succeed，严格验证无附魔护甲不触发荆棘反伤。
//   两测试交叉验证：穿荆棘 zombie 掉血 + 无荆棘 zombie 不掉血 = 荆棘反伤正确。
//
// night batch + killAllEntities：同 thornsIiiReflectsDamageToAttacker。
// Ref: EnchantmentHelper.cpp:379（getEnchantmentLevel(armor, "minecraft:thorns") <= 0 跳过无附魔护甲）
function noThornsNoReflection(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn("minecraft:zombie", ZOMBIE_POS);
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "victim", 0 as any);

    equipPlainChest(player);

    // 负向对照：zombie 攻击玩家致玩家掉血（证明攻击链路通）+ zombie HP 保持 20（无荆棘反伤）。
    // condition：玩家 HP<20（已被 zombie 近战攻击）且 zombie HP===20（无反伤）。
    //   - 若 thornsIii 测试中 zombie 因其他原因掉血（非荆棘），本测试 zombie HP<20 → condition 不满足 → 超时 FAIL，
    //     从而暴露 thornsIii 假通过。
    //   - 若 zombie 根本没攻击玩家（玩家 HP==20），condition 不满足 → 超时 FAIL（避免"zombie 没掉血但也没攻击"假通过）。
    //   - 只有"玩家被攻击掉血 + zombie 不掉血"同时成立才 succeed，严格验证无附魔护甲不触发荆棘反伤。
    pollUntilSucceed(test, () => {
        const playerHp = readHp(player);
        const zombieHp = readHp(zombie);
        return playerHp > 0 && playerHp < 20 && zombieHp === 20;
    }, {
        startTick: 40, // zombie 需先接近玩家（~5.7 格，约 tick 40 接近）再攻击，首击约 tick 60+
        interval: 10,
        maxTick: 400,
        onTimeout: () => test.assert(false,
            `no-thorns chest should NOT reflect damage: player should be damaged by zombie but zombie HP `
            + `should stay 20. player HP=${readHp(player)}, zombie HP=${readHp(zombie)} `
            + `(if zombie HP<20 thorns triggered on unenchanted armor — getEnchantmentLevel check broken, `
            + `thornsIii test may be false pass; if player HP==20 zombie never attacked player)`),
    });
}

export function registerThornsEnchantTests(): void {
    GameTest.register("MobBehaviorTests", "thorns_iii_reflects_damage_to_attacker", thornsIiiReflectsDamageToAttacker)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(700);

    GameTest.register("MobBehaviorTests", "no_thorns_no_reflection", noThornsNoReflection)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(500);
}
