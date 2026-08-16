// 栅栏门红石驱动开关行为 GameTest。
//
// wiki tech_栅栏门.txt：栅栏门可被红石信号控制——被充能时打开（OPEN=true），断电时关闭。
//   - 红石路径是电平触发：有信号→开（OPEN=true），无信号→关（OPEN=false）。
//   - 玩家也可手动开关（右键），本测试只覆盖红石路径（手动交互依赖 interact，Cubium 为 stub）。
//   - 栅栏门是单格方块（无上下半），与门不同；放置在 solidSide 方块上方。
//   - 邻墙时 in_wall=true（栅栏门贴合墙的视觉/碰撞，state IN_WALL）。
//
// C++ 链路：FenceGateBlock（FenceGateBlock.cpp）有四个 state：
//   - HORIZONTAL_FACING（Direction，默认 North）
//   - OPEN（bool，默认 false）
//   - IN_WALL（bool，默认 false）
//   - POWERED（bool，默认 false）
//   - neighborChanged（FenceGateBlock.cpp:122-149）：`powered = isBlockPowered(pos)`，
//     `if (powered == wasPowered) return`（信号未变不处理），否则
//     `OPEN=powered, POWERED=powered` 写回（电平触发：充能→开，断电→关）。
//   - isBlockPowered 委托 RedstoneSystem::isBlockPowered→isIndirectlyPowered，遍历六方向强弱信号
//     （同门/活板门/漏斗链路）。
//   - updatePostPlacement（:151-170）：检测左右是否墙，是则 IN_WALL=true（in_wall 链路）。
//   - 红石块（RedstoneBlock）getWeakPower/getStrongPower 全向 15，放置即充能，适合作测试电源。
//
// 测试覆盖（6 个场景，覆盖 wiki 红石开关+in_wall 铗墙+手动开关核心行为）：
//   1. 充能打开：栅栏门（默认 open=false）相邻放红石块 → OPEN 翻 true（充能打开）。
//   2. 断电关闭：栅栏门已开（open=true），移除红石块 → OPEN 翻回 false（断电关闭）。
//   3. 再次充能打开：承接场景 2 终态（open=false），再放红石块 → OPEN=true（可重复触发）。
//   4. 邻墙变 in_wall：栅栏门左右放墙 → IN_WALL 翻 true（updatePostPlacement 铗墙链路）。
//   5. 手动打开：栅栏门（open=false）+ interactWithBlock（空手右键）→ OPEN 翻 true（one-sided）。
//   6. 手动关闭：承接场景 5（open=true）+ 再次 interactWithBlock → OPEN 翻回 false（one-sided）。
//
// 关键约束：
// 1. 栅栏门逻辑在 neighborChanged（电平触发），放/移红石块走 setBlockState flags=3 → 邻居栅栏门
//    neighborChanged → OPEN=powered 写回。同步触发，pollUntilSucceed 留余量防时序。
// 2. 栅栏门须放在 solidSide 方块上方——(3,1,1) 放 stone 支撑，栅栏门 (3,2,1)。
// 3. 红石块电源 (4,2,1) 水平相邻栅栏门（全向充能 15）。
// 4. 读 open/in_wall state 用 getState("open"/"in_wall" as any) 绕过 BlockStateSuperset 白名单。
// 5. 场景 2/3 用 runAtTickTime 分阶段编排（先等打开稳定，再移除/重放红石块）。
// 6. 场景 4 in_wall：墙放栅栏门左右两侧（facing North 时左右是 East/West，即 x±1）。
//    放墙触发栅栏门 updatePostPlacement → _isWall 检测左右墙 → IN_WALL=true。
//
// 不测「玩家手动开关」：依赖 SimulatedPlayer interact（Cubium 为 stub），跳过。
//   已补全 interactWithBlock 空手右键绑定（ScriptSimulatedPlayer.cpp），见下方
//   fence_gate_toggles_open_on_interact / fence_gate_toggles_back_on_second_interact 场景。
// 不测「in_wall 影响 collision shape」：shape 差异是视觉/碰撞细节，本测试只验 state 翻转。
//
// 跨服务端：栅栏门 open/powered/in_wall state 名两端一致，红石电平开关行为与 vanilla 一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_栅栏门.txt#红石（红石激活打开，电平触发）
// Ref: FenceGateBlock.cpp（neighborChanged: OPEN=isBlockPowered 电平触发；updatePostPlacement: in_wall 铗墙）
// Ref: RedstoneSystem::isBlockPowered 委托 RedstonePower::isPowered（同门/活板门/漏斗链路）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { Direction } from "@minecraft/server";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 栅栏门 (3,2,1)，下方 (3,1,1) stone 支撑，红石块电源 (4,2,1) 水平相邻。
// 场景 4 墙放 (2,2,1)/(5,2,1)（栅栏门 facing North 时左右是 East/West = x±1）。

// 读取栅栏门 open state（bool）。返回 null 表示读取失败或非栅栏门。
function getGateOpen(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("open" as any);
    return typeof value === "boolean" ? value : null;
}

// 读取栅栏门 in_wall state（bool）。返回 null 表示读取失败或非栅栏门。
function getGateInWall(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("in_wall" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 栅栏门：(3,1,1) stone 支撑，(3,2,1) 栅栏门（facing North 默认）。
function placeGateSetup(test: Test, gateType: string): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType(gateType, { x: 3, y: 2, z: 1 }); // 栅栏门（facing North 默认）
}

// 场景 1：栅栏门（默认 open=false）相邻放红石块 → 充能打开，OPEN 翻 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 栅栏门 + (4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块 flags=3 → 邻居栅栏门 neighborChanged → isBlockPowered=true != wasPowered(false) →
// OPEN=true, POWERED=true（充能打开）。
//
// 判定：pollUntilSucceed 轮询 open===true。
function fenceGateOpensWhenPowered(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");

    // (4,2,1) 放红石块 → 邻居栅栏门 neighborChanged → isBlockPowered=true → OPEN=true（充能打开）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 open === true（neighborChanged 同步触发，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getGateOpen(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `fence gate open: should be true when powered, got ${getGateOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：栅栏门已开（open=true），移除红石块 → 断电关闭，OPEN 翻回 false。
//
// 布局：承接场景 1——栅栏门 open=true（powered=true），(4,2,1) 设 air。
// air 放置向邻居栅栏门 neighborChanged → isBlockPowered=false != wasPowered(true) →
// OPEN=false, POWERED=false（断电关闭）。
//
// 判定：pollUntilSucceed 轮询 open===false。
function fenceGateClosesWhenPowerRemoved(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待栅栏门打开（open=true）后移除电源。
    test.runAtTickTime(5, () => {
        if (getGateOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `fence gate should be open before power removal, got open=${getGateOpen(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块 → 邻居栅栏门 neighborChanged → isBlockPowered=false → OPEN=false。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 open === false（断电关闭，恢复关闭）。
    pollUntilSucceed(
        test,
        () => getGateOpen(test, 3, 2, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `fence gate open: should be false after power removed, got ${getGateOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：栅栏门关闭（open=false）后再次充能 → 再次打开 open=true（可重复触发）。
//
// 布局：承接场景 2 终态——栅栏门 open=false（电源已移除），再 (4,2,1) 放红石块。
// 放红石块 → neighborChanged → isBlockPowered=true → OPEN=true（再次打开）。
function fenceGateReopensWhenRepowered(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 阶段 2：tick 5 移除红石块（断电关闭，open: true→false）。
    test.runAtTickTime(5, () => {
        if (getGateOpen(test, 3, 2, 1) !== true) {
            test.assert(false, `fence gate should be open after first power, got open=${getGateOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 阶段 3：tick 12 重放红石块（再次充能打开，open: false→true）。
    test.runAtTickTime(12, () => {
        if (getGateOpen(test, 3, 2, 1) !== false) {
            test.assert(false, `fence gate should be closed before re-power, got open=${getGateOpen(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 open === true（再次充能打开）。
    pollUntilSucceed(
        test,
        () => getGateOpen(test, 3, 2, 1) === true,
        {
            startTick: 16,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `fence gate open: should reopen true when repowered, got ${getGateOpen(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 4：邻墙变 in_wall——栅栏门左右放墙 → IN_WALL 翻 true（updatePostPlacement 铗墙链路）。
//
// 布局：(3,2,1) 栅栏门（facing North），左右 (2,2,1)/(5,2,1) 放墙（facing North 时左右是
//   East/West = x±1）。放墙触发栅栏门 updatePostPlacement → _isWall 检测左右墙 → IN_WALL=true。
//
// 判定：pollUntilSucceed 轮询 in_wall===true。
// 注意：先放栅栏门后放墙（放墙触发栅栏门 updatePostPlacement）。墙须是 WallBlock（如 cobblestone_wall）。
function fenceGateInWallWhenAdjacentWall(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");

    // 放墙前断言 in_wall=false（默认无墙）。
    test.runAtTickTime(2, () => {
        if (getGateInWall(test, 3, 2, 1) !== false) {
            test.assert(false, `fence gate in_wall should be false before placing walls, got ${getGateInWall(test, 3, 2, 1)}`);
            return;
        }
        // 左右放墙（栅栏门 facing North，左右 East/West = x±1）→ 触发栅栏门 updatePostPlacement。
        test.setBlockType("minecraft:cobblestone_wall", { x: 2, y: 2, z: 1 });
        test.setBlockType("minecraft:cobblestone_wall", { x: 5, y: 2, z: 1 });
    });

    // 轮询断言 in_wall === true（updatePostPlacement 检测到左右墙，翻 in_wall=true）。
    pollUntilSucceed(
        test,
        () => getGateInWall(test, 3, 2, 1) === true,
        {
            startTick: 6,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `fence gate in_wall: should be true when adjacent to walls, got ${getGateInWall(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 5：手动打开——栅栏门（open=false）+ interactWithBlock（空手右键）→ OPEN 翻 true。
//
// 布局：(3,1,1) stone + (3,2,1) 栅栏门（open=false）。
// interactWithBlock（空手右键）→ onBlockActivated：wasOpen=false → with(OPEN,true) setBlockState → Success。
//
// 判定：interactWithBlock 返 true（Success），open === true（栅栏门被手动打开）。
// one-sided：依赖 Cubium 补全的 interactWithBlock 绑定。
function fenceGateTogglesOpenOnInteract(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");
    test.assert(getGateOpen(test, 3, 2, 1) === false, `fence gate open should be false before, got ${getGateOpen(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // interactWithBlock 空手右键栅栏门 → onBlockActivated toggle(OPEN=true) → Success。
    // interactWithBlock 为 Cubium 补全的 SimulatedPlayer 方法（类型定义未声明），用 as any 绕过类型检查。
    const used = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean })
        .interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(used, "interactWithBlock should return true when toggling fence gate open");

    // 判定：open === true（栅栏门手动打开）。
    test.assert(getGateOpen(test, 3, 2, 1) === true, `fence gate open should be true after interact, got ${getGateOpen(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 6：手动关闭——承接场景 5（open=true）+ 再次 interactWithBlock → OPEN 翻回 false。
//
// 布局：(3,1,1) stone + (3,2,1) 栅栏门 open=true（先 interact 打开）。
// 再次 interactWithBlock → onBlockActivated：wasOpen=true → with(OPEN,false) → Success。
//
// 判定：第二次 interactWithBlock 返 true，open === false（栅栏门手动关闭）。
function fenceGateTogglesBackOnSecondInteract(test: Test): void {
    placeGateSetup(test, "minecraft:oak_fence_gate");

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const interact = (farmer as unknown as { interactWithBlock: (pos: unknown, dir: unknown) => boolean });

    // 第一次 interact 打开栅栏门（open false→true）。
    const firstUsed = interact.interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(firstUsed, "first interact should return true when toggling fence gate open");
    test.assert(getGateOpen(test, 3, 2, 1) === true, `fence gate open should be true after first interact, got ${getGateOpen(test, 3, 2, 1)}`);

    // 第二次 interact 关闭栅栏门（open true→false）。
    const secondUsed = interact.interactWithBlock({ x: 3, y: 2, z: 1 }, Direction.Up);
    test.assert(secondUsed, "second interact should return true when toggling fence gate closed");

    // 判定：open === false（栅栏门手动关闭）。
    test.assert(getGateOpen(test, 3, 2, 1) === false, `fence gate open should be false after second interact, got ${getGateOpen(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerFenceGateTests(): void {
    GameTest.register("BlockBehaviorTests", "fence_gate_opens_when_powered", fenceGateOpensWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "fence_gate_closes_when_power_removed", fenceGateClosesWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "fence_gate_reopens_when_repowered", fenceGateReopensWhenRepowered)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
    GameTest.register("BlockBehaviorTests", "fence_gate_in_wall_when_adjacent_wall", fenceGateInWallWhenAdjacentWall)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "fence_gate_toggles_open_on_interact", fenceGateTogglesOpenOnInteract)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "fence_gate_toggles_back_on_second_interact", fenceGateTogglesBackOnSecondInteract)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
