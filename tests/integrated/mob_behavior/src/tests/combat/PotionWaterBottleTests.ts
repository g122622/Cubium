// 喷溅药水（水瓶）命中行为类 GameTest（验证 PotionEntity::onImpact 水瓶特例对齐 vanilla AbstractThrownPotion）。
//
// 验证 Cubium 喷溅药水实体（splash_potion）默认携带水瓶物品，命中破裂时对范围内水敏感实体（烈焰人/雪傀儡）
// 造成 1.0 indirectMagic 伤害、对非水敏感实体（村民）0 伤害，对齐 MC Java 1.21.11
// AbstractThrownPotion.onHitAsWater。
//
// C++ 链路（ProjectileItemEntity.cpp）：
//   PotionEntity 构造（:411-424）：attach PotionProjectileComponent + setItemStack(createSplashPotionItem(WATER))
//     → 实体默认携带 splash_potion 物品且 PotionContents 为空（potionId 空）→
//       PotionUtils::getPotion 对空 potionId 返回 Potions::WATER → isWaterBottle==true。
//     此前 getItemStack 返回空 ItemStack（getPotion 返 EMPTY → isWaterBottle=false），致
//     test.spawn 的 splash_potion 永不进入 onHitAsWater 水瓶分支（修复于任务 #330）。
//   test.spawn("minecraft:potion", pos) + setVelocity({x,y,z})（任务 #324 绑定）
//     → PotionEntity tick → ProjectileEntity::performRayTrace（用 m_velocity 做射线终点）
//     → 命中 onImpact（PotionEntity::onImpact :437）→ 首行 ProjectileEntity::onImpact dispatch
//       （对齐 vanilla onHit 首行 super.onHit()，任务 #330 修复，此前未调基类致 onBlockHit 死代码）
//     → isWater==true → _onHitAsWater（:656）
//   _onHitAsWater：AABB inflate(4,2,4) 取范围内 LivingEntity，distSq<16 时：
//     - isWaterSensitive()==true → hurt(indirectMagic(this, shooter), 1.0F)
//     - isOnFire && isAlive → extinguishFire()
//   isWaterSensitive() 经 Entity 基类虚派发（任务 #330 修复：Entity.hpp 新增 virtual isWaterSensitive()
//   默认 false，BlazeEntity/SnowGolemEntity override 返回 true；此前为非虚函数仅在子类定义，基类指针
//   查询返回 false，致 _onHitAsWater 对 blaze/snow_golem 的 hurt 永不触发——水敏感伤害链路死代码）。
//
// vanilla 对齐（AbstractThrownPotion.java）：
//   onHit:70-85 首行 super.onHit() dispatch，再 potioncontents.is(Potions.WATER)→onHitAsWater。
//   onHitAsWater:87-106 AABB inflate(4,2,4)，WATER_SENSITIVE_OR_ON_FIRE 谓词：水敏感→
//     hurt(damageSources().indirectMagic(this, owner), 1.0F)；着火→extinguishFire()。
//   isSensitiveToWater()（Entity 默认 false，Blaze/SnowGolem override true）。
//
// wiki 依据（tech_喷溅药水.txt + tech_烈焰人.txt + tech_雪傀儡.txt）：
//   "水瓶可以扑灭火和浇灭蜡烛"；"烈焰人接触水或被水瓶击中受伤害"；"雪傀儡在水中/被水瓶击中受伤害"。
//
// 前置能力（任务 #324）：Entity.setVelocity。静止药水 performRayTrace delta≈0 必 miss，setVelocity 让
// 药水高速（3.0/tick）1 tick 命中近距离靶。
//
// 防假通过设计（正反对照）：
//   - splash_water_bottle_damages_blaze：药水 setVelocity 朝 blaze 飞，命中破裂 _onHitAsWater 对 blaze
//    （水敏感）hurt 1.0。blaze HP 20→19。断言 HP∈[17,20]（1 伤害后 19，容忍 blaze 自然回血/无敌帧）。
//     下界 17 防 0 伤害假通过（isWaterSensitive 虚派发失效 HP=20）。
//   - splash_water_bottle_no_damage_villager：药水 setVelocity 朝 villager 飞，命中破裂 _onHitAsWater 对
//     villager（非水敏感）不 hurt。villager HP===20（满血未受伤）。
//     若 isWaterSensitive 误对所有实体 true：villager HP=19→FAIL，暴露水敏感判定失效。
//   两测试交叉验证：blaze 掉 1 vs villager 掉 0 = isWaterSensitive 虚派发 + 水瓶特例 hurt 对齐。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。
// 水瓶间接魔法伤害不依赖光照/时间/难度（固定 1.0），default batch 即可。
//
// 时序：
//   - tick 0：spawn 靶实体（blaze/villager）+ spawn 药水 + setVelocity 朝靶飞。
//   - tick 1：药水 tick，performRayTrace 命中靶 → onImpact → _onHitAsWater → hurt。
//   - tick 20：断言靶 HP（留足 hurt 链路完成 + 无敌帧消退）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: ProjectileItemEntity.cpp:411-424（PotionEntity 构造：默认水瓶 ItemStack，任务 #330）
// Ref: ProjectileItemEntity.cpp:437-587（onImpact：调基类 dispatch + 水瓶分支 _onHitAsWater，任务 #330）
// Ref: ProjectileItemEntity.cpp:656-701（_onHitAsWater：水敏感 hurt 1.0 + 着火灭火）
// Ref: Entity.hpp（virtual isWaterSensitive 默认 false，任务 #330 修复虚派发）
// Ref: BlazeEntity.hpp:143 / SnowGolemEntity.hpp:123（isWaterSensitive override true）
// Ref: AbstractThrownPotion.java:70-106（vanilla onHit + onHitAsWater）
// Ref: Entity.java isSensitiveToWater（默认 false，Blaze/SnowGolem override true）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// Cubium 实体注册名：minecraft:potion（VanillaEntities.cpp:1093 EntityTypeKeys::POTION → "minecraft:potion"）。
// 注：Java/基岩原版该实体 ID 为 splash_potion，Cubium 注册为 minecraft:potion（同一 PotionEntity 承载喷溅+滞留）。
const SPLASH_POTION_TYPE = "minecraft:potion";
const BLAZE_TYPE = "minecraft:blaze";
const VILLAGER_TYPE = "villager_v2";

// creeper_pit 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=2 → 结构 y=1 air 腔，脚下 y=0 grass_block。
// 药水 (3,2,3) 朝 +Z setVelocity({0,0,3.0})，1 tick 跨 3 格，射线 z∈[3,6] 覆盖靶 z∈[4,5]。
const POTION_POS = { x: 3, y: 2, z: 3 };
const BLAZE_POS = { x: 3, y: 2, z: 5 };
const VILLAGER_TARGET_POS = { x: 3, y: 2, z: 5 };

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN（同 WitherEffectTests 范式）。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 水瓶药水 setVelocity 命中烈焰人 → 1.0 indirectMagic 伤害（验证 isWaterSensitive 虚派发 + 水瓶 hurt）。
//
// 药水 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 blaze (3,2,5)（距 2 格，射线 z∈[3,6] 覆盖 blaze z=5）。
// onImpact isWater==true → _onHitAsWater：blaze isWaterSensitive()==true → hurt(indirectMagic, 1.0)。
// blaze HP 20→19。
//
// 判定（tick 20，药水命中后 ~19 tick，hurt 链路完成 + 无敌帧消退）：
//   blaze HP∈[17,20]（1 伤害后 19，容忍 blaze 自然回血/无敌帧偏差）。
//   - 下界 17：证明承受了水瓶伤害（HP 20→19），排除"isWaterSensitive 虚派发失效 / isWaterBottle 判定 false
//     / _onHitAsWater 链路断裂 → HP=20 不掉"假通过。
//   - 上界 20：防异常（虽允许 20 容忍回血，但若恒 20 多次轮询则超时 FAIL 暴露 0 伤害）。
//   - 若 isWaterSensitive 虚派发失效（基类返回 false）：blaze 不 hurt，HP=20→超时 FAIL。
//   - 若 setVelocity 失效药水未命中：blaze HP=20→超时 FAIL。
function splashWaterBottleDamagesBlaze(test: Test): void {
    (test as any).killAllEntities();
    const blaze = test.spawn(BLAZE_TYPE, BLAZE_POS);
    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);

    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中 blaze（距 2 格，射线 z∈[3,6] 覆盖 blaze z=5）。
    (potion as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const hp = readHp(blaze);
        // blaze HP∈[17,20]（1 伤害后 19，容忍自然回血/无敌帧偏差；下界 17 防异常重伤）。
        return !Number.isNaN(hp) && hp >= 17 && hp <= 19;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const hp = readHp(blaze);
            test.assert(false,
                `splash_water_bottle_damages_blaze: failed: blaze hp=${hp} `
                + `(expected 17..19 = 20 - 1 water-bottle indirectMagic damage; `
                + `if hp=20 isWaterSensitive vdispatch broken [Entity base returns false] or isWaterBottle=false [empty ItemStack] or _onHitAsWater link broken or setVelocity missed; `
                + `if hp<17 abnormal [multiple hits or extra damage])`);
        },
    });
}

// 水瓶药水 setVelocity 命中村民 → 0 伤害（验证非水敏感实体不被 hurt，负向对照防假通过）。
//
// 药水 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 villager (3,2,5)。
// onImpact isWater==true → _onHitAsWater：villager isWaterSensitive()==false（基类默认）→ 不 hurt。
// villager HP 保持 20。
//
// 判定（tick 20）：villager HP===20（满血未受伤，非水敏感不 hurt）。
//   - 若 isWaterSensitive 误对所有实体 true：villager HP=19→FAIL，暴露水敏感判定失效。
//   - 若药水未命中：villager HP=20（未受伤），本断言通过但无法区分"0 伤害"与"未命中"——
//     故本测试需与 splash_water_bottle_damages_blaze 配对：blaze 掉 1 证明药水确实命中且 _onHitAsWater 生效，
//     villager 不掉血证明 hurt 仅对水敏感实体触发。
//
// villager 被动站桩不移动，命中比 blaze 更稳定（无位移风险）。
function splashWaterBottleNoDamageVillager(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_TARGET_POS);
    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);

    (potion as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        // villager HP===20（非水敏感不 hurt，满血未受伤）。
        return !Number.isNaN(hp) && hp === 20;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 60,
        onTimeout: () => {
            const hp = readHp(villager);
            test.assert(false,
                `splash_water_bottle_no_damage_villager: failed: villager hp=${hp} `
                + `(expected 20 = no damage [non-water-sensitive entity, base isWaterSensitive=false]; `
                + `if hp<20 isWaterSensitive wrongly true for all [vdispatch returns true]; `
                + `if hp=20 correct — pair with splash_water_bottle_damages_blaze to confirm potion hits)`);
        },
    });
}

export function registerPotionWaterBottleTests(): void {
    GameTest.register("MobBehaviorTests", "splash_water_bottle_damages_blaze", splashWaterBottleDamagesBlaze)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "splash_water_bottle_no_damage_villager", splashWaterBottleNoDamageVillager)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
