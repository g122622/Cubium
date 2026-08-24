// 末影龙龙息火球生成龙息云范围伤害行为类 GameTest。
//
// 验证 Cubium 龙息火球命中后生成龙息区域效果云（AreaEffectCloud）并对范围内实体造成瞬间伤害，
// 对齐 MC Java 1.21.11 DragonFireball + AreaEffectCloud。
//
// C++ 链路：
//   test.spawn("minecraft:dragon_fireball", pos) + setVelocity({x,y,z})（任务 #324 新增绑定）
//     → DragonFireballEntity tick → ProjectileEntity::performRayTrace（ProjectileEntity.cpp:378）
//       用 m_builtIn.velocity->m_velocity 做射线终点（setVelocity 设此字段）
//     → 命中实体 onEntityHit（AbstractFireballEntity.cpp:276）/ 命中方块 onBlockHit（:284）
//     → _createDragonBreathCloud（:291）：生成 AreaEffectCloudEntity 于火球位置，
//       radius=3.0、duration=600、radiusPerTick=(7-3)/600（渐扩到7）、waitTime=10、reapplicationDelay=20，
//       addEffect(InstantDamage II, amplifier=1) + DragonBreath 粒子
//   → AreaEffectCloudEntity::tick（EffectEntities.cpp:621）：ticksLived<waitTime(10) 跳过，
//     ticksLived%5==0 时 _applyEffects（:651）
//   → _applyEffects（:656）：getEntitiesInAABB(中心±radius, Y±0.5) 取范围内 LivingEntity，
//     XZ 平面距离≤radius 时 applyInstantEffect(InstantDamage, amplifier=1, multiplier=0.5)（:736）
//   → applyInstantEffect（EffectEntities.cpp:79-137）：amount=(int)(0.5*(6<<1)+0.5)=(int)(6.5)=6 伤害/次
//
// vanilla 对齐（DragonFireball.java:30-62 + AreaEffectCloud.java:193-271 + HealOrHarmMobEffect.java:35）：
//   参数完全一致：setRadius(3.0F)、setDuration(600)、setRadiusPerTick((7-radius)/duration)、
//   addEffect(InstantDamage, amplifier=1)、setWaitTime、setReapplicationDelay(20)。
//   伤害公式 (int)(0.5*(6<<1)+0.5)=6 对齐 vanilla applyInstantenousEffect(multiplier=0.5)。
//   每 5 tick 应用一次（TIME_BETWEEN_APPLICATIONS=5），reapplicationDelay=20 限制同一实体 1 秒一次。
//
// wiki 依据（mob_区域效果云.txt:33,35,49,56 + tech_末影龙火球.txt:36-38）：
//   "末影龙火球撞击后生成效果为瞬间伤害II、紫色的区域效果云"；"半径3，30秒内扩展"；
//   "进入区域效果云的实体每秒受相应效果"（reapplicationDelay=20 tick=1秒）；
//   "即时生效效果在云中效力减至普通药水的1/2"（multiplier=0.5）。
//   注：wiki 文字说扩展到半径5，vanilla 源码实为7，Cubium 按源码实现为7（以源码为准）。
//
// 前置能力（任务 #324）：Entity.setVelocity({x,y,z}) 绑定。此前脚本层无法给 spawn 出的投射物设速度，
// 静止投射物 performRayTrace delta≈0 必 miss（ProjectileEntity.cpp:415），永不触发 onBlockHit/onEntityHit，
// 致本链路端到端测试不可构造。setVelocity 直接设 m_velocity，投射物下一 tick 用此速度做 raytrace 命中目标。
//
// 防假通过设计（正反对照）：
//   - dragon_fireball_spawns_area_effect_cloud：火球 setVelocity 朝 villager 飞 → 命中生成龙息云
//     + villager 受 InstantDamage II 扣血（6/次，waitTime 后每5tick一次）。
//     断言：① 区域内出现 area_effect_cloud 实体（生成链路）；② villager HP 下降≥5（伤害链路，6/次）。
//   - static_dragon_fireball_no_cloud：静止火球（不 setVelocity）→ 不命中 → 不生成云 + villager 不受伤。
//     断言：① 无 area_effect_cloud 实体；② villager HP 不变（满血20）。
//     若 setVelocity 绑定失效或火球 spawn 即自动飞行（无需 setVelocity），本测试 FAIL（静止火球也生成云），
//     暴露 dragon_fireball_spawns_area_effect_cloud 的假通过风险。
//   两测试交叉验证：setVelocity 驱动命中 vs 静止不命中 = 命中链路由速度驱动 + 生成链路对齐。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）+ default batch。killAllEntities 清场防
// 残留实体污染区域查询。龙息云伤害不依赖光照/时间/难度（魔法伤害固定）。
//
// 时序：
//   - tick 0：spawn dragon_fireball (3,2,3) + setVelocity({0,0,3.0})（朝 +Z 飞，1 tick 跨 3 格命中 villager）；
//     spawn villager (3,2,5) 作命中靶 + 龙息云生成位置。
//   - tick 1：火球 tick，performRayTrace 射线 z∈[3,6] 覆盖 villager z=5 → onEntityHit 生成龙息云于 (3,2,5)。
//   - tick 11+：龙息云 waitTime=10 后 ticksLived%5==0（tick 11 起算云自身 ticksLived，约 tick 11-15 首次生效）
//     每 5 tick 6 伤害，villager HP 20→14→8...
//   - tick 40：断言（火球命中后 ~39 tick，龙息云已生效多次，villager HP 下降≥5 稳定可断言）。
//
// 实体身份隔离：villager 用闭包句柄读 HP（同 WitherEffectTests 范式），不依赖区域查询区分。
// area_effect_cloud 用区域查询（getEntities {type}），云无移动且生命周期 600 tick，区域查询稳定。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: AbstractFireballEntity.cpp:276-350（DragonFireballEntity onEntityHit/onBlockHit/_createDragonBreathCloud）
// Ref: EffectEntities.cpp:621-654（AreaEffectCloudEntity tick：waitTime + 每5tick _applyEffects）
// Ref: EffectEntities.cpp:656-765（_applyEffects：范围内 LivingEntity applyInstantEffect multiplier=0.5）
// Ref: EffectEntities.cpp:79-137（applyInstantEffect：(int)(0.5*(6<<1)+0.5)=6 伤害）
// Ref: ProjectileEntity.cpp:378-394（performRayTrace 用 m_velocity 做射线终点）
// Ref: DragonFireball.java:30-62（vanilla 生成参数）+ AreaEffectCloud.java:193-271（vanilla 伤害逻辑）
// Ref: mob_区域效果云.txt:33,35,49,56 + tech_末影龙火球.txt:36-38（wiki 龙息云参数）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const VILLAGER_TYPE = "villager_v2";
const DRAGON_FIREBALL_TYPE = "minecraft:dragon_fireball";
const AREA_EFFECT_CLOUD_TYPE = "minecraft:area_effect_cloud";

// creeper_pit 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。y=2 → 结构 y=1 air 腔，脚下 y=0 grass_block。
const FIREBALL_SPAWN_POS = { x: 3, y: 2, z: 3 };
const VILLAGER_POS = { x: 3, y: 2, z: 5 };
// 区域查询范围（覆盖整个 creeper_pit），用于查 area_effect_cloud。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体句柄当前 HP（currentValue）。句柄无效或无 health 组件返 NaN（同 WitherEffectTests 范式）。
function readHp(entity: unknown): number {
    const health = (entity as any)?.getComponent?.("minecraft:health") as
        | { currentValue?: number }
        | undefined;
    return health?.currentValue ?? NaN;
}

// 查询区域内是否存在 area_effect_cloud 实体（区域限定排除并行测试污染）。
function hasAreaEffectCloud(test: Test): boolean {
    const clouds = test.getDimension().getEntities({
        type: AREA_EFFECT_CLOUD_TYPE,
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
    });
    return clouds.length > 0;
}

// 龙息火球 setVelocity 命中 villager → 生成龙息云 + villager 受 InstantDamage II 扣血。
//
// 火球 (3,2,3) setVelocity({0,0,3.0}) 朝 +Z 飞，1 tick 跨 3 格命中 villager (3,2,5)。
// onEntityHit 调 _createDragonBreathCloud 生成龙息云于 (3,2,5)，waitTime=10 后每 5 tick 对范围内
// LivingEntity applyInstantEffect(InstantDamage, amp=1, mult=0.5) = 6 伤害/次。
//
// 判定（tick 40，火球命中后 ~39 tick，龙息云已多次生效）：
//   ① 区域内出现 area_effect_cloud 实体（生成链路：onEntityHit→_createDragonBreathCloud）。
//   ② villager HP 下降≥5（伤害链路：龙息云 _applyEffects→applyInstantEffect 6伤害/次）。
//      满血 20，waitTime 后首次 6 伤害→HP 14，下降 6≥5 满足。容忍时序偏差用≥5 而非精确 6。
//   - 若 setVelocity 绑定失效：火球静止不命中，无云生成，①②均失败→FAIL，暴露绑定缺陷。
//   - 若 _createDragonBreathCloud 未接 onEntityHit：无云生成，①失败→FAIL。
//   - 若 _applyEffects/applyInstantEffect 链路断裂：有云但 villager 不掉血，②失败→FAIL。
function dragonFireballSpawnsAreaEffectCloud(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    const fireball = test.spawn(DRAGON_FIREBALL_TYPE, FIREBALL_SPAWN_POS);

    // setVelocity 朝 +Z 3.0/tick，1 tick 跨 3 格命中 villager（距 2 格，射线 z∈[3,6] 覆盖 villager z=5）。
    (fireball as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const cloudPresent = hasAreaEffectCloud(test);
        const hp = readHp(villager);
        // ① 龙息云已生成 + ② villager 受 InstantDamage 扣血（HP 下降≥5，满血20→≤15）。
        return cloudPresent && !Number.isNaN(hp) && hp <= 15;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 120,
        onTimeout: () => {
            const cloudPresent = hasAreaEffectCloud(test);
            const hp = readHp(villager);
            test.assert(false,
                `dragon_fireball_spawns_area_effect_cloud: failed: cloudPresent=${cloudPresent}, villager hp=${hp} `
                + `(expected cloudPresent=true [onEntityHit→_createDragonBreathCloud] & hp<=15 [InstantDamage II 6/hit]; `
                + `if cloudPresent=false setVelocity broken or fireball did not hit [onEntityHit not triggered]; `
                + `if cloudPresent=true but hp=20 cloud _applyEffects/applyInstantEffect link broken; `
                + `if hp=NaN villager killed by cloud [HP 20→14→8→2→dead, extend maxTicks or check cloud damage])`);
        },
    });
}

// 静止龙息火球（不 setVelocity）不命中 → 不生成龙息云 + villager 不受伤（负向对照，防假通过）。
//
// 火球 (3,2,3) spawn 后不 setVelocity，velocity=0。performRayTrace delta≈0（ProjectileEntity.cpp:415）
// 返回 miss，不触发 onEntityHit/onBlockHit，不生成龙息云。villager 不受任何伤害，HP 保持 20。
//
// 判定（tick 40，远超火球若命中会生成云的时间）：
//   ① 区域内无 area_effect_cloud 实体（静止火球不命中不生成云）。
//   ② villager HP 仍为 20（满血未受伤）。
// 交叉验证：无云 + HP=20 = 龙息云生成确由"火球命中"触发（velocity 驱动 raytrace），非 spawn 即生成。
//   - 若 setVelocity 绑定对"未调用"也误设速度：火球自动飞行命中生成云，①失败（有云）→FAIL，
//     暴露 dragon_fireball_spawns_area_effect_cloud 假通过风险（云可能由其他原因生成）。
//   - 若火球 spawn 即自带初速度（无需 setVelocity）：同样 ①失败→FAIL。
function staticDragonFireballNoCloud(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    // spawn 静止龙息火球，不 setVelocity（velocity=0，performRayTrace miss，不命中不生成云）。
    test.spawn(DRAGON_FIREBALL_TYPE, FIREBALL_SPAWN_POS);

    pollUntilSucceed(test, () => {
        const cloudPresent = hasAreaEffectCloud(test);
        const hp = readHp(villager);
        // ① 无龙息云 + ② villager 满血未受伤。
        return !cloudPresent && !Number.isNaN(hp) && hp >= 19;
    }, {
        startTick: 40,
        interval: 5,
        maxTick: 80,
        onTimeout: () => {
            const cloudPresent = hasAreaEffectCloud(test);
            const hp = readHp(villager);
            test.assert(false,
                `static_dragon_fireball_no_cloud: failed: cloudPresent=${cloudPresent}, villager hp=${hp} `
                + `(expected cloudPresent=false [static fireball does not hit] & hp>=19 [no damage]; `
                + `if cloudPresent=true static fireball wrongly hit [setVelocity leaked or fireball auto-flies]; `
                + `if hp<19 villager damaged by something else [unrelated source])`);
        },
    });
}

export function registerDragonFireballTests(): void {
    GameTest.register("MobBehaviorTests", "dragon_fireball_spawns_area_effect_cloud", dragonFireballSpawnsAreaEffectCloud)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(200);

    GameTest.register("MobBehaviorTests", "static_dragon_fireball_no_cloud", staticDragonFireballNoCloud)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);
}
