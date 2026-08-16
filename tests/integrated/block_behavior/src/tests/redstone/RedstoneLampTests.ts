// 红石灯充能点亮/失能熄灭行为 GameTest。
//
// wiki mechanism_红石灯.txt#用途（:52-59）：红石灯是机械元件，被下列形式激活：
//   - 毗邻的开启的电源（例外：红石火把不激活其附着物，侦测器只激活指向方）；
//   - 毗邻的充能方块；
//   - 毗邻的指向红石灯且激活的红石中继器/比较器/红石粉。
// 「红石灯收到红石信号会立刻被点亮。失去红石信号供给后需要 4 游戏刻才能熄灭。」（:59）
//   - JE：失去信号后创建 4 刻后的计划刻，执行时若无信号则熄灭（:60）。
//
// C++ 链路：RedstoneLampBlock 有 lit state（bool，LIT）。
//   - onBlockAdded（RedstoneLampBlock.cpp:76-88）：isPowered 与 isLit 不一致时，应亮则立即 setBlockState
//     (lit=true)，应灭则 scheduleBlockTick(4, High)。
//   - neighborChanged（:90-116）：同 onBlockAdded 逻辑——shouldLit 与 isCurrentlyLit 不一致时，应亮立即
//     setBlockState(lit=true)，应灭 scheduleBlockTick(4, High)。
//   - tick（:118-127）：执行时若 !isPowered 且 isLit → setBlockState(lit=false) 熄灭。
//   - isPowered 委托 RedstonePower::isPowered→isIndirectlyPowered（RedstonePower.cpp:112-142）：遍历六方向
//     邻居，对每个调 getStrongPower 与 getWeakPower，任一 >0 返回 true。
//   - 红石块（RedstoneBlock.cpp:41-61）getWeakPower/getStrongPower 全向返回 15，是持续开启的电源，放置即
//     供电（无 POWERED state 切换），适合做测试电源。
//
// 电源选择说明：本测试用红石块（minecraft:redstone_block）作电源，而非拉杆/按钮。原因：LeverBlock::
//   onBlockAdded（LeverBlock.cpp:89-95）为空实现——拉杆放置/状态修改（setBlockWithStates 改 POWERED）
//   不向邻居传播红石信号，信号只在玩家 toggle（右键）路径传播（_notifyNeighbors 仅 toggle 调用）。由于
//   GameTest 无右键交互 API（SimulatedPlayer interact 为 stub），无法用拉杆模拟「翻转供电」。红石块无
//   state 切换、放置即全向供电，走 setBlockState flags=3 邻居 neighborChanged 链路，能可靠点亮红石灯。
//   （注：拉杆放置不传播信号是与 vanilla 的偏差，vanilla 拉杆 setBlockState 触发 neighborChanged 链；
//   该偏差属 LeverBlock 另一问题，本文件不涉及，仅以红石块规避。）
//
// 测试覆盖（2 个场景，行为与 wiki 一致，可跨服务端对比）：
//   1. 红石灯相邻放置红石块 → 灯立即 lit=true（neighborChanged→isPowered 真→立即点亮）。
//   2. 红石灯已亮，移除相邻红石块（→air）→ 灯 4 tick 后 lit=false（neighborChanged→isPowered 假→
//      scheduleTick(4)→tick 熄灭）。
//
// 关键约束：
// 1. 场景 1「先灯后块」：先放红石灯（默认 lit=false），再放相邻红石块。放红石块走 setBlockState flags=3
//    → 对邻居红石灯 neighborChanged → shouldLit=isPowered(红石块 weakPower 15)>0=true ≠ isLit(false) →
//    立即 setBlockState(lit=true)。同 tick 同步点亮，但用 pollUntilSucceed 轮询 lit=true 留余量（防
//    neighborChanged 链路时序）。
// 2. 场景 2 移除红石块必须是非 no-op 写入——先有红石块再设 air，保证红石块→air 真实状态变化派发
//    neighborChanged。air 放置向邻居红石灯 neighborChanged → shouldLit=isPowered(air 邻居全 0)=false ≠
//    isLit(true) → scheduleBlockTick(4, High)。4 tick 后 tick 执行 → !isPowered && isLit → setBlockState
//    (lit=false)。用 pollUntilSucceed 轮询 lit=false（startTick 留 4+ 余量）。
// 3. 读 lit state 用 getState("lit" as any) 绕过 BlockStateSuperset 白名单（同栅栏/灯笼范式）。
//
// 不测「红石火把不激活其附着红石灯」例外：需精确布置红石火把朝向 + 附着关系，且红石火把放置信号传播
//   链路复杂，按「单一职责」本文件聚焦红石块供电的核心亮灭行为。TODO: 可补 redstone_torch_does_not_
//   power_attached_lamp 例外测试。
// 不测「充能方块激活」（红石块本身被充能再激活其他灯）：需中继器/红石线充能红石块，链路复杂，跳过。
// 不测「红石粉指向激活」：依赖红石线连接形态 + 指向判定，RedstoneWireTests 已覆盖连接，本文件聚焦
//   红石块直接供电。
//
// 跨服务端：红石灯 lit state 名两端一致（Java 式 bool），亮灭行为与 vanilla 一致（立即亮、4 tick 灭）。
//   注意 Cubium tick(4) 延迟与 vanilla 4 游戏刻对应，pollUntilSucceed maxTick 需放宽容忍时序偏差。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_红石灯.txt#用途（电源激活+4刻熄灭）
// Ref: RedstoneLampBlock.cpp（onBlockAdded/neighborChanged/tick/isPowered 链路）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）
// Ref: RedstonePower.cpp（isIndirectlyPowered 遍历六方向强弱信号）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,2,1) 作红石灯位、(4,2,1) 作红石块电源位，水平相邻，均在 air 空腔内。

// 读取红石灯 lit state（bool）。返回 null 表示读取失败或非红石灯。
function getLampLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 红石灯相邻放置红石块后立即点亮（lit=true）。
//
// 布局：(3,2,1) 放红石灯（默认 lit=false），(4,2,1) 放红石块（水平相邻，全向供电 15）。
// 放红石块走 setBlockState flags=3 → 邻居红石灯 (3,2,1) neighborChanged →
// shouldLit=isPowered(相邻红石块 weakPower 15)>0=true ≠ isLit(false) → 立即 setBlockState(lit=true)。
//
// 判定：pollUntilSucceed 轮询 lit===true（neighborChanged 同步触发，留余量防时序）。
function redstoneLampLightsUpWhenAdjacentToPowerSource(test: Test): void {
    // (3,2,1) 放红石灯（默认 lit=false，无电源时熄灭）。
    test.setBlockType("minecraft:redstone_lamp", { x: 3, y: 2, z: 1 });

    // (4,2,1) 放红石块（水平相邻红石灯，getWeakPower 全向 15）。放红石块 flags=3 → 邻居红石灯
    // neighborChanged → shouldLit=isPowered(红石块)>0=true ≠ isLit(false) → 立即 setBlockState(lit=true)。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 lit === true（neighborChanged 同步点亮，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getLampLit(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `redstone_lamp lit: should be true, got ${getLampLit(test, 3, 2, 1)}`);
            },
        },
    );
}

// 红石灯已亮，移除相邻红石块后 4 tick 熄灭（lit=false）。
//
// 布局：(3,2,1) 红石灯 + (4,2,1) 红石块（灯已因红石块点亮 lit=true），再 (4,2,1) 设 air 移除红石块。
// air 放置向邻居红石灯 neighborChanged → shouldLit=isPowered(air 邻居全 0)=false ≠ isLit(true) →
// scheduleBlockTick(4, High)。4 tick 后 tick 执行 → !isPowered && isLit → setBlockState(lit=false)。
//
// 判定：pollUntilSucceed 轮询 lit===false（scheduleTick(4) 延迟熄灭，startTick 留 4+ 余量）。
function redstoneLampTurnsOffWhenPowerRemoved(test: Test): void {
    // (3,2,1) 放红石灯，（4,2,1）放红石块（灯因红石块 neighborChanged 立即点亮 lit=true）。
    test.setBlockType("minecraft:redstone_lamp", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待灯点亮（lit=true）后再移除电源——用 runAtTickTime 在点亮稳定后移除红石块。
    // startTick=2 给 neighborChanged 点亮留余量；移除红石块触发熄灭 scheduleTick(4)。
    test.runAtTickTime(5, () => {
        // 确认灯已点亮（若未点亮说明供电链路异常，断言失败暴露缺陷）。
        if (getLampLit(test, 3, 2, 1) !== true) {
            test.assert(false, `redstone_lamp should be lit before power removal, got ${getLampLit(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块（红石块→air 真实状态变化，派发邻居更新）。air 放置向邻居红石灯
        // neighborChanged → shouldLit=false ≠ isLit(true) → scheduleBlockTick(4, High) → 4 tick 后熄灭。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 lit === false（scheduleTick(4) 延迟熄灭，startTick 留 4+ 余量；5+4=9 起轮询）。
    pollUntilSucceed(
        test,
        () => getLampLit(test, 3, 2, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `redstone_lamp lit: should be false after power removed, got ${getLampLit(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerRedstoneLampTests(): void {
    GameTest.register("BlockBehaviorTests", "redstone_lamp_lights_up_when_adjacent_to_power_source", redstoneLampLightsUpWhenAdjacentToPowerSource)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "redstone_lamp_turns_off_when_power_removed", redstoneLampTurnsOffWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
