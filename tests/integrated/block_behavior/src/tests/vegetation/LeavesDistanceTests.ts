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
// 枯萎（randomTick 消失）与 persistent 门测试不写独立的枯萎概率测试，但需注意：vanilla LeavesBlock.randomTick
// 的枯萎判定 decaying 无概率门限（!persistent && distance==7 即 removeBlock，对齐 Cubium LeavesBlock.cpp:138），
// 故孤立树叶（persistent=false distance=7）在 randomTick 命中时必然枯萎消失。测试3 的孤立树叶用 persistent=true
// 规避枯萎（见 leavesDistanceSevenWhenNoLog 注释），否则全量并行下被其他测试调高的 randomTickSpeed 污染致枯萎
// 假失败。persistent=true 不影响 distance 计算（_updateDistance 不检查 persistent），仍能验证 distance 传播。
// TODO: 待需要时可用高 randomTickSpeed + persistent=false + distance==7 概率性测枯萎消失行为本身。
//
// 跨服务端：distance state 名两端均为 distance（DISTANCE_1_7），值域 1-7 一致，传播机制两端一致，
// 可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_树叶.txt（树叶距离判定）
// Ref: LeavesBlock.cpp（_updateDistance/_getDistance/updatePostPlacement/tick）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { BlockPermutation } from "@minecraft/server";
import type { Vector3 } from "@minecraft/server";
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
// 放置后无原木邻居，distance 维持 7。
//
// 【重要】必须用 persistent=true 的树叶，原因：树叶 randomTick 枯萎条件是 !persistent && distance==7
// （LeavesBlock.cpp:138，对齐 vanilla LeavesBlock.decaying 无概率门限——vanilla randomTick 也是
// decaying 为 true 即 removeBlock）。孤立树叶 defaultState persistent=false distance=7 恰满足枯萎条件，
// 默认 randomTickSpeed=3 下 60 tick 内约 4.3% 概率被 randomTick 命中枯萎消失；全量并行环境下若其他测试
// （CopperOxidationTests/GrassSpreadTests 等）调高 randomTickSpeed=1000 且 gamerule 跨测试持久未恢复，
// 孤立树叶几乎必然枯萎消失 → distance 读取变 air → getState("distance") 返回 undefined → getLeavesDistance
// 返回 -1，测试假失败（got -1）。用 persistent=true 树叶（对齐玩家手放 getStateForPlacement 设 persistent=true）
// 规避枯萎：persistent=true 不满足 decaying 的 !persistent 条件，randomTick 不移除；distance 仍由
// _updateDistance 正常计算（_updateDistance 不检查 persistent），无原木邻居时 distance=7。
//
// persistent=true 树叶用 BlockPermutation.resolve + setBlockPermutation 放置（setBlockType 取 defaultState
// persistent=false 无法设 persistent，同 sweetBerryBush.ts 的 resolve+setBlockPermutation 范式）。
//
// 判定：等待若干 tick 后 distance==7（验证无原木邻居时 distance 不被错误降低）。startTick=5 留
// updatePostPlacement + tick 重算窗口。persistent=true 保证树叶存活不枯萎，distance 稳定 7。
function leavesDistanceSevenWhenNoLog(test: Test): void {
    // 孤立树叶 (3,1,3)（六向邻居 air/glass，无原木无叶）。persistent=true 防枯萎消失。
    const permutation = BlockPermutation.resolve("minecraft:oak_leaves", {
        persistent: true,
        distance: 7,
    }) as any;
    (test as unknown as {
        setBlockPermutation: (blockData: unknown, blockLocation: Vector3) => void;
    }).setBlockPermutation(permutation, { x: 3, y: 1, z: 3 });

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
