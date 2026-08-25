// 水瓶灭火分支行为类 GameTest（验证 PotionEntity::_onHitAsWater 的 extinguishFire 分支对齐 vanilla）。
//
// 验证 Cubium 喷溅药水（水瓶）命中破裂时，对范围内着火实体调用 extinguishFire 灭火，对齐 MC Java 1.21.11
// AbstractThrownPotion.onHitAsWater:100-102（着火且存活 → extinguishFire()）。
//
// C++ 链路（ProjectileItemEntity.cpp）：
//   _onHitAsWater（:656-701）：AABB inflate(4,2,4) 取范围内 LivingEntity，distSq<16 时：
//     - isWaterSensitive()==true → hurt(indirectMagic, 1.0)（已在 splash_water_bottle_damages_blaze 验证）
//     - isOnFire() && isAlive() → extinguishFire()（本测试验证此分支）
//   extinguishFire（Entity.hpp:1892）：若实体正在燃烧，播放灭火音效 + clearFire()（m_fire=0）。
//
// 前置能力（任务 #331）：Entity.setOnFire(seconds, useEffects?) 脚本绑定。
//   - setOnFire(10, true) → mc::Entity::igniteForSeconds(10)（×20=200 tick）→ FireComponent.m_fire=200。
//   - onFireTicksRemaining 组件（既有 readonly 绑定）读 m_fire → 200。
// 此前脚本层仅 readonly onFireTicksRemaining，无法主动点火，致灭火链路端到端测试不可构造。
//
// 防假通过设计（正反对照）：
//   - water_bottle_extinguishes_burning_entity：villager setOnFire(10) → 投水瓶命中 → _onHitAsWater
//     extinguishFire → onFireTicksRemaining 归零（≤1，容忍 1 tick 误差）。断言着火实体被灭火。
//   - burning_entity_not_extinguished_without_potion：villager setOnFire(10) 不投水瓶 → 自然燃烧衰减
//     但 tick 20 时仍燃烧（200 tick 远未耗尽）→ onFireTicksRemaining>10。断言未灭火。
//     若 setOnFire 绑定失效（m_fire 未设）：两测试 onFireTicksRemaining 均 0，本测试 FAIL，
//     暴露 water_bottle_extinguishes_burning_entity 的假通过风险（0→0 误判为灭火）。
//   两测试交叉验证：投水瓶灭火 vs 不投仍燃烧 = extinguishFire 链路由水瓶命中触发。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。
// villager 非水敏感（isWaterSensitive==false），故 _onHitAsWater 仅触发 extinguishFire 不 hurt，
// 隔离"水敏感伤害"干扰，专注验证灭火分支。
//
// 时序：
//   - tick 0：spawn villager + setOnFire(10)（m_fire=200）+ spawn 水瓶 + setVelocity 朝 villager 飞。
//   - tick 1：水瓶 tick 命中 villager → onImpact → _onHitAsWater → isOnFire→extinguishFire。
//   - tick 20：断言 onFireTicksRemaining≤1（灭火，容忍 1 tick 误差）。
//
// className 恒为 MobBehaviorTests。
// Ref: ProjectileItemEntity.cpp:656-701（_onHitAsWater：isOnFire→extinguishFire）
// Ref: Entity.hpp:1886-1894（clearFire / extinguishFire）
// Ref: AbstractThrownPotion.java:87-106（vanilla onHitAsWater：着火→extinguishFire）
// Ref: MinecraftModuleFactory.cpp Entity.setOnFire 绑定（任务 #331）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const SPLASH_POTION_TYPE = "minecraft:potion";
const VILLAGER_TYPE = "villager_v2";

const POTION_POS = { x: 3, y: 2, z: 3 };
const VILLAGER_POS = { x: 3, y: 2, z: 5 };

// 读取实体剩余火焰 tick（onfire 组件 onFireTicksRemaining）。无组件返 -1。
function readFireTicks(entity: unknown): number {
    const onfire = (entity as any)?.getComponent?.("minecraft:onfire") as
        | { onFireTicksRemaining?: number }
        | undefined;
    if (onfire == null) return -1;
    return onfire.onFireTicksRemaining ?? -1;
}

// 水瓶命中着火村民 → 灭火（验证 _onHitAsWater extinguishFire 分支）。
//
// villager setOnFire(10)→m_fire=200，水瓶 setVelocity 命中 → _onHitAsWater：
// isOnFire()==true && isAlive() → extinguishFire() → m_fire=0。
//
// 判定（tick 20）：onFireTicksRemaining≤1（灭火，容忍 1 tick 误差）。
//   - 若 _onHitAsWater 漏 extinguishFire 分支：m_fire 仍 200→超时 FAIL。
//   - 若 setOnFire 绑定失效：m_fire=0，setOnFire 后即 0，本测试从 0→0 假通过——
//     需与 burning_entity_not_extinguished_without_potion 配对（后者断言不投水瓶时仍燃烧>10，
//     若 setOnFire 失效则该对照 FAIL，暴露假通过）。
function waterBottleExtinguishesBurningEntity(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    // 点火 10 秒（200 tick）。villager 非水敏感，_onHitAsWater 仅触发灭火不 hurt。
    (villager as any).setOnFire(10, true);

    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);
    (potion as any).setVelocity({ x: 0, y: 0, z: 3.0 });

    pollUntilSucceed(test, () => {
        const fire = readFireTicks(villager);
        // 灭火后 isOnFire()=false → onfire 组件返 undefined（fireTicks=-1）；或残留 ≤1 tick。
        // 注：onfire 组件绑定门控 isOnFire（Entity.cpp:1593 仅着火时返回组件），故灭火后读不到组件
        // 即 fireTicks=-1 是灭火成功的正常表现。负向对照 burning_entity_not_extinguished_without_potion
        // 已证明 setOnFire 生效（villager 着火后 tick 20 仍燃烧>10），故此处 fire=-1 不可能是"从未着火"。
        return fire === -1 || (fire >= 0 && fire <= 1);
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const fire = readFireTicks(villager);
            test.assert(false,
                `water_bottle_extinguishes_burning_entity: failed: villager fireTicks=${fire} `
                + `(expected -1 [extinguished, isOnFire=false no onfire component] or <=1; `
                + `if fire=200 _onHitAsWater missing extinguishFire branch [isOnFire check or clearFire]; `
                + `if fire=-1 but setOnFire broken — pair with burning_entity_not_extinguished_without_potion [if that also fails, setOnFire binding broken])`);
        },
    });
}

// 着火村民不投水瓶 → 仍燃烧（负向对照，防 setOnFire 失效假通过）。
//
// villager setOnFire(10)→m_fire=200，不投水瓶。tick 20 时 m_fire≈180（每 tick -1 衰减）仍远>10。
// 断言 onFireTicksRemaining>10（仍燃烧）。
//   - 若 setOnFire 绑定失效：m_fire=0，fireTicks=0→FAIL，暴露 water_bottle_extinguishes 的假通过。
//   - 若 villager 因基岩版 fireImmune 不着火：同样 fireTicks=0→FAIL（villager 非亡灵非火焰免疫，可着火）。
function burningEntityNotExtinguishedWithoutPotion(test: Test): void {
    (test as any).killAllEntities();
    const villager = test.spawn(VILLAGER_TYPE, VILLAGER_POS);
    (villager as any).setOnFire(10, true);
    // 不投水瓶（对照：自然燃烧衰减，tick 20 时仍 >10）。

    pollUntilSucceed(test, () => {
        const fire = readFireTicks(villager);
        // 仍燃烧（m_fire 从 200 衰减，tick 20 约 180，远>10）。
        return fire > 10;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const fire = readFireTicks(villager);
            test.assert(false,
                `burning_entity_not_extinguished_without_potion: failed: villager fireTicks=${fire} `
                + `(expected >10 = still burning without potion; `
                + `if fire=0 setOnFire binding broken [m_fire not set] or villager fire-immune [unexpected]; `
                + `this is the negative control for water_bottle_extinguishes_burning_entity)`);
        },
    });
}

export function registerPotionExtinguishTests(): void {
    GameTest.register("MobBehaviorTests", "water_bottle_extinguishes_burning_entity", waterBottleExtinguishesBurningEntity)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "burning_entity_not_extinguished_without_potion", burningEntityNotExtinguishedWithoutPotion)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
