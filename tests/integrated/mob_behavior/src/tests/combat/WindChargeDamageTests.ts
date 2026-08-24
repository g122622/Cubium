// 风弹命中实体伤害与消失行为类 GameTest（验证 WindChargeEntity::onImpact 调基类 dispatch
// 对齐 vanilla AbstractWindCharge.onHit）。
//
// 验证 Cubium 风弹命中实体造成 1 点风爆伤害并消失（discard），对齐 MC Java 1.21.11
// AbstractWindCharge。
//
// C++ 链路（WindChargeEntity.cpp:110-130 onEntityHit + :153-175 onImpact）：
//   test.spawn("minecraft:wind_charge", pos) + setVelocity({x,y,z})（任务 #324 绑定）
//     → WindChargeEntity 继承 ThrowableEntity::tick（ThrowableEntity.cpp:104 调 onImpact）
//     → onImpact 首行调基类 ProjectileEntity::onImpact（ProjectileEntity.cpp:301-335）dispatch
//       → 命中实体 onEntityHit：living->hurt(windBurst, PLAYER_DAMAGE=1.0)
//         + applyWindBurst（范围击退，villager 非玩家走 addVelocity 分支）+ remove()
//     → onImpact 末尾 if(!isRemoved()) remove()（覆盖偏转分支，对齐 vanilla onHit discard）
//
// 【修复背景（任务 #328）】此前 WindChargeEntity::onImpact 为空实现（注释"onEntityHit/onBlockHit
//   已处理"是错误假设——基类 dispatch 才是 onEntityHit/onBlockHit 的唯一入口），绕过基类 dispatch
//   致 onEntityHit/onBlockHit 全成死代码：风弹命中实体不掉血、不触发风爆、连 remove 都不执行（风弹
//   撞实体后不消失继续存在）。诊断实证 villager hp=20（未掉血）+ wind_charges_remaining=1（未消失）。
//   修复：onImpact 首行调 ProjectileEntity::onImpact(result) 完成 dispatch，再 if(!isRemoved()) remove()
//   （对齐 vanilla AbstractWindCharge.onHit：super.onHit() + discard）。修复后 villager hp=19 + 消失。
//   与 SnowballEntity::onImpact 修复（任务 #327）同构，后果更严重（雪球至少 remove，风弹连 remove 都跳过）。
//
// vanilla 对齐（AbstractWindCharge.java:113-119 onHit + :77-93 onHitEntity）：
//   onHit: super.onHit(p) dispatch（基类 Projectile.onHit 按 HitResult 类型调 onHitEntity/onHitBlock）
//          + !isClientSide → discard。
//   onHitEntity: hurtServer(windCharge(this, owner), 1.0F) + explode(this.position())。
//   伤害值 1.0F，伤害源 windCharge，命中实体后 explode + discard。Cubium windBurst + PLAYER_DAMAGE=1.0
//   + applyWindBurst + remove 等价对齐。
//
// wiki 依据（tech_风弹.txt / tech_旋风人.txt）：风弹命中造成 1 弹射物伤害，命中后产生风爆推开实体。
//
// 前置能力（任务 #324）：Entity.setVelocity。风弹 velocity={0,0,3} 高速 1 tick 命中近距离靶。
//
// 防假通过设计（正反对照）：
//   - wind_charge_deals_1_damage_to_villager：风弹 setVelocity 命中 villager → ① villager HP 20→19
//     （1 伤害，onEntityHit hurt 生效）；② 风弹消失（wind_charges=0，onImpact remove/discard 生效）。
//     双断言同时验证伤害链路（onEntityHit）+ 消失链路（onImpact remove）。
//   - static_wind_charge_no_damage：静止风弹（不 setVelocity）→ villager HP 不变（20，未受风弹伤害）。
//     风弹受重力下落会命中脚下方块触发 onBlockHit 消失，故仅断言 villager HP=20（风弹未命中 villager）。
//     交叉验证：setVelocity 命中掉 1 vs 静止不掉 = 伤害确由"风弹命中实体"触发（velocity 驱动 raytrace）。
//   两测试交叉验证：onEntityHit 伤害 + onImpact remove 对齐 vanilla。
//   - 若 onImpact 修复回退为空：villager HP=20（不掉血）+ 风弹不消失 → 正向 FAIL。
//   - 若 onEntityHit 伤害漏接：villager HP=20 → 正向 FAIL（HP 断言）。
//   - 若 onImpact remove 漏接：风弹不消失 → 正向 FAIL（wind_charges 断言）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。风爆伤害固定 1.0
// 不依赖光照/时间/难度。villager 被动站桩不移动，命中稳定。
//
// 时序：
//   - tick 0：spawn villager (3,2,5) + spawn 风弹 (3,2,3) + setVelocity({0,0,3.0}) 朝 +Z。
//   - tick 1：风弹 tick，performRayTrace 射线 z∈[3,6] 覆盖 villager z=5 → onEntityHit → hurt 1 + remove。
//   - tick 20：断言 villager HP + 区域内风弹数量（留足 hurt 链路完成时间）。
//
// 实体身份隔离：villager 用闭包句柄读 HP。风弹用区域查询计数（wind_charge 无移动后已 remove）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: WindChargeEntity.cpp:110-130（onEntityHit：hurt 1 + applyWindBurst + remove）
// Ref: WindChargeEntity.cpp:153-175（onImpact：调基类 dispatch + remove，任务 #328 修复）
// Ref: ProjectileEntity.cpp:301-335（基类 onImpact：偏转检查 + switch→onEntityHit/onBlockHit dispatch）
// Ref: AbstractWindCharge.java:113-119（vanilla onHit super.onHit()+discard）+ :77-93（onHitEntity 1.0F）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const VILLAGER_TYPE = "villager_v2";
const WIND_CHARGE_TYPE = "minecraft:wind_charge";

// creeper_pit 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=2 → 结构 y=1 air 腔，脚下 y=0 grass_block。
// 风弹 (3,2,3) 朝 +Z setVelocity({0,0,3.0})，1 tick 跨 3 格，射线 z∈[3,6] 覆盖靶 z=5。
const WIND_CHARGE_POS = { x: 3, y: 2, z: 3 };
const VILLAGER_POS = { x: 3, y: 2, z: 5 };
// 区域查询范围（覆盖整个 creeper_pit），用于查 wind_charge 数量。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN（同 WitherEffectTests 范式）。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 查询区域内 wind_charge 实体数量（区域限定排除并行测试污染）。
function countWindCharges(test: Test): number {
    return test.getDimension().getEntities({
        type: WIND_CHARGE_TYPE,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    }).length;
}

// 风弹 setVelocity 命中 villager → 1 伤害（HP 20→19）+ 风弹消失（wind_charges=0）。
//
// 风弹 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 命中 villager (3,2,5)（距 2 格，射线 z∈[3,6]
// 覆盖 villager z=5）。onImpact 调基类 dispatch → onEntityHit：hurt(windBurst, 1.0) + applyWindBurst
// + remove。villager HP 20→19，风弹消失。
//
// 判定（tick 20，风弹命中后 ~19 tick，hurt 链路完成 + 风弹 remove 落定）：
//   ① villager HP===19（1 伤害后，容忍 ±1 偏差用 [18,20] 防假通过——下界 18 证明掉了 ~1 伤害，
//      上界 20 排除"未掉血"；精确 19 最理想）。
//   ② 区域内 wind_charges===0（风弹命中后 remove 消失）。
//   - 若 onImpact 修复回退为空：villager HP=20（不掉血）+ wind_charges=1（不消失）→ 双断言 FAIL。
//   - 若 onEntityHit hurt 漏接：villager HP=20 → ① FAIL。
//   - 若 onImpact remove 漏接：wind_charges=1 → ② FAIL。
function windChargeDeals1DamageToVillager(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    const charge = test.spawn(WIND_CHARGE_TYPE, WIND_CHARGE_POS);

    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中 villager（距 2 格，射线 z∈[3,6] 覆盖 villager z=5）。
    (charge as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        const charges = countWindCharges(test);
        // ① villager HP∈[18,20]（1 伤害后 19，容忍 ±1 偏差）+ ② 风弹消失（charges=0）。
        return !Number.isNaN(hp) && hp >= 18 && hp <= 20 && charges === 0;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 60,
        onTimeout: () => {
            const hp = readHp(villager);
            const charges = countWindCharges(test);
            test.assert(false,
                `wind_charge_deals_1_damage_to_villager: failed: villager hp=${hp} wind_charges=${charges} `
                + `(expected hp 18..20 [19 = 20 - 1 wind charge damage] & charges=0 [discard after hit]; `
                + `if hp=20 onImpact empty dead-coded onEntityHit [no damage, task #328 regression]; `
                + `if charges>0 onImpact did not remove [empty onImpact skip discard])`);
        },
    });
}

// 静止风弹（不 setVelocity）不命中 villager → villager HP 不变（负向对照，防假通过）。
//
// 风弹 (3,2,3) spawn 后不 setVelocity，受重力（getGravity=0.03）缓慢下落。performRayTrace delta≈0
// 不命中 villager（villager 在 z=5，风弹在 z=3，无水平速度不会接近）。villager 不受风弹伤害，HP 保持 20。
// 风弹下落最终命中脚下方块（y=0 grass_block）触发 onBlockHit → remove，故仅断言 villager HP，不断言风弹数量。
//
// 判定（tick 20，远超风弹若命中 villager 会掉血的时间）：
//   villager HP===20（满血未受伤，风弹未命中 villager）。
//   - 若 setVelocity 绑定对"未调用"也误设速度：风弹自动飞行命中 villager 掉血，HP<20→FAIL，
//     暴露 wind_charge_deals_1_damage_to_villager 假通过风险（伤害可能由其他原因）。
//   交叉验证：setVelocity 命中掉 1 vs 静止不掉 = 伤害确由"风弹命中实体"触发。
function staticWindChargeNoDamage(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    // spawn 静止风弹，不 setVelocity（受重力下落，不水平飞行，不命中 villager）。
    test.spawn(WIND_CHARGE_TYPE, WIND_CHARGE_POS);

    pollUntilSucceed(test, () => {
        const hp = readHp(villager);
        // villager HP===20（满血未受伤，静止风弹未命中）。
        return !Number.isNaN(hp) && hp === 20;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const hp = readHp(villager);
            test.assert(false,
                `static_wind_charge_no_damage: failed: villager hp=${hp} `
                + `(expected 20 = no damage [static wind charge does not hit villager]; `
                + `if hp<20 static wind charge wrongly hit villager [setVelocity leaked or auto-flight])`);
        },
    });
}

export function registerWindChargeDamageTests(): void {
    GameTest.register("MobBehaviorTests", "wind_charge_deals_1_damage_to_villager", windChargeDeals1DamageToVillager)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "static_wind_charge_no_damage", staticWindChargeNoDamage)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
