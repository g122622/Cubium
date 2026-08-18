// 激活铁轨红石充能激活行为 GameTest。
//
// wiki mechanism_铁轨.txt#激活铁轨：激活铁轨是红石机械元件，被红石信号激活时 POWERED=true（激活时
//   令上方矿车减速/ shaking 等，本测试只验 POWERED state 翻转）。红石路径是电平触发：有信号→POWERED=true，
//   无信号→POWERED=false（与红石灯立即亮不同，铁轨无延迟，neighborChanged 直接 setBlockState）。
//
// C++ 链路：ActivatorRailBlock（redstone/ActivatorRailBlock.cpp）继承 AbstractRailBlock，有 SHAPE +
//   POWERED + WATERLOGGED 三个 state。默认 state（:62-64）：shape=NorthSouth, powered=false, waterlogged=false。
//   - neighborChanged（:73-92）：`shouldBePowered = RedstonePower::isPowered(world, pos)`，
//     `isCurrentlyPowered = isPowered(state)`，不等则 `with(POWERED, shouldBePowered) setBlockState(3)` →
//     立即翻转（无 scheduleTick 延迟，与红石灯 4 tick 延迟不同）。
//   - isPowered 委托 RedstonePower::isPowered→isIndirectlyPowered（RedstonePower.cpp:112-142），遍历六方向
//     邻居强弱信号（同红石灯/门/活板门链路）。
//   - 红石块（RedstoneBlock）getWeakPower/getStrongPower 全向 15，放置即充能，适合作测试电源。
//   - AbstractRailBlock::isValidPosition（AbstractRailBlock.cpp:215-221）要求下方 canSupportRigidBlock
//     （solid 支撑）；neighborChanged（:199-203）无支撑则移除铁轨。故铁轨须放 stone 上方。
//
// 测试覆盖（2 个场景，覆盖 wiki 红石激活+断电复位核心行为，与红石灯范式一致）：
//   1. 充能激活：激活铁轨（默认 powered=false）相邻放红石块 → POWERED 翻 true（充能激活）。
//   2. 断电复位：激活铁轨已激活（powered=true），移除红石块 → POWERED 翻回 false（断电复位）。
//
// 关键约束：
// 1. 激活铁轨须放 solid 支撑上方——(3,1,1) 放 stone 支撑，铁轨 (3,2,1)。否则 neighborChanged 无支撑
//    自毁（AbstractRailBlock.cpp:199-203）。
// 2. 红石逻辑在 neighborChanged（电平触发，无延迟），放/移红石块走 setBlockState flags=3 → 邻居铁轨
//    neighborChanged → POWERED=shouldBePowered 立即写回。同步触发，pollUntilSucceed 留余量防时序。
// 3. 红石块电源 (4,2,1) 水平相邻铁轨（全向充能 15）。红石块无支撑要求，但 (4,1,1) 放 stone 仅惯例。
// 4. 读 powered state 用 getState("powered" as any) 绕过 BlockStateSuperset 白名单。
// 5. 场景 2 用 runAtTickTime 分阶段编排（先等激活稳定，再移除红石块）。
//
// 不测「动力铁轨（PoweredRail）8 格传导链路」：PoweredRail POWERED 涉 _findPoweredRailSignal 8 格传导，
//   复杂度中等；激活铁轨只查直接红石信号（最简），本文件聚焦激活铁轨。TODO: 待需扩展铁轨传导覆盖时补
//   powered_rail_propagates_signal。
// 不测「探测铁轨（DetectorRail）POWERED」：走 tick() + 矿车实体检测（实体依赖），不可测，跳过。
// 不测「激活铁轨令上方矿车 shaking/减速」：涉矿车实体 AI，非确定，跳过。
//
// 跨服务端：激活铁轨 activator_rail 方块名两端一致，powered state 名两端一致，红石电平激活行为
//   两端一致（立即翻转无延迟）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_铁轨.txt#激活铁轨（红石激活，电平触发）
// Ref: ActivatorRailBlock.cpp（neighborChanged: POWERED=isPowered，立即翻转无延迟）
// Ref: AbstractRailBlock.cpp（isValidPosition 下方 solid 支撑；neighborChanged 无支撑自毁）
// Ref: RedstonePower.cpp（isIndirectlyPowered 遍历六方向强弱信号，同红石灯/门链路）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 激活铁轨 (3,2,1)，下方 (3,1,1) stone 支撑（铁轨需 canSupportRigidBlock 下方），红石块电源 (4,2,1) 水平相邻。

// 读取激活铁轨 powered state（bool）。返回 null 表示读取失败或非激活铁轨。
function getRailPowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 激活铁轨：(3,1,1) stone 支撑，(3,2,1) 激活铁轨（shape=NorthSouth 默认 powered=false）。
function placeActivatorRail(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑（铁轨需 canSupportRigidBlock 下方）
    test.setBlockType("minecraft:activator_rail", { x: 3, y: 2, z: 1 }); // 激活铁轨 powered=false
}

// 场景 1：激活铁轨（默认 powered=false）相邻放红石块 → 充能激活，POWERED 翻 true。
//
// 布局：(3,1,1) stone + (3,2,1) 激活铁轨 + (4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块 flags=3 → 邻居铁轨 neighborChanged → shouldBePowered=isPowered(红石块 15)>0=true !=
//   isCurrentlyPowered(false) → with(POWERED,true) setBlockState（立即翻转无延迟）。
//
// 判定：pollUntilSucceed 轮询 powered===true。
function activatorRailPowersWhenPowered(test: Test): void {
    placeActivatorRail(test);

    // (4,2,1) 放红石块 → 邻居铁轨 neighborChanged → isPowered=true → POWERED=true（充能激活）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 powered === true（neighborChanged 同步触发，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getRailPowered(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `activator_rail powered: should be true when powered, got ${getRailPowered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：激活铁轨已激活（powered=true），移除红石块 → 断电复位，POWERED 翻回 false。
//
// 布局：承接场景 1——铁轨 powered=true（红石块供电），(4,2,1) 设 air。
// air 放置向邻居铁轨 neighborChanged → shouldBePowered=isPowered(air 邻居全 0)=false !=
//   isCurrentlyPowered(true) → with(POWERED,false) setBlockState（立即翻转复位）。
//
// 判定：pollUntilSucceed 轮询 powered===false。
function activatorRailDepowersWhenPowerRemoved(test: Test): void {
    placeActivatorRail(test);
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待铁轨激活（powered=true）后移除电源。
    test.runAtTickTime(5, () => {
        if (getRailPowered(test, 3, 2, 1) !== true) {
            test.assert(false, `activator_rail should be powered before power removal, got powered=${getRailPowered(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块 → 邻居铁轨 neighborChanged → isPowered=false → POWERED=false。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 powered === false（断电复位，恢复未激活）。
    pollUntilSucceed(
        test,
        () => getRailPowered(test, 3, 2, 1) === false,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `activator_rail powered: should be false after power removed, got ${getRailPowered(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerActivatorRailTests(): void {
    GameTest.register("BlockBehaviorTests", "activator_rail_powers_when_powered", activatorRailPowersWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "activator_rail_depowers_when_power_removed", activatorRailDepowersWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
