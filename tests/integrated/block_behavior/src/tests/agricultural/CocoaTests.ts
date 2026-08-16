// 可可果骨粉催熟与支撑自毁行为 GameTest。
//
// wiki tech_可可豆.txt（:57-63）：可可果有三个生长阶段（age 0/1/2，maxAge=2）。"骨粉能够将可可果
// 催熟到下一生长阶段"（:63）。"若生长可可果的原木或木头被移除，可可果也会掉落"（:63 支撑自毁）。
//
// C++ 链路：
//   - 骨粉催熟：CocoaBlock 实现 IGrowable。canUseBonemeal（CocoaBlock.cpp:193-202）恒返回 true（100%
//     即时生效，非概率）。grow（:204-214）当 age < maxAge 时 withAge(age+1)，无随机（MC_UNUSED(random)）。
//     故骨粉 age 0→1、1→2 确定性可测。canGrow 未成熟返回 true（:183-191）。
//   - 支撑自毁：CocoaBlock::updatePostPlacement（:134-158）当 facing==attachDir（即 FACING，指向
//     丛林原木方向）且 _canAttachTo（:284-295，检查 FACING 方向方块是否 ∈ JUNGLE_LOGS 标签）失败时
//     返回 air 自毁。故移除 FACING 方向的丛林原木 → 可可果自毁。反应同 tick 同步。
//
// 放置语义：setBlockType 走 _resolveBlock 取 defaultState（facing=North, age=0），不经
// getStateForPlacement。CocoaBlock::getStateForPlacement（:109-126）会遍历玩家朝向找丛林原木侧面
// 自动设 facing，但 setBlockType 不走此路径。故需用 setBlockWithStates 显式设 facing=east 放可可果
// （FACING=East 表示可可豆朝东，附在 East 邻位丛林原木上）。
//
// 测试布局（East 附着范式）：
//   - (4,1,1) 放 jungle_log（East 邻位，∈ JUNGLE_LOGS 标签，作可可豆 FACING=East 的附着面）。
//   - (3,1,1) 用 setBlockWithStates 放 facing=east 的可可果（附在 East 邻位丛林原木上）。
//
// 关键约束（同支撑自毁范式）：
// 1. 先放 jungle_log 再放可可果，保证可可果放置时 FACING=East 方向有附着（贴近 vanilla 放置语义）。
//    放置不向自身派发 updatePostPlacement，facing=east 被保留。
// 2. 骨粉测试：SimulatedPlayer 持骨粉对 (3,1,1) 可可果 useItemOnBlock → grow age+1 同步写回，
//    立即可读 age。
// 3. 支撑自毁测试：移除 (4,1,1) jungle_log（→air）必须是非 no-op 写入以派发更新。air 放置向 West
//    邻位可可果派发 updatePostPlacement(East) → facing==East==attachDir → _canAttachTo(East)=air 非
//    JUNGLE_LOGS → 返回 air，可可果自毁。
//
// 不测 randomTick 生长（:160-181，1/5 概率 + 光照阈值）：概率性，跳过。
// 不测「可可果接触水掉落」：依赖水流动/含水体系，复杂且非本文件核心行为点，跳过。
//
// 跨服务端：可可果 age state 名两端一致（age 0-2），facing state 名两端一致（facing north/south/
// east/west），骨粉 +1 age 与支撑自毁行为与 vanilla 一致（同步），可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_可可豆.txt（骨粉催熟下一阶段 + 原木移除掉落）
// Ref: CocoaBlock.cpp（grow/canUseBonemeal/updatePostPlacement/_canAttachTo）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 铺设可可果 East 附着布局：(4,1,1) jungle_log + (3,1,1) facing=east 可可果（指定 age）。
// 返回可可果位置供后续骨粉/断言使用。
function placeCocoaSetup(test: Test, age: number): void {
    // (4,1,1) 放 jungle_log（East 邻位，∈ JUNGLE_LOGS，作可可豆 FACING=East 附着面）。
    test.setBlockType("minecraft:jungle_log", { x: 4, y: 1, z: 1 });

    // (3,1,1) 用 setBlockWithStates 放 facing=east + age 的可可果。setBlockType 只放 defaultState
    // （facing=North），需 setBlockWithStates 显式设 facing=east（附在 East 丛林原木上）。
    test.setBlockWithStates("minecraft:cocoa", { x: 3, y: 1, z: 1 }, `facing=east,age=${age}`);
}

// 取可可果 age state（number）。返回 null 表示读取失败。
function getCocoaAge(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("age");
    return typeof value === "number" ? value : null;
}

// 骨粉催熟可可果：age=0 → age=1（验证 grow +1 age 链路）。
// canUseBonemeal 恒 true，grow age<maxAge 时 age+1（无随机），确定性。
// Ref: tech_可可豆.txt#种植（骨粉催熟下一阶段）
function bonemealGrowsCocoa(test: Test): void {
    const cocoaPos = { x: 3, y: 1, z: 1 };

    // 铺设 age=0 可可果布局（jungle_log + facing=east cocoa age=0）。
    placeCocoaSetup(test, 0);

    // SimulatedPlayer 持骨粉对可可果使用（direction=Up，从上方使用，同 CropBoneMealTests 范式）。
    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        cocoaPos,
        Direction.Up,
    );
    test.assert(used, "cocoa bonemeal: useItemOnBlock should return true");

    // 判定：骨粉 grow 同步 setBlockState，立即可读 age===1。
    pollUntilSucceed(
        test,
        () => getCocoaAge(test, 3, 1, 1) === 1,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `cocoa bonemeal: age should be 1, got ${getCocoaAge(test, 3, 1, 1)}`,
                );
            },
        },
    );
}

// 骨粉催熟可可豆到成熟：age=1 → age=2（maxAge=2，验证第二阶段催熟）。
// 与 bonemealGrowsCocoa 互补，覆盖 age=1→2 分支。
function bonemealGrowsCocoaToMaxAge(test: Test): void {
    const cocoaPos = { x: 3, y: 1, z: 1 };

    // 铺设 age=1 可可果布局。
    placeCocoaSetup(test, 1);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        cocoaPos,
        Direction.Up,
    );
    test.assert(used, "cocoa bonemeal maxage: useItemOnBlock should return true");

    // 判定：age 1→2（maxAge=2）。
    pollUntilSucceed(
        test,
        () => getCocoaAge(test, 3, 1, 1) === 2,
        {
            startTick: 5,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(
                    false,
                    `cocoa bonemeal maxage: age should be 2, got ${getCocoaAge(test, 3, 1, 1)}`,
                );
            },
        },
    );
}

// 移除可可果所附着的 East 邻位丛林原木时可可果自毁变 air。
// updatePostPlacement(East)：facing==East==attachDir → _canAttachTo(East)=air 非 JUNGLE_LOGS → 返回 air。
// Ref: tech_可可豆.txt#种植（原木被移除可可果掉落）
function cocoaBreaksWhenAttachedLogRemoved(test: Test): void {
    // 铺设 age=0 可可果布局（jungle_log + facing=east cocoa）。
    placeCocoaSetup(test, 0);

    // (4,1,1) 设 air 移除丛林原木（jungle_log→air 真实状态变化，非 no-op，派发邻居更新）。air 放置
    // 向 West 邻位可可果派发 updatePostPlacement(East) → facing==East==attachDir → _canAttachTo(East)=
    // air 非 JUNGLE_LOGS → 返回 air，可可果自毁。
    test.setBlockType("minecraft:air", { x: 4, y: 1, z: 1 });

    // 断言可可果格 (3,1,1) 可可果已自毁消失（同 tick 同步）。
    test.succeedWhenBlockPresent("minecraft:cocoa", { x: 3, y: 1, z: 1 }, false);
}

export function registerCocoaTests(): void {
    GameTest.register("BlockBehaviorTests", "bonemeal_grows_cocoa", bonemealGrowsCocoa)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "bonemeal_grows_cocoa_to_max_age", bonemealGrowsCocoaToMaxAge)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "cocoa_breaks_when_attached_log_removed", cocoaBreaksWhenAttachedLogRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
