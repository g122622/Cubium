// 动力铁轨红石充能激活与信号传导行为 GameTest。
//
// wiki mechanism_铁轨.txt#动力铁轨：动力铁轨是红石机械元件，被红石信号激活时 POWERED=true（激活时
//   加速上方矿车，本测试只验 POWERED state 翻转 + 信号传导）。红石路径是电平触发：有信号→POWERED=true，
//   无信号→POWERED=false（立即翻转无延迟）。
//   - 动力铁轨独有的信号传导：相邻已激活的动力铁轨会把激活状态沿铁轨链传导最多 8 格（_findPoweredRailSignal），
//     使整条无直接电源的铁轨链也被激活。这是动力铁轨区别于激活铁轨（不传导）的核心行为。
//
// C++ 链路：PoweredRailBlock（redstone/PoweredRailBlock.cpp）继承 AbstractRailBlock，有 SHAPE + POWERED +
//   WATERLOGGED 三个 state。默认 state（:64-66）：shape=NorthSouth, powered=false, waterlogged=false。
//   - neighborChanged（:86-110）：`shouldBePowered = RedstonePower::isPowered(world, pos)`（直接红石信号），
//     `if (!shouldBePowered) shouldBePowered = _findPoweredRailSignal(...) || _findPoweredRailSignal(...)`（传导），
//     不等则 `with(POWERED, shouldBePowered) setBlockState(3)` 立即翻转。直接信号优先（:98），无直接信号才查传导。
//   - _findPoweredRailSignal（:116-210）：沿铁轨 shape 方向迭代最多 8 格，查相邻动力铁轨是否已 POWERED=true，
//     找到则返 true（传导激活）。NorthSouth shape 沿 z 轴，EastWest 沿 x 轴。
//   - isPowered 委托 RedstonePower::isPowered→isIndirectlyPowered（同红石灯/激活铁轨链路）。
//   - AbstractRailBlock::isValidPosition 要求下方 canSupportRigidBlock（solid 支撑），无支撑 neighborChanged 自毁。
//
// 测试覆盖（2 个场景，覆盖 wiki 直接充能+信号传导核心行为）：
//   1. 直接充能：单块动力铁轨 + 相邻红石块 → POWERED 翻 true（isPowered 直接分支，与激活铁轨同构）。
//   2. 信号传导：2 块相连动力铁轨（沿 z 轴 NorthSouth），末端相邻红石块激活末端 → 首块经
//      _findPoweredRailSignal 传导 POWERED 翻 true（动力铁轨独有传导链路，首块无直接电源）。
//
// 关键约束：
// 1. 动力铁轨须放 solid 支撑上方——下方 (3,1,*) 放 stone 支撑，否则 neighborChanged 无支撑自毁。
// 2. 场景 2 两铁轨沿 z 轴相连（NorthSouth shape）：铁轨 A (3,2,1) + 铁轨 B (3,2,2)，setBlockType 默认
//    shape=NorthSouth，onBlockAdded updateDir 沿 z 相连保持 NorthSouth 直轨。
// 3. 场景 2 末端 B 相邻红石块 (4,2,2) → B isPowered=true 直接激活 → B POWERED=true（flags=3 通知邻居 A）
//    → A neighborChanged → isPowered(直接)=false → _findPoweredRailSignal 沿 z 查到 B 已 POWERED → A POWERED=true。
// 4. 传导是链式 neighborChanged（B 翻转通知 A），同步触发但 pollUntilSucceed 留余量防时序。
// 5. 读 powered state 用 getState("powered" as any) 绕过 BlockStateSuperset 白名单。
//
// 不测「动力铁轨加速矿车」：涉矿车实体物理，非确定，跳过。
// 不测「8 格最大传导距离边界」：需铺 8+ 块铁轨链，布局复杂且传导时序累积，跳过。本场景 2 块已验证
//   传导链路核心（_findPoweredRailSignal 找到相邻已激活铁轨）。TODO: 待需扩展传导边界覆盖时补。
// 不测「断电复位」：与激活铁轨 depowers 同构（移除红石块 → isPowered=false → POWERED=false），且传导
//   复位依赖链式 neighborChanged 时序，本文件聚焦充能+传导激活。TODO: 可补 powered_rail_depowers。
//
// 跨服务端：动力铁轨 powered_rail 方块名两端一致，powered state 名两端一致，红石激活+信号传导行为
//   两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_铁轨.txt#动力铁轨（红石激活+信号传导）
// Ref: PoweredRailBlock.cpp（neighborChanged: isPowered 直接优先 + _findPoweredRailSignal 传导；立即翻转）
// Ref: PoweredRailBlock.cpp（_findPoweredRailSignal 沿 shape 方向迭代 8 格查相邻已激活铁轨）
// Ref: AbstractRailBlock.cpp（isValidPosition 下方 solid 支撑；neighborChanged 无支撑自毁）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 场景 1：铁轨 (3,2,1)，下方 (3,1,1) stone 支撑，红石块 (4,2,1) 水平相邻。
// 场景 2：铁轨 A (3,2,1) + 铁轨 B (3,2,2)（沿 z 轴 NorthSouth 相连），下方 (3,1,1)/(3,1,2) stone 支撑，
//   红石块 (4,2,2) 水平相邻 B（末端）。

// 读取动力铁轨 powered state（bool）。返回 null 表示读取失败或非动力铁轨。
function getRailPowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：直接充能——单块动力铁轨 + 相邻红石块 → POWERED 翻 true（isPowered 直接分支）。
//
// 布局：(3,1,1) stone + (3,2,1) 动力铁轨 + (4,2,1) 红石块（水平相邻，全向充能 15）。
// 放红石块 flags=3 → 邻居铁轨 neighborChanged → shouldBePowered=isPowered(红石块 15)>0=true（直接信号
// 优先，不走 _findPoweredRailSignal）→ with(POWERED,true) setBlockState 立即翻转。
//
// 判定：pollUntilSucceed 轮询 powered===true。
function poweredRailPowersWhenDirectlyPowered(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:powered_rail", { x: 3, y: 2, z: 1 }); // 动力铁轨 powered=false

    // (4,2,1) 放红石块 → 邻居铁轨 neighborChanged → isPowered(直接)=true → POWERED=true。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    pollUntilSucceed(
        test,
        () => getRailPowered(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `powered_rail powered: should be true when directly powered, got ${getRailPowered(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：信号传导——2 块相连动力铁轨，末端红石块激活末端 → 首块经传导 POWERED 翻 true。
//
// 布局：铁轨 A (3,2,1) + 铁轨 B (3,2,2)（沿 z 轴 NorthSouth 相连），下方 stone 支撑，红石块 (4,2,2) 相邻 B。
// 放红石块 → B neighborChanged → isPowered(红石块)=true → B POWERED=true（flags=3 通知邻居 A）。
// A neighborChanged → isPowered(直接，A 相邻无红石块)=false → _findPoweredRailSignal 沿 z 查到 B 已
// POWERED=true → shouldBePowered=true → A POWERED=true（传导激活，A 无直接电源）。
//
// 判定：pollUntilSucceed 轮询 A (3,2,1) powered===true（经传导激活，非直接电源）。
// 注意：传导是链式 neighborChanged（B 翻转通知 A），同步但留余量防时序。
function poweredRailPropagatesSignalToAdjacentRail(test: Test): void {
    // 支撑 + 两铁轨沿 z 轴相连（NorthSouth 默认 shape）。
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // A 支撑
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 2 }); // B 支撑
    test.setBlockType("minecraft:powered_rail", { x: 3, y: 2, z: 1 }); // 铁轨 A（首块，无直接电源）
    test.setBlockType("minecraft:powered_rail", { x: 3, y: 2, z: 2 }); // 铁轨 B（末端，将相邻红石块）

    // (4,2,2) 放红石块（水平相邻 B）→ B 直接激活 POWERED=true → 传导使 A POWERED=true。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 2 });

    // 轮询断言 A (3,2,1) powered===true（经 _findPoweredRailSignal 传导激活，A 无直接电源）。
    // 传导链式 neighborChanged 同步触发，但留较大余量（startTick=4, maxTick=60）防多级传导时序。
    pollUntilSucceed(
        test,
        () => getRailPowered(test, 3, 2, 1) === true,
        {
            startTick: 4,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `powered_rail A powered: should be true via signal propagation from B, got A=${getRailPowered(test, 3, 2, 1)}, B=${getRailPowered(test, 3, 2, 2)}`);
            },
        },
    );
}

export function registerPoweredRailTests(): void {
    GameTest.register("BlockBehaviorTests", "powered_rail_powers_when_directly_powered", poweredRailPowersWhenDirectlyPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "powered_rail_propagates_signal_to_adjacent_rail", poweredRailPropagatesSignalToAdjacentRail)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
}
