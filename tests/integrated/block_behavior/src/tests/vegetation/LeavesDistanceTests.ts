// 树叶 distance 传播行为 GameTest（确定性的距离判定，非枯萎）。
//
// 树叶有两个相关机制：(1) distance 传播——树叶据距原木的距离设 distance state（1-7）；(2) 枯萎——
// distance==7 且 persistent=false 的树叶在 randomTick 概率性消失。本文件只测 (1) distance 传播
// （确定性 scheduledTick 路径），不测 (2) 枯萎（randomTick 概率性，非确定，不强测）。
//
// distance 传播链路（LeavesBlock.cpp）：
//   - 邻居方块变化触发 LeavesBlock::updatePostPlacement（:87-112），计算 neighborDistance=_getDistance
//     (facingState)+1，若距离变化 scheduleBlockTick(currentPos, *this, 1)（:103，1 tick 延迟）。
//   - 1 tick 后 tick()（:114-124）调 _updateDistance（:158-178）重算：六向邻居取
//     min(_getDistance(neighbor)+1)，邻居是原木(LOGS tag)记 0（故原木邻接树叶 distance=1）。
//   - _getDistance（:180-195）：原木→0、树叶→其 distance、其他→7。
//   全程无随机数（tick 的 random 参数被 MC_UNUSED），确定性 scheduledTick。
//
// 放置语义：setBlockType("minecraft:oak_leaves") 走 GameTestHelper::setBlock → _resolveBlock 取
// defaultState（distance=7, persistent=false），不经 getStateForPlacement（故 persistent=false，
// 区别于玩家手放）。setBlockState flags=3 触发邻居 updatePostPlacement → 1 tick 后 distance 收敛。
//
// 测试覆盖：
//   1. 原木邻接树叶 → 树叶 distance=1（原木记 0，+1=1）。
//   2. 叶链（原木-叶-叶-叶）→ 各叶 distance=1/2/3（多跳传播）。
//   3. 孤立树叶（周围无原木无叶）→ distance 保持 7（无原木邻居，min 始终 7）。
//
// 枯萎（randomTick 概率消失）与 persistent 门测试不写：randomTick 由 tickEnvironment 概率性调度
// （ServerWorld.cpp:1308-1375 每 tick 在 chunk section 随机选位），非确定；persistent=true 不枯萎
// 需 randomTick 命中才区分，确定性范围内无法验证。按「随机性行为不强测」准则跳过。
// TODO: 待需要时可用高 randomTickSpeed + maxTicks 概率性测枯萎，但非确定性。
//
// 跨服务端：distance state 名两端均为 distance（DISTANCE_1_7），值域 1-7 一致，传播机制两端一致，
// 可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_树叶.txt（树叶距离判定）
// Ref: LeavesBlock.cpp（_updateDistance/_getDistance/updatePostPlacement/tick）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass/cobblestone 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。

// 取指定坐标树叶的 distance state。返回 -1 表示非树叶或读取失败。
function getLeavesDistance(test: Test, x: number, y: number, z: number): number {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return -1;
    }
    // getState 的 key 类型是 keyof BlockStateSuperset（官方白名单），"distance" 不在其中（Cubium 树叶
    // 专有 state 名）。用 as any 绕过编译期类型边界，运行时 Cubium BlockPermutation.getState 支持任意
    // 已注册 state 名（同 sweetBerryBush.ts 的 any 范式）。
    const distance = block?.permutation?.getState("distance" as any);
    return typeof distance === "number" ? distance : -1;
}

// 原木邻接树叶 → 树叶 distance=1（wiki tech_树叶.txt 距离判定）。
//
// 布局：(3,1,1) 放原木，(4,1,1) 放树叶（原木 East 邻居）。原木放置触发树叶 updatePostPlacement
// → scheduleBlockTick(1) → 1 tick 后 tick() 重算 _updateDistance：East 邻居原木 _getDistance=0，
// +1=1 → 树叶 distance=1。
//
// 判定：pollUntilSucceed 轮询 (4,1,1) distance==1。distance 传播是 1 tick 延迟 scheduledTick，
// startTick=5 留调度余量。树叶 defaultState distance=7，原木放置后收敛到 1。
//
// 跨服务端：distance state 名两端一致，传播机制一致，可对比。
function leavesDistanceOneWhenLogAdjacent(test: Test): void {
    // 先放树叶 (4,1,1)（defaultState distance=7 persistent=false）。
    test.setBlockType("minecraft:oak_leaves", { x: 4, y: 1, z: 1 });

    // 放原木 (3,1,1)（树叶 West 邻居）。原木放置触发树叶 updatePostPlacement → 1 tick 后 distance=1。
    test.setBlockType("minecraft:oak_log", { x: 3, y: 1, z: 1 });

    pollUntilSucceed(
        test,
        () => getLeavesDistance(test, 4, 1, 1) === 1,
        {
            startTick: 5,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaves distance should be 1 when log adjacent, got ${getLeavesDistance(test, 4, 1, 1)}`,
                );
            },
        },
    );
}

// 叶链多跳传播：原木-叶-叶-叶 → 各叶 distance=1/2/3（验证 _updateDistance 的 min 链式传播）。
//
// 布局：(1,1,1) 原木，(2,1,1)/(3,1,1)/(4,1,1) 三连树叶（沿 +X 方向）。
//   - (2,1,1) 邻接原木 → distance=1
//   - (3,1,1) 邻接 (2,1,1) distance=1 → distance=2
//   - (4,1,1) 邻接 (3,1,1) distance=2 → distance=3
// _updateDistance 六向取 min(neighborDistance+1)，叶链逐跳 +1。但传播是异步的：原木放后 (2) 先收敛 1，
// (2) 收敛后触发 (3) updatePostPlacement → (3) 收敛 2，再触发 (4) → (4) 收敛 3。链式 1-tick 延迟，
// 三跳约需 3+ tick。pollUntilSucceed 轮询三叶全部收敛。
//
// 判定：(2)=1 && (3)=2 && (4)=3 全部成立。maxTick=80 留三跳链式传播余量。
function leavesDistanceIncreasesAwayFromLog(test: Test): void {
    // 三连树叶 (2,1,1)/(3,1,1)/(4,1,1)（defaultState distance=7）。
    test.setBlockType("minecraft:oak_leaves", { x: 2, y: 1, z: 1 });
    test.setBlockType("minecraft:oak_leaves", { x: 3, y: 1, z: 1 });
    test.setBlockType("minecraft:oak_leaves", { x: 4, y: 1, z: 1 });

    // 原木 (1,1,1)（叶链 West 端）。原木放置触发链式 distance 传播。
    test.setBlockType("minecraft:oak_log", { x: 1, y: 1, z: 1 });

    pollUntilSucceed(
        test,
        () => {
            return (
                getLeavesDistance(test, 2, 1, 1) === 1 &&
                getLeavesDistance(test, 3, 1, 1) === 2 &&
                getLeavesDistance(test, 4, 1, 1) === 3
            );
        },
        {
            startTick: 5,
            interval: 4,
            maxTick: 80,
            onTimeout: () => {
                test.assert(
                    false,
                    `leaves chain distance mismatch: (2)=${getLeavesDistance(test, 2, 1, 1)} (3)=${getLeavesDistance(test, 3, 1, 1)} (4)=${getLeavesDistance(test, 4, 1, 1)} expected 1/2/3`,
                );
            },
        },
    );
}

// 孤立树叶（周围无原木无叶）→ distance 保持 7（无原木邻居，min 始终 7）。
//
// 布局：(3,1,3) 放孤立树叶，六向邻居均为 air/glass（非原木非叶）。树叶 defaultState distance=7，
// 放置后 updatePostPlacement 邻居 air _getDistance=7（其他方块），+1=8，但 DISTANCE_1_7 钳制 7；
// _updateDistance min(7+1=8,...) 但实际 _getDistance 对 air 返回 7，min(7+1,...)=8，钳制回 7。
// tick() 重算后 distance 维持 7。
//
// 判定：等待若干 tick 后 distance==7（验证无原木邻居时 distance 不被错误降低）。startTick=5 留
// updatePostPlacement + tick 重算窗口。
function leavesDistanceSevenWhenNoLog(test: Test): void {
    // 孤立树叶 (3,1,3)（六向邻居 air/glass，无原木无叶）。
    test.setBlockType("minecraft:oak_leaves", { x: 3, y: 1, z: 3 });

    pollUntilSucceed(
        test,
        () => getLeavesDistance(test, 3, 1, 3) === 7,
        {
            startTick: 5,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(
                    false,
                    `isolated leaves distance should be 7 (no log adjacent), got ${getLeavesDistance(test, 3, 1, 3)}`,
                );
            },
        },
    );
}

export function registerLeavesDistanceTests(): void {
    GameTest.register("BlockBehaviorTests", "leaves_distance_one_when_log_adjacent", leavesDistanceOneWhenLogAdjacent)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "leaves_distance_increases_away_from_log", leavesDistanceIncreasesAwayFromLog)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "leaves_distance_seven_when_no_log", leavesDistanceSevenWhenNoLog)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
