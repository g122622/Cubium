// 活板门红石驱动开关行为 GameTest。
//
// wiki block_木活板门.txt#红石元件（指向活板门#红石元件）：活板门是红石元件，被红石信号激活时打开
// （OPEN=true）。木活板门可手动+红石控制，铁活板门仅红石控制。红石路径是电平触发：有信号→开，
// 无信号→关（与 wiki「玩家手动关闭被激活活板门需新上升沿」的锁存描述针对玩家交互路径，非红石电平路径）。
//
// C++ 链路：TrapDoorBlock（TrapDoorBlock.cpp）有 open（OPEN bool，默认 false）、half（HALF）、
//   powered（POWERED bool，默认 false）state。
//   - neighborChanged（:164-194）：`isPowered = RedstoneSystem::isBlockPowered(world, pos)`，
//     `if (isPowered == wasPowered) return`（信号未变不处理），否则
//     `OPEN=isPowered, POWERED=isPowered`（电平触发：充能→开，断电→关）。
//   - isBlockPowered 委托 RedstonePower::isPowered→isIndirectlyPowered，遍历六方向强弱信号。
//   - 红石块 getWeakPower/getStrongPower 全向 15，放置即充能，适合作测试电源。
//
// 测试覆盖（3 个场景，覆盖 wiki 红石开关核心行为，可跨服务端对比）：
//   1. 充能打开：活板门（默认 open=false）相邻放红石块 → OPEN 翻 true（充能打开）。
//   2. 断电关闭：活板门已开（open=true），移除红石块 → OPEN 翻回 false（断电关闭）。
//   3. 再次充能打开：承接场景 2 终态（open=false），再放红石块 → OPEN=true（可重复触发）。
//
// 关键约束：
// 1. 活板门逻辑在 neighborChanged（电平触发），放/移红石块走 setBlockState flags=3 → 邻居活板门
//    neighborChanged → OPEN=isPowered 写回。同步触发，pollUntilSucceed 留余量防时序。
// 2. 活板门须放在方块上方（支撑）。放活板门 (3,2,1)，下方 (3,1,1) 放 stone 支撑。
// 3. 活板门默认 half=Bottom（下半），setBlockType 放置走默认 state，无需指定 half。
// 4. 读 open state 用 getState("open" as any) 绕过白名单。
// 5. 场景 2/3 用 runAtTickTime 分阶段编排。
//
// 不测「玩家手动开关木活板门」：需 SimulatedPlayer interact（Cubium 是 stub），跳过。
//   TODO: 可补 wooden_trapdoor_toggles_on_player_interact（待 interact 实现后）。
// 不测「含水（waterlogged）」：涉水流+含水 state，复杂，跳过。
// 不测「梯子攀爬（isLadder）」：需实体 AI 攀爬判定，非确定，跳过。
//
// 跨服务端：活板门 open/powered state 名两端一致，红石电平开关行为与 vanilla 一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_木活板门.txt#红石元件（红石激活打开）
// Ref: TrapDoorBlock.cpp（neighborChanged: OPEN=isBlockPowered，电平触发充能开/断电关）
// Ref: RedstoneSystem::isBlockPowered 委托 RedstonePower::isPowered（同红石灯/铜灯/漏斗链路）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 活板门放 (3,2,1)（half=Bottom 默认），下方 (3,1,1) 放 stone 支撑，红石块电源 (4,2,1) 水平相邻。

// 读取活板门 open state（bool）。返回 null 表示读取失败或非活板门。
function getTrapdoorOpen(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("open" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 活板门：(3,1,1) stone 支撑，(3,2,1) 活板门（half=Bottom 默认）。
function placeTrapdoorOnStone(test: Test, trapdoorType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 });
    test.setBlockType(trapdoorType, { x: 3, y: 2, z: 1 });
}

// 场景 1：活板门（默认 open=false）相邻放红石块 → 充能打开，OPEN 翻 true。
//
// 布局：(3,1,1) stone + (3,2,1) 活板门（open=false），(4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块 flags=3 → 邻居活板门 neighborChanged → isBlockPowered=true != wasPowered(false) →
// OPEN=true, POWERED=true（充能打开）。
//
// 判定：pollUntilSucceed 轮询 open===true。
function trapdoorOpensWhenPowered(test: Test): void {
    placeTrapdoorOnStone(test, "minecraft:oak_trapdoor");

    // (4,2,1) 放红石块 → 邻居活板门 neighborChanged → isBlockPowered=true → OPEN=true（充能打开）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    pollUntilSucceed(
        test,
        () => getTrapdoorOpen(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `trapdoor open: should be true when powered, got ${getTrapdoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：活板门已开（open=true），移除红石块 → 断电关闭，OPEN 翻回 false。
//
// 布局：承接场景 1——活板门 open=true（powered=true），(4,2,1) 设 air。
// air 放置向邻居活板门 neighborChanged → isBlockPowered=false != wasPowered(true) →
// OPEN=false, POWERED=false（断电关闭）。
//
// 判定：pollUntilSucceed 轮询 open===false。
function trapdoorClosesWhenPowerRemoved(test: Test): void {
    placeTrapdoorOnStone(test, "minecraft:oak_trapdoor");
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待活板门打开（open=true）后移除电源。
    test.runAtTickTime(5, () => {
        if (getTrapdoorOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `trapdoor should be open before power removal, got open=${getTrapdoorOpen(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块 → 邻居活板门 neighborChanged → isBlockPowered=false → OPEN=false。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    pollUntilSucceed(
        test,
        () => getTrapdoorOpen(test, 3, 2, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `trapdoor open: should be false after power removed, got ${getTrapdoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：活板门关闭（open=false）后再次充能 → 再次打开 open=true（可重复触发）。
//
// 布局：承接场景 2 终态——活板门 open=false（电源已移除），再 (4,2,1) 放红石块。
// 放红石块 → neighborChanged → isBlockPowered=true → OPEN=true（再次打开）。
function trapdoorReopensWhenRepowered(test: Test): void {
    placeTrapdoorOnStone(test, "minecraft:oak_trapdoor");
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 阶段 2：tick 5 移除红石块（断电关闭，open: true→false）。
    test.runAtTickTime(5, () => {
        if (getTrapdoorOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `trapdoor should be open after first power, got open=${getTrapdoorOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 阶段 3：tick 12 重放红石块（再次充能打开，open: false→true）。
    test.runAtTickTime(12, () => {
        if (getTrapdoorOpen(test, 3, 2, 1) !== false) {
            test.assert(false, `trapdoor should be closed before re-power, got open=${getTrapdoorOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });
    });

    pollUntilSucceed(
        test,
        () => getTrapdoorOpen(test, 3, 2, 1) === true,
        {
            startTick: 16,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `trapdoor open: should reopen true when repowered, got ${getTrapdoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerTrapdoorTests(): void {
    GameTest.register("BlockBehaviorTests", "trapdoor_opens_when_powered", trapdoorOpensWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "trapdoor_closes_when_power_removed", trapdoorClosesWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "trapdoor_reopens_when_repowered", trapdoorReopensWhenRepowered)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
