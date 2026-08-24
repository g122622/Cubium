// 雪球命中实体伤害行为类 GameTest（验证 SnowballEntity::onEntityHit 对烈焰人 3 伤害对齐 vanilla）。
//
// 验证 Cubium 雪球命中烈焰人（blaze）造成 3 伤害、命中其他实体 0 伤害，对齐 MC Java 1.21.11
// Snowball.onHitEntity。
//
// C++ 链路（ProjectileItemEntity.cpp:120-143 SnowballEntity::onEntityHit + :155 onImpact）：
//   test.spawn("minecraft:snowball", pos) + setVelocity({x,y,z})（任务 #324 绑定）
//     → SnowballEntity tick → ProjectileEntity::performRayTrace（用 m_velocity 做射线终点）
//     → 命中实体 onImpact（SnowballEntity::onImpact）→ 调基类 ProjectileEntity::onImpact dispatch
//       → onEntityHit：dynamic_cast<BlazeEntity*> 判定
//         - 烈焰人：damage=3，DamageSources::mobProjectile(this, shooter)，livingTarget->hurt(source, 3.0)
//         - 其他实体：damage=0，if(damage>0) 门控跳过（不调 hurt，0 伤害）
//
// 【修复背景（任务 #327）】此前 SnowballEntity::onImpact 直接放粒子+remove，未调用基类
//   ProjectileEntity::onImpact 的 dispatch（switch result.type → onEntityHit/onBlockHit），致
//   onEntityHit 永不触发——dynamic_cast<BlazeEntity*> + hurt(3.0) 链路成为死代码，雪球命中烈焰人
//   不掉血。修复：onImpact 首行调 ProjectileEntity::onImpact(result) 完成 dispatch（对齐 vanilla
//   Snowball.onHit 首行 super.onHit()），再放粒子+remove。本测试即验证此修复。
//
// vanilla 对齐（Snowball.java:51-57）：
//   int i = entity instanceof Blaze ? 3 : 0;
//   entity.hurt(this.damageSources().thrown(this, this.getOwner()), i);
//   硬编码 instanceof Blaze（非数据驱动/非标签），伤害值 3（烈焰人）/ 0（其他），伤害源 thrown。
//   Cubium dynamic_cast<BlazeEntity*> + damage=3 + mobProjectile 等价对齐。
//
// wiki 依据（tech_雪球.txt:59-62 + tech_烈焰人.txt:50）：
//   "雪球会对烈焰人造成 3 伤害"；"烈焰人被雪球击中时受到 3 伤害"。
//
// 前置能力（任务 #324）：Entity.setVelocity。此前雪球命中烈焰人测试因 raytrace 不稳定放弃
// （ThrowableItemTests.ts:16-17：投射物飞行+烈焰人移动+重力叠加致命中不稳定）。setVelocity 让雪球
// 高速（3.0/tick）近距离（1 格）1 tick 命中，blaze 来不及显著位移，命中稳定。
//
// 防假通过设计（正反对照）：
//   - snowball_deals_3_damage_to_blaze：雪球 setVelocity 命中 blaze → blaze HP 20→17（掉 3）。
//     断言 HP∈[14,19]（3 伤害后 17，容忍 blaze 自然回血/无敌帧/多次命中偏差；下界 14 防 0 伤害假通过，
//     上界 19 防异常重伤）。blaze HP=20（BlazeEntity.cpp:173 MAX_HEALTH=20）。
//   - snowball_deals_0_damage_to_villager：雪球命中 villager → villager HP 不变（0 伤害，不调 hurt）。
//     断言 HP===20（满血未受伤）。villager 被动站桩不移动，命中更稳定，同时验证"非烈焰人 0 伤害"门控。
//   两测试交叉验证：烈焰人掉 3 vs villager 掉 0 = BlazeEntity 判定 + 3 伤害对齐。
//   若 onEntityHit 漏 BlazeEntity 判定（对所有实体 0 伤害）：blaze HP=20 不掉→FAIL。
//   若 onEntityHit 误对所有实体 3 伤害：villager HP=17→FAIL。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。blaze 是亡灵火属性，
// 雪球伤害不受光照/时间影响（3 固定伤害）。default batch 即可（blaze 不依赖黑暗生成，test.spawn 强制生成）。
//
// 时序：
//   - tick 0：spawn 靶实体（blaze/villager）+ spawn 雪球 + setVelocity 朝靶飞。
//   - tick 1：雪球 tick，performRayTrace 命中靶 → onEntityHit → hurt。
//   - tick 20：断言靶 HP（留足时间 hurt 链路完成 + blaze 自然回血/无敌帧消退）。
//
// 实体身份隔离：靶实体用闭包句柄读 HP（同 WitherEffectTests 范式）。雪球命中后自身 remove，无需查询。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: ProjectileItemEntity.cpp:120-143（SnowballEntity::onEntityHit：BlazeEntity→3 伤害，其他→0）
// Ref: ProjectileItemEntity.cpp:155（SnowballEntity::onImpact：调基类 dispatch + 粒子 + remove，任务 #327 修复）
// Ref: ProjectileEntity.cpp:301-335（基类 onImpact：偏转检查 + switch→onEntityHit/onBlockHit dispatch）
// Ref: Snowball.java:51-66（vanilla onHitEntity instanceof Blaze ? 3 : 0 + onHit super.onHit()+discard）
// Ref: BlazeEntity.cpp:173（MAX_HEALTH=20）
// Ref: tech_雪球.txt:59-62 + tech_烈焰人.txt:50（wiki 雪球对烈焰人 3 伤害）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const SNOWBALL_TYPE = "minecraft:snowball";
const BLAZE_TYPE = "minecraft:blaze";
const VILLAGER_TYPE = "villager_v2";

// creeper_pit 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=2 → 结构 y=1 air 腔，脚下 y=0 grass_block。
// 雪球 (3,2,3) 朝 +Z setVelocity({0,0,3.0})，1 tick 跨 3 格，射线 z∈[3,6] 覆盖靶 z∈[4,5]。
const SNOWBALL_POS = { x: 3, y: 2, z: 3 };
const BLAZE_POS = { x: 3, y: 2, z: 5 };
const VILLAGER_TARGET_POS = { x: 3, y: 2, z: 5 };

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN（同 WitherEffectTests 范式）。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 雪球 setVelocity 命中烈焰人 → 3 伤害（验证 SnowballEntity::onEntityHit BlazeEntity 判定 + 3 伤害）。
//
// 雪球 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 blaze (3,2,5)（距 2 格，射线 z∈[3,6]
// 覆盖 blaze z=5）。onEntityHit dynamic_cast<BlazeEntity*> 命中 → damage=3 → hurt(mobProjectile, 3.0)。
// blaze HP 20→17。
//
// 判定（tick 20，雪球命中后 ~19 tick，hurt 链路完成 + 无敌帧消退）：
//   blaze HP∈[14,19]（3 伤害后 17，容忍 blaze 自然回血/无敌帧/多次命中偏差）。
//   - 下界 14：证明承受了 ~3 雪球伤害（HP 20→17），排除"0 伤害链路失效 HP=20"假通过。
//   - 上界 19：防异常重伤（如多次命中累加 6/9 或 blaze 被引燃额外掉血）。
//   - 若 onEntityHit 漏 BlazeEntity 判定：damage=0 不调 hurt，HP=20→超时 FAIL。
//   - 若 setVelocity 失效雪球未命中：blaze HP=20→超时 FAIL。
//
// 注：blaze 是敌对 mob 会飞行，但 setVelocity 高速（3.0）1 tick 命中，blaze 位移 <0.5 格仍在射线上。
//   pollUntilSucceed 容忍 blaze 偶发位移致命中延迟（interval 轮询覆盖 tick 20~60）。
function snowballDeals3DamageToBlaze(test: Test): void {
    (test as any).killAllEntities();
    const blaze = test.spawn(BLAZE_TYPE, BLAZE_POS);
    const snowball = test.spawn(SNOWBALL_TYPE, SNOWBALL_POS);

    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中 blaze（距 2 格，射线 z∈[3,6] 覆盖 blaze z=5）。
    (snowball as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const hp = readHp(blaze);
        // blaze HP∈[14,19]（3 伤害后 17，容忍自然回血/无敌帧/多次命中偏差）。
        return !Number.isNaN(hp) && hp >= 14 && hp <= 19;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 60,
        onTimeout: () => {
            const hp = readHp(blaze);
            test.assert(false,
                `snowball_deals_3_damage_to_blaze: failed: blaze hp=${hp} `
                + `(expected 14..19 = 20 - 3 snowball damage; `
                + `if hp=20 onEntityHit missing BlazeEntity check [damage=0 no hurt] or setVelocity broken [snowball missed]; `
                + `if hp<14 abnormal [multiple hits or extra damage])`);
        },
    });
}

// 雪球命中 villager → 0 伤害（验证非烈焰人实体 0 伤害门控，负向对照防假通过）。
//
// 雪球 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 villager (3,2,5)。
// onEntityHit dynamic_cast<BlazeEntity*> 失败（villager 非 blaze）→ damage=0 → if(damage>0) 门控跳过，
// 不调 hurt，villager HP 保持 20。
//
// 判定（tick 20）：villager HP===20（满血未受伤，0 伤害不调 hurt）。
//   - 若 onEntityHit 误对所有实体 3 伤害：villager HP=17→FAIL，暴露 blaze 判定失效。
//   - 若雪球未命中：villager HP=20（未受伤），本断言通过但无法区分"0 伤害"与"未命中"——
//     故本测试需与 snowball_deals_3_damage_to_blaze 配对：blaze 掉 3 证明雪球确实命中且 hurt 生效，
//     villager 不掉血证明 hurt 仅对 blaze 触发（非命中问题）。
//
// villager 被动站桩不移动，命中比 blaze 更稳定（无位移风险）。
function snowballDeals0DamageToVillager(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_TARGET_POS);
    const snowball = test.spawn(SNOWBALL_TYPE, SNOWBALL_POS);

    (snowball as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        // villager HP===20（0 伤害不调 hurt，满血未受伤）。
        return !Number.isNaN(hp) && hp === 20;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const hp = readHp(villager);
            test.assert(false,
                `snowball_deals_0_damage_to_villager: failed: villager hp=${hp} `
                + `(expected 20 = no damage [non-blaze entity, onEntityHit damage=0 gate]; `
                + `if hp<20 onEntityHit wrongly damages non-blaze [missing BlazeEntity check]; `
                + `if hp=20 correct — pair with snowball_deals_3_damage_to_blaze to confirm snowball hits)`);
        },
    });
}

export function registerSnowballDamageTests(): void {
    GameTest.register("MobBehaviorTests", "snowball_deals_3_damage_to_blaze", snowballDeals3DamageToBlaze)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "snowball_deals_0_damage_to_villager", snowballDeals0DamageToVillager)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
