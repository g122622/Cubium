// 水瓶浇灭方块分支行为类 GameTest（验证 PotionEntity::_dowseFire 浇灭营火对齐 vanilla AbstractThrownPotion）。
//
// 验证 Cubium 喷溅药水（水瓶）命中方块时，对命中面外侧+反向+四水平邻接的点燃营火调用 dowse 熄灭，
// 对齐 MC Java 1.21.11 AbstractThrownPotion.onHitBlock:50-67 + dowseFire:110-121。
//
// C++ 链路（ProjectileItemEntity.cpp）：
//   onBlockHit（:589-618）：先调基类 ProjectileEntity::onBlockHit（清速+通知方块 onProjectileHit），
//     再 isWaterBottle 时 dowseFire 命中面外侧 blockPos1 + blockPos1.offset(opposite) + 四水平邻接。
//     注：任务 #330 修复前 onImpact 未调基类 dispatch，致 onBlockHit（含 _dowseFire）成死代码。
//   _dowseFire（:620-654）：FIRE tag→setBlockState(air)；AbstractCandleBlock isLit→extinguish；
//     CampfireBlock isLitCampfire→extinguish（setBlock LIT=false + 熄灭音效）。
//
// 几何推导（药水自上方下落命中营火顶面）：
//   creeper_pit 结构放置原点在地板下方一格：helper y=1 = 结构 y=0（grass_block 地板），
//   helper y=2 = 结构 y=1（air 腔），helper y=3 = 结构 y=2（air 腔）。
//   营火放 helper (3,2,5)（air 腔），下方 helper (3,1,5)=grass_block 顶面支撑。
//   营火碰撞盒 box(0,0,0,16,7,16)（高 7/16=0.4375），世界 y∈[2.0, 2.4375]。
//   药水从正上方 helper (3,3,5)（air）setVelocity({0,-2.0,0}) 下落，1 tick 射线
//   (3.5,3.0,5.5)→(3.5,1.0,5.5) 垂直穿过营火碰撞盒 y∈[2.0,2.4375]，命中营火顶面 hitFace=Down。
//   （水平飞行方案不可靠：射线 y=2.0 与营火碰撞盒底面 y=2.0 相切，raycast 边界判定易 miss；
//    且药水脚底贴 grass_block 顶面易先命中地板。垂直下落确保射线明确穿过矮营火碰撞盒。）
//   blockPos1 = result.blockPos.offset(Down) = (3,1,5)（grass_block，无火）。
//   _dowseFire(3,1,5)（grass_block，FIRE/isLit/isLitCampfire 均否，no-op）；
//   _dowseFire(blockPos1.offset(opposite=Up)) = _dowseFire(3,2,5)（即营火自身位置）→
//     CampfireBlock::isLitCampfire==true → extinguish → lit=false。
//   四水平 _dowseFire(2,1,5)/(4,1,5)/(3,1,4)/(3,1,6) 均 grass_block 无火。
//   结论：营火 (3,2,5) 被 _dowseFire 浇灭 lit→false。
//
// 防假通过设计（正反对照）：
//   - water_bottle_dowse_campfire：放点燃营火 + 投水瓶自上方命中 → _dowseFire 浇灭 → lit=false。
//   - campfire_not_dowsed_without_potion：放点燃营火不投水瓶 → lit 仍 true（负向对照，防放置失效假通过）。
//   两测试交叉验证：投水瓶浇灭 vs 不投仍 lit = _dowseFire 链路由水瓶命中触发。
//
// 火源选择：用 campfire[lit=true] 而非 minecraft:fire。营火 lit 是确定状态翻转，无 randomTick 自然熄灭
// 噪声（营火仅被水浇/水瓶/斧头熄灭）；minecraft:fire 走 randomTick 有概率自然熄灭引入时序噪声
// （TntTests.ts:24-26 警告火放置稳定性差）。setBlockType 放默认 campfire 即 lit=true（ShovelTests 范式）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。
// 营火 (3,2,5) 下方 (3,1,5) grass_block 顶面支撑。
//
// 时序：
//   - tick 0：放营火 (3,2,5) + spawn 水瓶 (3,3,5) + setVelocity 朝 -Y 下落命中营火顶面。
//   - tick 1：水瓶 tick 命中营火 → onBlockHit → _dowseFire(3,2,5) → extinguish → lit=false。
//   - tick 20：断言营火 lit===false（浇灭）。
//
// className 恒为 MobBehaviorTests。
// Ref: ProjectileItemEntity.cpp:589-618（onBlockHit：dowseFire blockPos1 + opposite + 四水平）
// Ref: ProjectileItemEntity.cpp:620-654（_dowseFire：CampfireBlock isLitCampfire→extinguish）
// Ref: AbstractThrownPotion.java:50-67,110-121（vanilla onHitBlock + dowseFire）
// Ref: ShovelTests.ts:63-70（getCampfireLit getState("lit") 范式）
// Ref: MinecraftModuleFactory.cpp Entity.setVelocity 绑定（任务 #324）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const SPLASH_POTION_TYPE = "minecraft:potion";
// creeper_pit Y 偏移：结构放置原点在地板下方一格，故 helper y=1 = 结构 y=0（grass_block 地板），
// helper y=2 = 结构 y=1（air 腔），helper y=3 = 结构 y=2（air 腔）。
// 营火放 helper (3,2,5)（air 层），下方 helper (3,1,5) grass_block 顶面支撑；
// 药水 spawn helper (3,3,5)（air）朝 -Y 下落命中营火顶面。
const CAMPFIRE_POS = { x: 3, y: 2, z: 5 };
const POTION_POS = { x: 3, y: 3, z: 5 };

// 读取营火 lit state（boolean）。null 表示读取失败或非营火（如 air，getState 抛异常）。
// 用 try-catch 防 air 方块 getState("lit") 抛 "Cannot get property lit as it does not exist in minecraft:air"
// 致测试崩溃（营火放置失败/被浇灭变 air 后读取场景）。
function getCampfireLit(test: Test, pos: { x: number; y: number; z: number }): boolean | null {
    const block = test.getBlock(pos);
    if (block === undefined) {
        return null;
    }
    try {
        const value = block?.permutation?.getState("lit" as any);
        return typeof value === "boolean" ? value : null;
    } catch {
        return null;
    }
}

// 水瓶自上方下落命中点燃营火 → 浇灭（验证 _dowseFire CampfireBlock 分支）。
//
// 营火 (3,2,5) lit=true，药水从 (3,3,5) setVelocity({0,-2.0,0}) 下落命中营火顶面 →
// onBlockHit → blockPos1=(3,1,5)（命中面外侧 grass_block），_dowseFire(blockPos1.offset(Up))=
// _dowseFire(3,2,5)（营火自身）→ isLitCampfire→extinguish → lit=false。
//
// 判定（tick 20）：营火 lit===false（浇灭）。
//   - 若 _dowseFire 漏 CampfireBlock 分支：lit 仍 true→超时 FAIL。
//   - 若 onBlockHit 未调 _dowseFire（onImpact 未 dispatch）：lit 仍 true→超时 FAIL。
//   - 若药水未命中营火（下落射线偏移）：lit 仍 true→超时 FAIL。
//   - 若 setBlockType 放营火失效：前置断言 lit===true 即 FAIL（无需轮询）。
function waterBottleDowseCampfire(test: Test): void {
    (test as any).killAllEntities();
    // 放点燃营火（minecraft:campfire 默认 lit=true）于 grass_block 顶面（helper y=2 air 层）。
    test.setBlockType("minecraft:campfire", CAMPFIRE_POS);
    // 前置断言：营火已放置且 lit=true（防放置失效假通过，getState 异常时 lit=null 断言失败）。
    test.assert(getCampfireLit(test, CAMPFIRE_POS) === true,
        `campfire lit should be true before potion, got ${getCampfireLit(test, CAMPFIRE_POS)}`);

    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);
    // 朝 -Y 下落 2.0/tick，1 tick 射线 (3.5,3.0,5.5)→(3.5,1.0,5.5) 垂直穿过营火碰撞盒 y∈[2.0,2.4375]。
    (potion as any).setVelocity({ x: 0, y: -2.0, z: 0 });

    pollUntilSucceed(test, () => {
        return getCampfireLit(test, CAMPFIRE_POS) === false;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const lit = getCampfireLit(test, CAMPFIRE_POS);
            test.assert(false,
                `water_bottle_dowse_campfire: failed: campfire lit=${lit} `
                + `(expected false = dowsed by _dowseFire; `
                + `if lit=true _dowseFire missing CampfireBlock branch [isLitCampfire check] or onBlockHit not dispatching _dowseFire [onImpact base dispatch broken] or potion missed campfire [vertical ray missed low campfire collision box]; `
                + `if lit=null campfire became air [dowsed to air not lit=false] or placement lost)`);
        },
    });
}

// 点燃营火不投水瓶 → 仍 lit=true（负向对照，防放置失效假通过）。
//
// 营火 (3,2,5) lit=true，不投水瓶。营火无 randomTick 自然熄灭，lit 保持 true。
// 断言 lit===true（仍点燃）。
//   - 若 setBlockType 放营火失效：前置断言 lit===true 即 FAIL。
//   - 此为 water_bottle_dowse_campfire 的负向对照，确认"浇灭"由水瓶命中触发而非放置即熄灭。
function campfireNotDowsedWithoutPotion(test: Test): void {
    (test as any).killAllEntities();
    test.setBlockType("minecraft:campfire", CAMPFIRE_POS);
    test.assert(getCampfireLit(test, CAMPFIRE_POS) === true,
        `campfire lit should be true before, got ${getCampfireLit(test, CAMPFIRE_POS)}`);
    // 不投水瓶（对照：营火无 randomTick 熄灭，lit 保持 true）。

    pollUntilSucceed(test, () => {
        return getCampfireLit(test, CAMPFIRE_POS) === true;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const lit = getCampfireLit(test, CAMPFIRE_POS);
            test.assert(false,
                `campfire_not_dowsed_without_potion: failed: campfire lit=${lit} `
                + `(expected true = still lit without potion; `
                + `if lit=false campfire self-extinguished [unexpected, no randomTick] or placement lost; `
                + `this is the negative control for water_bottle_dowse_campfire)`);
        },
    });
}

export function registerPotionDowseFireTests(): void {
    GameTest.register("MobBehaviorTests", "water_bottle_dowse_campfire", waterBottleDowseCampfire)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "campfire_not_dowsed_without_potion", campfireNotDowsedWithoutPotion)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
