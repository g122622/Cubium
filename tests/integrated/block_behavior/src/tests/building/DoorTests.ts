// 门红石驱动开关行为 GameTest。
//
// wiki tech_门.txt#红石元件（指向门#红石元件）：门是红石元件，被红石信号激活时打开（OPEN=true）。
//   - 铁门仅能被红石信号控制（玩家无法手动开关）。
//   - 木门可手动+红石控制。
//   - 红石路径是电平触发：有信号→开（OPEN=true），无信号→关（OPEN=false）。
//   - 门由上下两半组成（HALF=Lower/Upper），充能时双半 OPEN/POWERED 同步翻转。
//
// C++ 链路：DoorBlock（DoorBlock.cpp）有 half（DOUBLE_BLOCK_HALF，默认 Lower）、facing
//   （HORIZONTAL_FACING，默认 North）、open（OPEN bool，默认 false）、hinge（HINGE，默认 Left）、
//   powered（POWERED bool，默认 false）五个 state。
//   - neighborChanged（DoorBlock.cpp:125-165）：`powered = isBlockPowered(pos) || isBlockPowered(otherHalfPos)`，
//     `if (powered == wasPowered) return`（信号未变不处理），否则 `OPEN=powered, POWERED=powered` 写回
//     当前半 + 另一半（电平触发：充能→开，断电→关）。
//   - isBlockPowered 委托 RedstonePower::isPowered→isIndirectlyPowered，遍历六方向强弱信号
//     （同红石灯/活板门/漏斗链路）。
//   - updatePostPlacement（:167-205）：双半 facing/open/hinge/powered 同步，下半无支撑变 air。
//   - 红石块（RedstoneBlock）getWeakPower/getStrongPower 全向 15，放置即充能，适合作测试电源。
//
// 测试用铁门（minecraft:iron_door）：纯红石控制（无手动交互歧义），木门红石路径与铁门等价。
//
// 测试覆盖（4 个场景，覆盖 wiki 红石开关+双半同步核心行为，可跨服务端对比）：
//   1. 充能打开：铁门（默认 open=false）相邻放红石块 → OPEN 翻 true（充能打开）。
//   2. 断电关闭：铁门已开（open=true），移除红石块 → OPEN 翻回 false（断电关闭）。
//   3. 再次充能打开：承接场景 2 终态（open=false），再放红石块 → OPEN=true（可重复触发）。
//   4. 双半同步：双半铁门充能后，下半与上半 OPEN 同步翻转（验证双半配对同步链路）。
//
// 关键约束：
// 1. 门逻辑在 neighborChanged（电平触发），放/移红石块走 setBlockState flags=3 → 邻居门
//    neighborChanged → OPEN=powered 写回。同步触发，pollUntilSucceed 留余量防时序。
// 2. setBlockType 不走 onBlockPlacedBy（不走 BlockItem 放置流程），只放单格默认 state（half=Lower）。
//    故双半门须手动放：下半 setBlockType（half=Lower 默认），上半 setBlockWithStates("half=upper")
//    （Cubium 专有 API，设 state 字符串）。
// 3. 门下半须放在 solidSide 方块上方——(3,1,1) 放 stone 支撑。门下半 (3,2,1)，上半 (3,3,1)。
// 4. 红石块电源 (4,2,1) 水平相邻下半（全向充能 15）。
// 5. 读 open state 用 getState("open" as any) 绕过 BlockStateSuperset 白名单。
// 6. 场景 2/3 用 runAtTickTime 分阶段编排（先等打开稳定，再移除/重放红石块）。
//
// 不测「木门手动开关」：依赖 SimulatedPlayer interact（Cubium 为 stub），跳过。
//   TODO: 待 interact 实现后补 wooden_door_toggles_on_player_interact。
// 不测「hinge 左右差异」：hinge 计算依赖放置上下文（BlockItemUseContext），setBlockType 不触发，
//   默认 Left，跳过。
// 不测「含水（waterlogged）」：涉水流+含水 state，复杂，跳过。
//
// 跨服务端：门 open/powered/half state 名两端一致，红石电平开关行为与 vanilla 一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_门.txt#红石元件（红石激活打开，双半同步）
// Ref: DoorBlock.cpp（neighborChanged: OPEN=isBlockPowered，电平触发充能开/断电关；updatePostPlacement 双半同步）
// Ref: RedstoneSystem::isBlockPowered 委托 RedstonePower::isPowered（同红石灯/活板门/漏斗链路）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 门下半 (3,2,1)，门上半 (3,3,1)，下方 (3,1,1) stone 支撑，红石块电源 (4,2,1) 水平相邻下半。

// 读取门 open state（bool）。返回 null 表示读取失败或非门。
function getDoorOpen(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("open" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 双半铁门：(3,1,1) stone 支撑，(3,2,1) 门下半（half=Lower 默认），(3,3,1) 门上半（half=upper）。
// setBlockType 只放默认 state（half=Lower），上半须用 setBlockWithStates 显式设 half=upper。
function placeDoorSetup(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:iron_door", { x: 3, y: 2, z: 1 }); // 下半（half=Lower 默认）
    test.setBlockWithStates("minecraft:iron_door", { x: 3, y: 3, z: 1 }, "half=upper"); // 上半
}

// 场景 1：铁门（默认 open=false）相邻放红石块 → 充能打开，OPEN 翻 true。
//
// 布局：双半铁门 + (4,2,1) 放红石块（水平相邻下半，全向充能 15）。
// 放红石块 flags=3 → 邻居门 neighborChanged → isBlockPowered=true != wasPowered(false) →
// OPEN=true, POWERED=true（充能打开，双半同步）。
//
// 判定：pollUntilSucceed 轮询下半 open===true。
function doorOpensWhenPowered(test: Test): void {
    placeDoorSetup(test);

    // (4,2,1) 放红石块 → 邻居门 neighborChanged → isBlockPowered=true → OPEN=true（充能打开）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言下半 open === true（neighborChanged 同步触发，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getDoorOpen(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `door open: should be true when powered, got ${getDoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：铁门已开（open=true），移除红石块 → 断电关闭，OPEN 翻回 false。
//
// 布局：承接场景 1——门 open=true（powered=true），(4,2,1) 设 air。
// air 放置向邻居门 neighborChanged → isBlockPowered=false != wasPowered(true) →
// OPEN=false, POWERED=false（断电关闭）。
//
// 判定：pollUntilSucceed 轮询下半 open===false。
function doorClosesWhenPowerRemoved(test: Test): void {
    placeDoorSetup(test);
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待门打开（open=true）后移除电源。
    test.runAtTickTime(5, () => {
        if (getDoorOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `door should be open before power removal, got open=${getDoorOpen(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块 → 邻居门 neighborChanged → isBlockPowered=false → OPEN=false。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言下半 open === false（断电关闭，恢复关闭）。
    pollUntilSucceed(
        test,
        () => getDoorOpen(test, 3, 2, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `door open: should be false after power removed, got ${getDoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：铁门关闭（open=false）后再次充能 → 再次打开 open=true（可重复触发）。
//
// 布局：承接场景 2 终态——门 open=false（电源已移除），再 (4,2,1) 放红石块。
// 放红石块 → neighborChanged → isBlockPowered=true → OPEN=true（再次打开）。
function doorReopensWhenRepowered(test: Test): void {
    placeDoorSetup(test);
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 阶段 2：tick 5 移除红石块（断电关闭，open: true→false）。
    test.runAtTickTime(5, () => {
        if (getDoorOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `door should be open after first power, got open=${getDoorOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 阶段 3：tick 12 重放红石块（再次充能打开，open: false→true）。
    test.runAtTickTime(12, () => {
        if (getDoorOpen(test, 3, 2, 1) !== false) {
            test.assert(false, `door should be closed before re-power, got open=${getDoorOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言下半 open === true（再次充能打开）。
    pollUntilSucceed(
        test,
        () => getDoorOpen(test, 3, 2, 1) === true,
        {
            startTick: 16,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `door open: should reopen true when repowered, got ${getDoorOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 4：双半同步——充能后下半与上半 OPEN 同步翻转（验证双半配对同步链路）。
//
// 布局：双半铁门 + (4,2,1) 放红石块。neighborChanged 写回当前半（下半）+ 另一半（上半）OPEN。
// 判定：pollUntilSucceed 轮询下半 open===true 且上半 open===true（双半同步打开）。
// 此场景验证 updatePostPlacement/neighborChanged 双半同步链路（OPEN/POWERED 双半一致）。
function doorHalvesSyncOpen(test: Test): void {
    placeDoorSetup(test);

    // (4,2,1) 放红石块 → 邻居下半 neighborChanged → 写回下半+上半 OPEN=true（双半同步）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言下半 (3,2,1) 与上半 (3,3,1) open 均 === true（双半同步打开）。
    pollUntilSucceed(
        test,
        () => getDoorOpen(test, 3, 2, 1) === true && getDoorOpen(test, 3, 3, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `door halves should sync open: lower open=${getDoorOpen(test, 3, 2, 1)}, upper open=${getDoorOpen(test, 3, 3, 1)} (expected both true)`);
            },
        },
    );
}

export function registerDoorTests(): void {
    GameTest.register("BlockBehaviorTests", "door_opens_when_powered", doorOpensWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "door_closes_when_power_removed", doorClosesWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "door_reopens_when_repowered", doorReopensWhenRepowered)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "door_halves_sync_open", doorHalvesSyncOpen)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
