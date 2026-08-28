// 闪电直击伤害行为类 GameTest。
//
// 验证 Cubium LightningBoltEntity 的闪电直击伤害链路对齐 vanilla 1.21.11：
//   1. 闪电对伤害范围内（±3 XZ）的 LivingEntity 造成 5.0 伤害。
//   2. 闪电对伤害范围外的实体不造成伤害（验证 DAMAGE_RADIUS_XZ=3 边界）。
//
// vanilla 链路（LightningBolt.java:140-160 tick 内 life>=0 分支）：
//   闪电在 life==2/1/0 三个 tick 各对范围内存活实体调 entity.thunderHit(level, this)。
//   Entity.thunderHit（Entity.java:2725-2732）：
//     setRemainingFireTicks(remainingFireTicks + 1);  // 普通实体 0→1，不引燃
//     if (remainingFireTicks == 0) igniteForSeconds(8.0F);  // 仅刚灭火免疫期(-1→0)触发，普通场景不引燃
//     hurtServer(level, lightningBolt(), 5.0F);  // 5 伤害
//   wiki tech_闪电束.txt#造成伤害（第 54 行）：闪电对 6×6×12 区域实体造成 5 伤害；
//   第 56 行：JE 闪电直接伤害不着火（引燃来自生成的火方块），与 thunderHit 普通场景不引燃一致。
//
// Cubium 链路（EffectEntities.cpp:488-536 _damageEntities）：
//   m_lightningState==2 首次 tick 触发，对 ±3 XZ 范围内 LivingEntity hurt(lightningBolt, 5.0f)，
//   再调 entity->onStruckByLightning()（充能/转化回调）。
//   注：Cubium 仅首 tick 伤害一次，vanilla 三 tick 各 thunderHit 但伤害免疫（20 tick 同额吞）让生物
//   实际只受一次 5——对 LivingEntity 两者等效。Cubium 缺 thunderHit 的 setRemainingFireTicks(+1)/
//   igniteForSeconds(8) 引燃分支，但该分支普通场景不触发（需 remainingFireTicks==-1 刚灭火态），
//   故非可观测缺陷（wiki 亦称 JE 闪电直接伤害不着火）。
//
// 测试实体选择：zombie（HP 20，亡灵 fireImmune，非转化实体）。
//   - fireImmune：闪电 _igniteBlocks 在 normal/hard 难度放火于击中点，fireImmune 实体不被引燃，
//     消除火引燃持续掉血干扰 HP 断言（cow/sheep 可被引燃致 HP 持续下降不稳定）。
//   - 非转化实体：zombie 不在猪/苦力怕/村民/哞菇转化列表，onStruckByLightning 基类空实现不转化
//     不 discard，仅受 5 伤害存活，HP 干净可测。
//   - night batch：zombie 亡灵白天燃烧掉血干扰，night 不燃。night batch 自然刷怪风险用闭包句柄规避。
//   - 闪电伤害类型 LightningBolt 属 IS_LIGHTNING 标签（非 IS_FIRE），fireImmune 实体不免疫闪电伤害，
//     zombie 正常受 5 伤害。
//
// 防假通过设计（正反对照）：
//   - lightning_direct_hit_deals_5_damage：zombie 在闪电同格（范围内）→ HP 20→15（受 5 伤害）。
//   - lightning_out_of_range_no_damage：zombie 距闪电 4 格（超 ±3 范围）→ HP 仍 20（不受伤害）。
//   若 _damageEntities 范围判定失效（恒全维度伤害）：范围外测试 FAIL（HP 应 20 实际 15）。
//   若伤害链路断裂（hurt 未接入/0 伤害）：范围内测试 FAIL（HP 应 15 实际 20）。
//   两测试交叉验证：范围内受 5 伤害 + 范围外不受伤害 = 闪电伤害范围与数值链路对齐。
//
// 确定性设计：用 test.spawn("lightning_bolt", pos) 直接生成闪电实体（同 creeper_charged_by_lightning
//   范式），不依赖雷暴天气渐变（/weather thunder 需 ~91 tick 强度达标 isThundering()）。直接 spawn
//   的闪电首 tick _initializeState 设 m_lightningState=2 同 tick _damageEntities，确定性最高。
//
// 实体身份隔离：用闭包直接持有 test.spawn 返回的 zombie 实体句柄读 getComponent("minecraft:health")
//   .currentValue，不按 type 区域查询，规避 night batch 自然刷怪污染。
//
// 环境选择：creeper_pit（7×5×7 开放坑，y=0 grass_block 地板）+ night batch + killAllEntities 清场。
//   - night batch：zombie 不燃 + 隔离白天亡灵燃烧干扰。
//   - killAllEntities：清场防自然刷怪干扰。
//   - 露天开放坑：闪电生成不依赖天空可达（直接 spawn 不走自然生成 canSeeSky 门控）。
//
// className 恒为 MobBehaviorTests（对齐 mob_behavior 包约定）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_闪电束.txt#造成伤害（5 伤害，JE 不着火）
// Ref: EffectEntities.cpp:488-536（_damageEntities：±3 XZ 范围 hurt(5.0) + onStruckByLightning）
// Ref: EffectEntities.cpp:331-349（_initializeState：m_lightningState=2 首 tick 触发）
// Ref: Entity.java:2725-2732（vanilla thunderHit：setRemainingFireTicks+1 + hurtServer 5.0）
// Ref: LightningBolt.java:140-160（vanilla tick life>=0 三 tick thunderHit）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const ZOMBIE_TYPE = "minecraft:zombie";
const LIGHTNING_TYPE = "minecraft:lightning_bolt";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=2 → 结构文件 y=1 air 腔，脚下 y=0（文件）grass_block 支撑。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 读取实体当前血量（HP）。实体句柄由闭包持有，规避区域查询污染。
// 实体死亡/移除后 getComponent("health") 返回 undefined → 返回 -1。
function readHp(entity: any): number {
    const health = entity.getComponent("minecraft:health");
    if (health === undefined) {
        return -1;
    }
    return (health as any).currentValue as number;
}

// 闪电直击范围内实体造成 5.0 伤害（wiki tech_闪电束.txt#造成伤害：5 伤害）。
//
// zombie(HP 20, fireImmune, 非转化) 与闪电同格 spawn：闪电 ±3 XZ 命中范围必覆盖 zombie。
// 闪电首 tick _damageEntities → hurt(lightningBolt, 5.0) → zombie HP 20→15。
// zombie fireImmune 不被闪电放的火引燃，HP 稳定在 15（无火持续掉血干扰）。
// 断言 HP<=16 && HP>=14（约 5 伤害，容忍浮点/护甲微扰；zombie 无护甲精确 15）。
//
// 若伤害链路断裂（hurt 未调/0 伤害）：HP 仍 20 不在 [14,16] → FAIL。
// 若伤害数值错误（如 10）：HP 10 不在 [14,16] → FAIL。
function lightningDirectHitDeals5Damage(test: Test): void {
    (test as any).killAllEntities();
    const zombie = test.spawn(ZOMBIE_TYPE, { x: 3, y: 2, z: 3 });
    // 闪电同格 spawn（确定性触发，无需雷暴渐变）。首 tick 即 _damageEntities。
    test.spawn(LIGHTNING_TYPE, { x: 3, y: 2, z: 3 });

    // 轮询断言 zombie HP 下降约 5（20→15）。
    // startTick=3：闪电 spawn 后约 1-2 tick 首 tick 伤害生效，t=3 余量充足。
    // maxTick=60：伤害当 tick 完成，HP 稳定 15；超时即 FAIL（伤害链路断裂）。
    pollUntilSucceed(test, () => {
        const hp = readHp(zombie);
        return hp >= 14 && hp <= 16;
    }, {
        startTick: 3,
        interval: 2,
        maxTick: 60,
        onTimeout: () => {
            test.assert(false,
                `lightning_direct_hit_deals_5_damage: failed: zombie HP=${readHp(zombie)} `
                + `(expected 14..16 ≈ 20-5; if HP=20 lightning hurt not wired/0 damage; `
                + `if HP<14 over-damage or fire ignition [zombie should be fireImmune])`);
        },
    });
}

// 闪电对范围外实体不造成伤害（验证 DAMAGE_RADIUS_XZ=3 边界）。
//
// zombie(HP 20) 在 (1,2,3)，闪电在 (5,2,3)，水平距离 4 格 > ±3 XZ 范围 → zombie 不在伤害 AABB 内。
// 闪电 _damageEntities 构建 AABB [x-3, x+3]×[z-3, z+3]，zombie x=1 闪电 x=5，|5-1|=4 > 3 → 不命中。
// 断言 zombie HP 仍 20（未受闪电伤害）。
//
// 若范围判定失效（_damageEntities 恒全维度伤害或半径错误过大）：zombie HP 下降 → FAIL。
// 此为 lightningDirectHitDeals5Damage 的反例对照，防"任何位置都掉血"假通过。
function lightningOutOfRangeNoDamage(test: Test): void {
    (test as any).killAllEntities();
    // zombie 在 x=1，闪电在 x=5，水平距 4 格 > DAMAGE_RADIUS_XZ(3)。
    const zombie = test.spawn(ZOMBIE_TYPE, { x: 1, y: 2, z: 3 });
    test.spawn(LIGHTNING_TYPE, { x: 5, y: 2, z: 3 });

    // 轮询断言 zombie HP 仍 20（不受范围外闪电伤害）。
    // startTick=5：等闪电首 tick 伤害窗口过后确认 zombie 未掉血。
    // maxTick=60：zombie 未被闪电命中 HP 恒 20；超时即 FAIL（范围判定失效误伤范围外实体）。
    pollUntilSucceed(test, () => {
        const hp = readHp(zombie);
        return hp >= 19 && hp <= 20;
    }, {
        startTick: 5,
        interval: 2,
        maxTick: 60,
        onTimeout: () => {
            test.assert(false,
                `lightning_out_of_range_no_damage: failed: zombie HP=${readHp(zombie)} `
                + `(expected 19..20; zombie at x=1 is 4 blocks from lightning at x=5, outside DAMAGE_RADIUS_XZ=3; `
                + `if HP<19 damage range check broken [over-sized AABB or no range filter])`);
        },
    });
}

export function registerLightningDamageTests(): void {
    // night batch：zombie 亡灵白天燃烧干扰，night 不燃 + killAllEntities 隔离自然刷怪。
    // maxTicks 100：闪电首 tick 伤害当 tick 完成，留 spawn + 轮询余量。
    GameTest.register("MobBehaviorTests", "lightning_direct_hit_deals_5_damage", lightningDirectHitDeals5Damage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(100);
    GameTest.register("MobBehaviorTests", "lightning_out_of_range_no_damage", lightningOutOfRangeNoDamage)
        .batch("night")
        .structureName("gametests:creeper_pit")
        .maxTicks(100);
}
