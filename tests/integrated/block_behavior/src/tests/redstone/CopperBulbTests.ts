// 铜灯红石边沿触发与锁存行为 GameTest。
//
// wiki block_铜灯.txt#用途：铜灯是红石可控光源，行为与普通红石灯不同——它是「边沿触发锁存器」：
//   - 收到红石信号的「上升沿」（从无信号到有信号）时，切换 LIT 状态（亮→灭 或 灭→亮）；
//   - 收到信号的「下降沿」（从有信号到无信号）时，不切换 LIT，仅更新 POWERED=false（LIT 锁存保持）；
//   - 因此铜灯在信号消失后保持当前亮灭状态，直到下一次上升沿才翻转。
// 这与红石灯（电平触发：有信号即亮、无信号 4tick 后灭）形成对照。
//
// C++ 链路：CopperBulbBlock 有 lit（bool LIT）与 powered（bool POWERED）两个 state。
//   - updatePostPlacement（CopperBulbBlock.cpp:64-87）：邻居更新触发，读 isBlockPowered（委托
//     RedstonePower::isPowered→isIndirectlyPowered，遍历六方向强弱信号）与 state POWERED 对比：
//       * isPowered != wasPowered（信号变化）：
//         - 上升沿（isPowered=true）：set POWERED=true 且 LIT=!isLit（翻转）；
//         - 下降沿（isPowered=false）：仅 set POWERED=false（LIT 不变，锁存）；
//       * 信号未变：返回原 state。
//   - isBlockPowered（RedstoneSystem.hpp:250-253）委托 RedstonePower::isPowered，与红石灯同链路。
//   - 红石块（RedstoneBlock.cpp）getWeakPower/getStrongPower 全向 15，放置即供电，适合作测试电源。
//
// 电源选择：同 RedstoneLampTests，用红石块（minecraft:redstone_block）作电源。原因：LeverBlock::
//   onBlockAdded 空实现，拉杆放置不传播信号，GameTest 无右键 API 无法 toggle；红石块无 state 切换、
//   放置即全向供电，走 setBlockState flags=3 邻居 updatePostPlacement 链路，能可靠触发铜灯边沿。
//
// 测试覆盖（3 个场景，覆盖 wiki 边沿触发+锁存核心行为，可跨服务端对比）：
//   1. 上升沿点亮：铜灯（默认 lit=false）相邻放红石块 → lit 翻转为 true。
//   2. 下降沿锁存：铜灯已亮（lit=true），移除红石块 → lit 保持 true（POWERED 转 false 但 LIT 锁存）。
//   3. 再上升沿熄灭：铜灯锁存亮（lit=true），再放红石块 → lit 翻转为 false（第二次上升沿翻转）。
//
// 关键约束：
// 1. 铜灯逻辑在 updatePostPlacement（邻居更新触发），放/移红石块走 setBlockState flags=3 → 邻居
//    铜灯 updatePostPlacement → 返回新 state（含 LIT 翻转）由 setBlockState 应用写回。同 tick 同步，
//    用 pollUntilSucceed 轮询留余量防时序。
// 2. 移除红石块（→air）须是非 no-op 写入——先有红石块再设 air，保证真实状态变化派发邻居更新。
// 3. 读 lit state 用 getState("lit" as any) 绕过 BlockStateSuperset 白名单（同红石灯范式）。
// 4. 场景 2/3 用 runAtTickTime 分阶段：先等上升沿点亮稳定，再移除/重放红石块触发下降沿/再上升沿，
//    pollUntilSucceed 轮询最终 LIT。多阶段时序用 waitForCondition 或 runAtTickTime 编排。
//
// 不测「比较器输出」（铜灯 LIT 时比较器输出 15）：需放置比较器+朝向，链路复杂，跳过。
//   TODO: 可补 copper_bulb_comparator_outputs_15_when_lit。
// 不测「涂蜡铜灯防氧化」：属 WeatheringCopperBlock 氧化体系，非边沿触发行为，跳过。
//
// 跨服务端：铜灯 lit/powered state 名两端一致（Java 式 bool），边沿触发+锁存行为与 vanilla 一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\block_铜灯.txt#用途（边沿触发锁存）
// Ref: CopperBulbBlock.cpp（updatePostPlacement 上升沿翻转 LIT / 下降沿锁存）
// Ref: RedstoneSystem.hpp:250-253（isBlockPowered 委托 RedstonePower::isPowered）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,2,1) 作铜灯位、(4,2,1) 作红石块电源位，水平相邻，均在 air 空腔内。

// 读取铜灯 lit state（bool）。返回 null 表示读取失败或非铜灯。
function getBulbLit(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("lit" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：铜灯（默认 lit=false）相邻放红石块 → 上升沿翻转 lit=true。
//
// 布局：(3,2,1) 放铜灯（默认 lit=false），(4,2,1) 放红石块（水平相邻，全向供电 15）。
// 放红石块走 setBlockState flags=3 → 邻居铜灯 updatePostPlacement →
// isPowered(红石块 weakPower 15)>0=true != wasPowered(false) → 上升沿 →
// set POWERED=true 且 LIT=!false=true。
//
// 判定：pollUntilSucceed 轮询 lit===true（updatePostPlacement 同步触发，留余量防时序）。
function copperBulbTogglesLitOnRedstoneRisingEdge(test: Test): void {
    // (3,2,1) 放铜灯（默认 lit=false，无电源时熄灭）。
    test.setBlockType("minecraft:copper_bulb", { x: 3, y: 2, z: 1 });

    // (4,2,1) 放红石块（水平相邻铜灯，getWeakPower 全向 15）。放红石块 flags=3 → 邻居铜灯
    // updatePostPlacement → isPowered=true != wasPowered=false → 上升沿 → LIT 翻转为 true。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 lit === true（updatePostPlacement 同步翻转，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getBulbLit(test, 3, 2, 1) === true,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `copper_bulb lit: should be true after rising edge, got ${getBulbLit(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：铜灯已亮（lit=true），移除红石块 → 下降沿锁存，lit 保持 true。
//
// 布局：(3,2,1) 铜灯 + (4,2,1) 红石块（灯已因上升沿点亮 lit=true, powered=true），再 (4,2,1) 设 air。
// air 放置向邻居铜灯 updatePostPlacement → isPowered(air 邻居全 0)=false != wasPowered(true) →
// 下降沿 → 仅 set POWERED=false，LIT 不变（锁存保持 true）。
//
// 判定：pollUntilSucceed 轮询 lit===true（下降沿不翻转 LIT，锁存保持）。注意此处断言 lit 仍为 true
//   （与红石灯「移除电源 4tick 后熄灭」相反，体现铜灯锁存特性）。
function copperBulbLatchesLitWhenPowerRemoved(test: Test): void {
    // (3,2,1) 放铜灯，（4,2,1）放红石块（灯因上升沿翻转 lit=true）。
    test.setBlockType("minecraft:copper_bulb", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待灯点亮（lit=true）后再移除电源——用 runAtTickTime 在点亮稳定后移除红石块。
    test.runAtTickTime(5, () => {
        // 确认灯已点亮（若未点亮说明上升沿翻转链路异常，断言失败暴露缺陷）。
        if (getBulbLit(test, 3, 2, 1) !== true) {
            test.assert(false, `copper_bulb should be lit before power removal, got ${getBulbLit(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块（红石块→air 真实状态变化，派发邻居更新）。air 放置向邻居铜灯
        // updatePostPlacement → isPowered=false != wasPowered=true → 下降沿 → 仅 POWERED=false，LIT 锁存。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 lit === true（下降沿锁存，LIT 保持）。startTick 留移除后余量。
    pollUntilSucceed(
        test,
        () => getBulbLit(test, 3, 2, 1) === true,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `copper_bulb lit: should latch true after falling edge, got ${getBulbLit(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：铜灯锁存亮（lit=true），再放红石块 → 第二次上升沿翻转 lit=false。
//
// 布局：承接场景 2 终态——铜灯 lit=true（powered=false，电源已移除）。再 (4,2,1) 放红石块。
// 放红石块向邻居铜灯 updatePostPlacement → isPowered=true != wasPowered(false) → 上升沿 →
// LIT=!true=false（第二次翻转，灭）。
//
// 判定：pollUntilSucceed 轮询 lit===false（第二次上升沿翻转熄灭）。
// 此场景验证铜灯「每次上升沿都翻转」的 toggle 特性（非红石灯的电平触发）。
function copperBulbTogglesOffOnSecondRisingEdge(test: Test): void {
    // 阶段 1：放铜灯 + 红石块（第一次上升沿，lit: false→true）。
    test.setBlockType("minecraft:copper_bulb", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 阶段 2：tick 5 移除红石块（下降沿，lit 锁存 true）。
    test.runAtTickTime(5, () => {
        if (getBulbLit(test, 3, 2, 1) !== true) {
            test.assert(false, `copper_bulb should be lit after first rising edge, got ${getBulbLit(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 阶段 3：tick 12 重放红石块（第二次上升沿，lit: true→false）。
    test.runAtTickTime(12, () => {
        // 确认锁存亮（lit=true），否则下降沿锁存链路异常。
        if (getBulbLit(test, 3, 2, 1) !== true) {
            test.assert(false, `copper_bulb should latch lit=true before second rising edge, got ${getBulbLit(test, 3, 2, 1)}`);
            return;
        }
        // 重放红石块 → 第二次上升沿 → LIT 翻转为 false。
        test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 lit === false（第二次上升沿翻转熄灭）。startTick 留第二次上升沿后余量。
    pollUntilSucceed(
        test,
        () => getBulbLit(test, 3, 2, 1) === false,
        {
            startTick: 16,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `copper_bulb lit: should toggle false on second rising edge, got ${getBulbLit(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerCopperBulbTests(): void {
    GameTest.register("BlockBehaviorTests", "copper_bulb_toggles_lit_on_redstone_rising_edge", copperBulbTogglesLitOnRedstoneRisingEdge)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "copper_bulb_latches_lit_when_power_removed", copperBulbLatchesLitWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "copper_bulb_toggles_off_on_second_rising_edge", copperBulbTogglesOffOnSecondRisingEdge)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
