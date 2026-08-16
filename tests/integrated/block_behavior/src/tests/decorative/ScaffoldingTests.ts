// 脚手架延展距离与悬空自毁行为 GameTest。
//
// wiki tech_脚手架.txt（:59-64）：脚手架计算水平延展距离：
//   - 直接放置在顶面支撑面完整的实心方块上时，延展距离为 0。
//   - 否则，延展距离等于正下方 1 格脚手架的延展距离，或水平相邻脚手架延展距离加 1，取最小可能值。
//   - 延展距离超过 6 格（即 =7）的脚手架会直接破坏并掉落物品。
// distance state（0-7）：0=直接支撑，7=过远需掉落。
//
// C++ 链路：ScaffoldingBlock 有 distance state（DISTANCE_0_7，0-7）。
//   - calculateDistance（ScaffoldingBlock.cpp:295-332）：下方方块 isSolidSide(Up) true → 0；下方是脚手架
//     → 继承其 distance；下方水平邻居脚手架 → min(邻居 distance+1)；否则 7。
//   - onBlockAdded（:163-168）/updatePostPlacement（:141-161）调度 tick(1, Normal)。
//   - tick（:170-249）：重算 distance + bottom。distance==7 且 previousDistance==7 → 直接破坏掉物品；
//     distance==7 且 previousDistance!=7 → 生成 FallingBlockEntity；否则更新 state（distance/bottom）。
//   - isValidPosition（:251-256）：calculateDistance < 7 才可放置（强放绕过）。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（distance=7, bottom=false），不经
// isValidPosition，故即使 distance=7（悬空）也能强放。onBlockAdded 调度 tick(1)，tick 内重算 distance
// 决定稳定（更新 state）或自毁（distance=7）。走 tick 延迟路径（非同步），需 pollUntilSucceed 轮询。
//
// 测试覆盖（2 个场景，行为与 vanilla 一致，可跨服务端对比）：
//   1. 脚手架直接放 stone 上方 → tick 后 distance=0（稳定，不自毁）。
//   2. 脚手架空悬（下方 air，无邻居脚手架）→ distance=7 → 自毁消失。
//
// 关键约束：
// 1. distance 重算走 tick(1) 延迟（onBlockAdded→scheduleBlockTick），非 onBlockAdded 同步。故用
//    pollUntilSucceed 轮询 distance/blocks，留 tick 余量（startTick=5, maxTick=60）。
// 2. 场景 2 悬空自毁：scaffolding 默认 distance=7，tick 里 previousDistance==7（default）→ 走「直接
//    破坏掉物品」分支（不生成 FallingBlockEntity），scaffolding 变 air。succeedWhenBlockPresent(false)
//    轮询消失。
//
// 不测「水平延展 6 格边界」：需铺设 7+ 格脚手架链 + 支撑，布局复杂，且 distance 递增逻辑已被场景 1
// （distance=0）覆盖核心 calculateDistance 路径。TODO: 可补 scaffolding_horizontal_spread_distance 测试。
// 不测「下落实体（FallingBlockEntity）生成」：场景 2 走直接破坏分支（previousDistance==7），不生成
// 下落实体；下落实体分支需 previousDistance!=7 → distance=7（如支撑被移除使稳定脚手架变悬空），
// 布局复杂且涉及实体生成，跳过。
// 不测「脚手架攀爬/碰撞」：依赖实体碰撞箱 + isLadder/getCollisionShape，属实体行为，跳过。
//
// 跨服务端：脚手架 distance state 名两端一致（Java 式 int 0-7），延展距离计算与自毁行为与 vanilla
// 一致（distance=7 即破坏），可跨服务端对比。getState("distance") 用 as any 绕过 BlockStateSuperset
// 白名单（同栅栏/树叶范式）。注意脚手架自毁走 tick 延迟，跨服务端对比时 tick 容忍度需放宽。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_脚手架.txt#下落的方块（延展距离 0-6，=7 破坏掉落）
// Ref: ScaffoldingBlock.cpp（calculateDistance/tick/onBlockAdded/isValidPosition）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 取脚手架 distance state（number 0-7）。返回 null 表示读取失败。
function getScaffoldingDistance(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("distance" as any);
    return typeof value === "number" ? value : null;
}

// 脚手架直接放 stone 上方 → tick 后 distance=0（稳定）。
// calculateDistance：下方 stone isSolidSide(Up) true → distance=0。tick 重算后 setBlockState(distance=0)。
// Ref: tech_脚手架.txt#下落的方块（直接放支撑面完整实心方块上 distance=0）
function scaffoldingDistanceZeroOnSolidSupport(test: Test): void {
    // (3,1,1) 铺 stone 作脚手架下方支撑（isSolidSide(Up) true，calculateDistance 返回 0）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });

    // (3,2,1) 放脚手架（在 stone 上，强放绕过 isValidPosition，default distance=7）。onBlockAdded →
    // tick(1) → calculateDistance：下方 stone isSolidSide(Up) true → distance=0 → setBlockState(distance=0)。
    test.setBlockType("minecraft:scaffolding", { x: 3, y: 2, z: 1 });

    // 轮询断言 distance === 0。tick(1) 延迟重算，pollUntilSucceed 留余量。
    pollUntilSucceed(
        test,
        () => getScaffoldingDistance(test, 3, 2, 1) === 0,
        {
            startTick: 5,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `scaffolding distance: should be 0, got ${getScaffoldingDistance(test, 3, 2, 1)}`,
                );
            },
        },
    );
}

// 脚手架空悬（下方 air，无邻居脚手架）→ distance=7 → 自毁消失。
// calculateDistance：下方 air（非脚手架、isSolidSide(Up) false）→ minDistance=7；下方水平邻居无脚手架
// → distance=7。tick 里 previousDistance==7（default）→ 直接破坏掉物品分支 → scaffolding 变 air。
// Ref: tech_脚手架.txt#下落的方块（延展距离 >6 即 =7 直接破坏掉落）
function scaffoldingFallsWhenNoSupport(test: Test): void {
    // (3,2,1) 放脚手架（下方 (3,1,1) air，无水平邻居脚手架，强放绕过 isValidPosition，default distance=7）。
    // onBlockAdded → tick(1) → calculateDistance：下方 air → 7；水平邻居 air → 7 → distance=7。
    // previousDistance==7（default）→ 直接破坏掉物品分支 → scaffolding 变 air。
    test.setBlockType("minecraft:scaffolding", { x: 3, y: 2, z: 1 });

    // 断言脚手架格 (3,2,1) 脚手架已自毁消失（tick(1) 延迟，succeedWhenBlockPresent 轮询）。
    test.succeedWhenBlockPresent("minecraft:scaffolding", { x: 3, y: 2, z: 1 }, false);
}

export function registerScaffoldingTests(): void {
    GameTest.register("BlockBehaviorTests", "scaffolding_distance_zero_on_solid_support", scaffoldingDistanceZeroOnSolidSupport)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "scaffolding_falls_when_no_support", scaffoldingFallsWhenNoSupport)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
