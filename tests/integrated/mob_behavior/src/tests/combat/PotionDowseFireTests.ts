// 水瓶浇灭方块分支行为类 GameTest（验证 PotionEntity::_dowseFire 浇灭营火/蜡烛对齐 vanilla AbstractThrownPotion）。
//
// 验证 Cubium 喷溅药水（水瓶）命中方块时，对命中面外侧+反向+四水平邻接的点燃营火/蜡烛调用 dowse 熄灭，
// 对齐 MC Java 1.21.11 AbstractThrownPotion.onHitBlock:50-67 + dowseFire:110-121。
// wiki（tech_喷溅药水.txt:177-179）：喷溅型水瓶扑灭命中及水平相邻的火、营火、灵魂营火，并熄灭点亮蜡烛。
//
// C++ 链路（ProjectileItemEntity.cpp）：
//   onBlockHit（:589-618）：先调基类 ProjectileEntity::onBlockHit（清速+通知方块 onProjectileHit），
//     再 isWaterBottle 时 dowseFire 命中面外侧 blockPos1 + blockPos1.offset(opposite) + 四水平邻接。
//     注：任务 #330 修复前 onImpact 未调基类 dispatch，致 onBlockHit（含 _dowseFire）成死代码。
//   _dowseFire（:620-654）三分支 if-else 链（顺序：FIRE→蜡烛→营火）：
//     - BlockTags::FIRE().contains → setBlockState(air)（火方块置空气）
//     - AbstractCandleBlock::isLit → extinguish（蜡烛 LIT=false + 熄灭音效）
//     - CampfireBlock::isLitCampfire → extinguish（营火 LIT=false + 熄灭音效）
//
// 任务 #332 修复的关键对齐缺陷：AbstractCandleBlock::isLit 此前缺 CANDLES/CANDLE_CAKES 标签门控，
// 营火（含 LIT 属性且 lit=true）被 isLit 误判为蜡烛，_dowseFire if-else 链中营火先命中 isLit(true)
// 进入蜡烛分支，dynamic_cast<AbstractCandleBlock*> 对营火返回 nullptr，extinguish 不执行，
// isLitCampfire 分支被 else if 跳过——营火浇灭成死代码。补标签门控后营火正确进入 isLitCampfire 分支。
// 本测试文件同时覆盖蜡烛分支（验证 isLit 标签门控后蜡烛浇灭正确）与营火分支。
//
// 几何推导（药水自上方下落命中矮方块顶面，营火/蜡烛均适用）：
//   creeper_pit 结构放置原点在地板下方一格：helper y=1 = 结构 y=0（grass_block 地板），
//   helper y=2 = 结构 y=1（air 腔），helper y=3 = 结构 y=2（air 腔）。
//   目标方块放 helper (3,2,5)（air 腔），下方 helper (3,1,5)=grass_block 顶面支撑。
//   营火碰撞盒 box(0,0,0,16,7,16)（高 7/16=0.4375），蜡烛碰撞盒更矮（约 3/16）。
//   药水从正上方 helper (3,3,5)（air）setVelocity({0,-2.0,0}) 下落，1 tick 射线
//   (3.5,3.0,5.5)→(3.5,1.0,5.5) 垂直穿过目标方块碰撞盒，命中顶面 hitFace。
//   （水平飞行方案不可靠：射线 y 与矮方块碰撞盒底面相切，raycast 边界判定易 miss；
//    且药水脚底贴 grass_block 顶面易先命中地板。垂直下落确保射线明确穿过矮方块碰撞盒。）
//
// 防假通过设计（正反对照）：
//   - water_bottle_dowse_campfire / water_bottle_dowse_candle：放点燃目标 + 投水瓶自上方命中 →
//     _dowseFire 浇灭 → lit=false。
//   - campfire_not_dowsed_without_potion / candle_not_dowsed_without_potion：放点燃目标不投水瓶 →
//     lit 仍 true（负向对照，防放置失效假通过）。
//   交叉验证：投水瓶浇灭 vs 不投仍 lit = _dowseFire 链路由水瓶命中触发。
//
// 火源选择：用 campfire[lit=true]（默认 lit=true）和 candle[lit=true]（BlockPermutation.resolve 放状态）
// 而非 minecraft:fire。营火/蜡烛 lit 是确定状态翻转，无 randomTick 自然熄灭噪声；
// minecraft:fire 走 randomTick 有概率自然熄灭引入时序噪声（TntTests.ts:24-26 警告火放置稳定性差）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）+ default batch。killAllEntities 清场。
// 目标方块 (3,2,5) 下方 (3,1,5) grass_block 顶面支撑。
//
// 时序：
//   - tick 0：放目标方块 + spawn 水瓶 (3,3,5) + setVelocity 朝 -Y 下落命中顶面。
//   - tick 1：水瓶 tick 命中目标 → onBlockHit → _dowseFire → extinguish → lit=false。
//   - tick 20：断言目标 lit===false（浇灭）。
//
// className 恒为 MobBehaviorTests。
// Ref: ProjectileItemEntity.cpp:589-618（onBlockHit：dowseFire blockPos1 + opposite + 四水平）
// Ref: ProjectileItemEntity.cpp:620-654（_dowseFire：FIRE/isLit(蜡烛)/isLitCampfire 三分支）
// Ref: AbstractCandleBlock.cpp isLit（三重门控 hasProperty(LIT)&&(CANDLES||CANDLE_CAKES)&&get(LIT)，任务 #332）
// Ref: AbstractThrownPotion.java:50-67,110-121（vanilla onHitBlock + dowseFire）
// Ref: tech_喷溅药水.txt:177-179（wiki：水瓶扑灭火/营火/灵魂营火，熄灭蜡烛）
// Ref: sweetBerryBush.ts（BlockPermutation.resolve 带 state 放置范式）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

const SPLASH_POTION_TYPE = "minecraft:potion";
// creeper_pit Y 偏移：结构放置原点在地板下方一格，故 helper y=1 = 结构 y=0（grass_block 地板），
// helper y=2 = 结构 y=1（air 腔），helper y=3 = 结构 y=2（air 腔）。
// 目标方块放 helper (3,2,5)（air 层），下方 helper (3,1,5) grass_block 顶面支撑；
// 药水 spawn helper (3,3,5)（air）朝 -Y 下落命中目标顶面。
const TARGET_POS = { x: 3, y: 2, z: 5 };
const POTION_POS = { x: 3, y: 3, z: 5 };

// 读取方块 lit state（boolean）。null 表示读取失败或无 lit 属性（如 air，getState 抛异常）。
// 用 try-catch 防 air 方块 getState("lit") 抛 "Cannot get property lit as it does not exist in minecraft:air"
// 致测试崩溃（方块放置失败/被浇灭变 air 后读取场景）。营火/蜡烛均有 lit 属性，非 air 时返回 boolean。
function getLitState(test: Test, pos: { x: number; y: number; z: number }): boolean | null {
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

// 放置点燃蜡烛（minecraft:candle，lit=true candles=1）于指定 helper 坐标。
// candle 默认 lit=false（CandleBlock.cpp:78），需用 BlockPermutation.resolve 带 state 放置 lit=true。
// 范式参考 sweetBerryBush.ts（两端统一 resolve + setBlockPermutation，any 绕过类型冲突）。
function placeLitCandle(test: Test, pos: { x: number; y: number; z: number }): void {
    const permutation = BlockPermutation.resolve("minecraft:candle", { lit: true, candles: 1 }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: { x: number; y: number; z: number }) => void;
    }).setBlockPermutation(permutation, pos);
}

// 水瓶自上方下落命中点燃营火 → 浇灭（验证 _dowseFire CampfireBlock 分支）。
//
// 营火 (3,2,5) lit=true，药水从 (3,3,5) setVelocity({0,-2.0,0}) 下落命中营火顶面 →
// onBlockHit → _dowseFire(营火) → isLitCampfire→extinguish → lit=false。
// 注：任务 #332 修复 isLit 标签门控前，营火误入蜡烛分支致此测试 FAIL；修复后正确进入营火分支。
//
// 判定（tick 20）：营火 lit===false（浇灭）。
//   - 若 _dowseFire 漏 CampfireBlock 分支（或 isLit 误判营火为蜡烛抢占分支）：lit 仍 true→超时 FAIL。
//   - 若 onBlockHit 未调 _dowseFire（onImpact 未 dispatch）：lit 仍 true→超时 FAIL。
//   - 若药水未命中营火（下落射线偏移）：lit 仍 true→超时 FAIL。
//   - 若 setBlockType 放营火失效：前置断言 lit===true 即 FAIL（无需轮询）。
function waterBottleDowseCampfire(test: Test): void {
    (test as any).killAllEntities();
    // 放点燃营火（minecraft:campfire 默认 lit=true）于 grass_block 顶面（helper y=2 air 层）。
    test.setBlockType("minecraft:campfire", TARGET_POS);
    // 前置断言：营火已放置且 lit=true（防放置失效假通过，getState 异常时 lit=null 断言失败）。
    test.assert(getLitState(test, TARGET_POS) === true,
        `campfire lit should be true before potion, got ${getLitState(test, TARGET_POS)}`);

    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);
    // 朝 -Y 下落 2.0/tick，1 tick 射线 (3.5,3.0,5.5)→(3.5,1.0,5.5) 垂直穿过营火碰撞盒 y∈[2.0,2.4375]。
    (potion as any).setVelocity({ x: 0, y: -2.0, z: 0 });

    pollUntilSucceed(test, () => {
        return getLitState(test, TARGET_POS) === false;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const lit = getLitState(test, TARGET_POS);
            test.assert(false,
                `water_bottle_dowse_campfire: failed: campfire lit=${lit} `
                + `(expected false = dowsed by _dowseFire; `
                + `if lit=true _dowseFire missing CampfireBlock branch [isLitCampfire check] or isLit misjudged campfire as candle [CANDLES/CANDLE_CAKES tag gating missing, task #332] or onBlockHit not dispatching _dowseFire [onImpact base dispatch broken] or potion missed campfire [vertical ray missed low campfire collision box]; `
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
    test.setBlockType("minecraft:campfire", TARGET_POS);
    test.assert(getLitState(test, TARGET_POS) === true,
        `campfire lit should be true before, got ${getLitState(test, TARGET_POS)}`);
    // 不投水瓶（对照：营火无 randomTick 熄灭，lit 保持 true）。

    pollUntilSucceed(test, () => {
        return getLitState(test, TARGET_POS) === true;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const lit = getLitState(test, TARGET_POS);
            test.assert(false,
                `campfire_not_dowsed_without_potion: failed: campfire lit=${lit} `
                + `(expected true = still lit without potion; `
                + `if lit=false campfire self-extinguished [unexpected, no randomTick] or placement lost; `
                + `this is the negative control for water_bottle_dowse_campfire)`);
        },
    });
}

// 水瓶自上方下落命中点燃蜡烛 → 浇灭（验证 _dowseFire 蜡烛分支 AbstractCandleBlock::isLit）。
//
// 蜡烛 (3,2,5) lit=true（BlockPermutation.resolve 放置 lit=true candles=1），药水从 (3,3,5)
// setVelocity({0,-2.0,0}) 下落命中蜡烛顶面 → onBlockHit → _dowseFire(蜡烛) →
// AbstractCandleBlock::isLit(蜡烛)==true（CANDLES 标签门控通过）→ extinguish → lit=false。
//
// 此测试独立验证 isLit 标签门控修复后蜡烛分支正确工作：任务 #332 修复前营火误入蜡烛分支掩盖了
// 蜡烛分支本身的正确性，修复后须确认真蜡烛仍被正确浇灭（isLit 对蜡烛返回 true，extinguish 执行）。
//
// 判定（tick 20）：蜡烛 lit===false（浇灭）。
//   - 若 _dowseFire 漏蜡烛分支：lit 仍 true→超时 FAIL。
//   - 若 isLit 标签门控误把蜡烛判为非蜡烛（CANDLES 标签查询失效）：lit 仍 true→超时 FAIL。
//   - 若 placeLitCandle 放置失效：前置断言 lit===true 即 FAIL。
function waterBottleDowseCandle(test: Test): void {
    (test as any).killAllEntities();
    placeLitCandle(test, TARGET_POS);
    // 前置断言：蜡烛已放置且 lit=true（防放置失效假通过）。
    test.assert(getLitState(test, TARGET_POS) === true,
        `candle lit should be true before potion, got ${getLitState(test, TARGET_POS)}`);

    const potion = test.spawn(SPLASH_POTION_TYPE, POTION_POS);
    // 朝 -Y 下落 2.0/tick 垂直穿过蜡烛碰撞盒（蜡烛约 3/16 高，比营火更矮，垂直下落仍可靠命中）。
    (potion as any).setVelocity({ x: 0, y: -2.0, z: 0 });

    pollUntilSucceed(test, () => {
        return getLitState(test, TARGET_POS) === false;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 70,
        onTimeout: () => {
            const lit = getLitState(test, TARGET_POS);
            test.assert(false,
                `water_bottle_dowse_candle: failed: candle lit=${lit} `
                + `(expected false = dowsed by _dowseFire; `
                + `if lit=true _dowseFire missing candle branch [isLit check] or isLit tag gating misjudged candle as non-candle [CANDLES tag query broken] or potion missed candle [vertical ray missed low candle collision box]; `
                + `if lit=null candle became air or placement lost)`);
        },
    });
}

// 点燃蜡烛不投水瓶 → 仍 lit=true（负向对照，防放置失效假通过）。
//
// 蜡烛 (3,2,5) lit=true，不投水瓶。蜡烛无 randomTick 自然熄灭，lit 保持 true。
// 断言 lit===true（仍点燃）。
//   - 若 placeLitCandle 放置失效：前置断言 lit===true 即 FAIL。
//   - 此为 water_bottle_dowse_candle 的负向对照，确认"浇灭"由水瓶命中触发而非放置即熄灭。
function candleNotDowsedWithoutPotion(test: Test): void {
    (test as any).killAllEntities();
    placeLitCandle(test, TARGET_POS);
    test.assert(getLitState(test, TARGET_POS) === true,
        `candle lit should be true before, got ${getLitState(test, TARGET_POS)}`);
    // 不投水瓶（对照：蜡烛无 randomTick 熄灭，lit 保持 true）。

    pollUntilSucceed(test, () => {
        return getLitState(test, TARGET_POS) === true;
    }, {
        startTick: 20,
        interval: 5,
        maxTick: 50,
        onTimeout: () => {
            const lit = getLitState(test, TARGET_POS);
            test.assert(false,
                `candle_not_dowsed_without_potion: failed: candle lit=${lit} `
                + `(expected true = still lit without potion; `
                + `if lit=false candle self-extinguished [unexpected, no randomTick] or placement lost; `
                + `this is the negative control for water_bottle_dowse_candle)`);
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

    GameTest.register("MobBehaviorTests", "water_bottle_dowse_candle", waterBottleDowseCandle)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(120);

    GameTest.register("MobBehaviorTests", "candle_not_dowsed_without_potion", candleNotDowsedWithoutPotion)
        .batch("default")
        .structureName("gametests:creeper_pit")
        .maxTicks(90);
}
