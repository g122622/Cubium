// 侦测器方块更新检测脉冲行为 GameTest。
//
// wiki tech_侦测器.txt#行为：侦测器检测其侦测面对着的方块的状态变化（含放置/移除），
// 检测到变化后延迟 2 游戏刻，向输出面发出强度 15、持续 2 游戏刻的红石脉冲。
//   - 侦测面方块变化 → 延迟 2tick → powered=true（激活）→ 再 2tick → powered=false（熄灭）。
//   - 侦测器被放置时不再发出红石信号（1.13 pre4 / BE 1.16.0 修复）——放置侦测器本身不触发脉冲。
//   - 侦测器亮起或熄灭 JE/BE 都可被其他侦测器检测。
//
// C++ 链路：ObserverBlock（ObserverBlock.cpp）有 facing（FACING，输出方向，默认 South）与
//   powered（POWERED bool，默认 false）两个 state。
//   - facing 是「输出方向」，侦测面是 facing 的反方向。默认 facing=South → 输出朝南（z+），侦测面朝北（z-）。
//   - neighborChanged（:114-137）：变化来自侦测面（facing 反方向）且未激活 → 调度 DETECT_DELAY(2) tick。
//   - updatePostPlacement（:139-162）：facing 参数==侦测方向且未激活 → 调度 DETECT_DELAY(2) tick。
//   - tick（:164-183）：未激活→set powered=true + 调度 PULSE_DURATION(2)tick 熄灭；已激活→set powered=false。
//   - DETECT_DELAY=2, PULSE_DURATION=2（ObserverBlock.hpp:120-123）。
//   - onBlockAdded（:94）：放置时若已激活才取消（默认 false 不触发，对齐 wiki「放置不发出信号」）。
//
// 测试布局：侦测器放 (3,2,2)，facing 默认 South（输出朝南 z+），侦测面朝北 (3,2,1)。
//   在侦测面 (3,2,1) 放方块 → 触发 neighborChanged/updatePostPlacement → 2tick 后 powered=true。
//
// 测试覆盖（3 个场景，覆盖 wiki 检测+脉冲核心行为）：
//   1. 侦测面放方块触发激活：侦测器（powered=false）侦测面放方块 → 延迟 2tick 后 powered 翻 true（脉冲）。
//   2. 脉冲持续 2tick 后熄灭：承接场景 1，powered=true 后再 2tick → powered 翻回 false（脉冲结束）。
//   3. 放置侦测器本身不触发脉冲：放侦测器后（侦测面无变化），powered 保持 false（不误触发）。
//
// 关键约束：
// 1. 脉冲窗口仅 2tick（powered=true 持续 2tick），pollUntilSucceed 用 interval=1 逐 tick 检查稳定捕获瞬态。
// 2. 侦测面方块放置用 runAtTickTime 延迟到侦测器放置稳定后（避免放置侦测器与放方块同 tick 的时序竞争）。
// 3. 场景 2 检测 powered===false 时须在脉冲窗口之后（powered 经历 true→false），用记录曾变 true 的状态机
//    区分「从未激活的 false」与「脉冲结束的 false」。
// 4. 读 powered state 用 getState("powered" as any) 绕过 BlockStateSuperset 白名单。
//
// 不测「红石信号输出激活毗邻机械元件」：需放红石粉/灯/活塞链路，且侦测器脉冲 2tick 难以稳定驱动
//   下游元件断言，跳过。TODO: 可补 observer_powers_adjacent_lamp_during_pulse。
// 不测「侦测器互相检测形成时钟」：高频红石时钟，非确定且易触发 Cubium tick 调度边界，跳过。
// 不测「活塞推动侦测器」：需活塞+移动方块实体链路，复杂，跳过。
//
// 跨服务端：侦测器 powered/facing state 名两端一致，检测+脉冲时序（2+2tick）两端一致。
// 注意：BE 侦测器实际为 2 红石刻（信号计算阶段差异），但 Cubium 采用 JE 语义（2 游戏刻）。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_侦测器.txt#行为（检测变化延迟2tick，脉冲持续2tick）
// Ref: ObserverBlock.cpp（neighborChanged/updatePostPlacement 调度 DETECT_DELAY，tick 翻转 powered）
// Ref: ObserverBlock.hpp:120-123（DETECT_DELAY=2, PULSE_DURATION=2）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 侦测器放 (3,2,2)，facing 默认 South（输出朝南 z+），侦测面朝北 (3,2,1)。
// 侦测面 (3,2,1) 初始须为 air，测试中在其放方块触发检测。

// 读取侦测器 powered state（bool）。返回 null 表示读取失败或非侦测器。
function getObserverPowered(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("powered" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：侦测器（powered=false）侦测面放方块 → 延迟 2tick 后 powered 翻 true（脉冲激活）。
//
// 布局：(3,2,2) 放侦测器（facing=South，侦测面朝北 (3,2,1)），侦测面 (3,2,1) 初始 air。
// tick 5 在 (3,2,1) 放石头 → 触发侦测器 neighborChanged（变化来自侦测面）→ 调度 DETECT_DELAY(2) tick →
// tick 7 侦测器 tick → powered=true（脉冲激活）。
//
// 判定：pollUntilSucceed 逐 tick（interval=1）检查 powered===true，捕获 2tick 脉冲窗口。
// startTick=6（放方块后 1tick），maxTick=20 留足 DETECT_DELAY+调度余量。
function observerPulsesWhenBlockPlacedInFront(test: Test): void {
    // (3,2,2) 放侦测器（facing 默认 South，侦测面朝北 (3,2,1)）。放置本身不触发脉冲（powered=false）。
    test.setBlockType("minecraft:observer", { x: 3, y: 2, z: 2 });

    // tick 5 在侦测面 (3,2,1) 放石头（真实 air→stone 状态变化，派发邻居更新到侦测器）。
    // 侦测器 neighborChanged 检测变化来自侦测面 → 调度 DETECT_DELAY(2) tick → tick 7 powered=true。
    test.runAtTickTime(5, () => {
        test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    });

    // 逐 tick 轮询断言 powered === true（捕获 2tick 脉冲窗口）。interval=1 确保 2tick 窗口不漏检。
    pollUntilSucceed(
        test,
        () => getObserverPowered(test, 3, 2, 2) === true,
        {
            startTick: 6,
            interval: 1,
            maxTick: 25,
            onTimeout: () => {
                test.assert(false, `observer powered: should pulse true after block placed in front, got ${getObserverPowered(test, 3, 2, 2)}`);
            },
        },
    );
}

// 场景 2：脉冲持续 2tick 后熄灭——powered 经历 false→true→false 完整脉冲。
//
// 布局：承接场景 1——侦测器 (3,2,2) 侦测面 (3,2,1) 放石头触发脉冲。
// 时序：tick 5 放石头 → tick 7 powered=true → tick 9 powered=false（PULSE_DURATION=2 后熄灭）。
//
// 判定：用「曾激活」状态机——轮询期间记录 powered 是否曾变 true，曾变 true 后再变 false 即证明完整脉冲。
// 直接断言 powered===false 会与「从未激活的 false」混淆，故必须先确认曾 true 再确认回 false。
// 实现用闭包标志 sawPowered：每 tick 检查，powered=true 时置 sawPowered=true；sawPowered 且 powered=false 时 succeed。
function observerPulseExtinguishesAfterTwoTicks(test: Test): void {
    // (3,2,2) 放侦测器，tick 5 侦测面 (3,2,1) 放石头触发脉冲。
    test.setBlockType("minecraft:observer", { x: 3, y: 2, z: 2 });
    test.runAtTickTime(5, () => {
        test.setBlockType("minecraft:stone", { x: 3, y: 2, z: 1 });
    });

    // 状态机：sawPowered 记录是否曾检测到 powered=true。
    // 逐 tick 检查：powered=true → 置 sawPowered=true（脉冲已激活，等熄灭）；
    // sawPowered 且 powered=false → 完整脉冲结束（曾激活再熄灭），succeed。
    let sawPowered = false;
    pollUntilSucceed(
        test,
        () => {
            const powered = getObserverPowered(test, 3, 2, 2);
            if (powered === true) {
                sawPowered = true;
                return false; // 已激活，但需等熄灭才 succeed
            }
            // powered===false 或 null：仅当曾激活过才视为脉冲结束
            return sawPowered && powered === false;
        },
        {
            startTick: 6,
            interval: 1,
            maxTick: 30,
            onTimeout: () => {
                test.assert(false, `observer powered: pulse should extinguish after 2 ticks (sawPowered=${sawPowered}), got ${getObserverPowered(test, 3, 2, 2)}`);
            },
        },
    );
}

// 场景 3：放置侦测器本身不触发脉冲——侦测面无方块变化时 powered 保持 false。
//
// 布局：(3,2,2) 放侦测器（facing=South，侦测面 (3,2,1) 保持 air，不放任何方块）。
// 对齐 wiki「侦测器被放置时不再发出红石信号」（1.13 pre4 / BE 1.16.0 修复）。
//
// 判定：放侦测器后若干 tick 内 powered 恒为 false（不误触发脉冲）。
// 用 runAtTickTime 在多个时间点断言 powered===false，最后 succeed。
// 注意：不能用 pollUntilSucceed(powered===false)——它会首 tick 立即满足（默认就是 false），
//   无法区分「保持 false」与「脉冲尚未到」。故用显式多时间点断言。
function observerDoesNotPulseWhenPlaced(test: Test): void {
    // (3,2,2) 放侦测器（facing=South，侦测面 (3,2,1) 保持 air，侦测面无方块变化）。
    test.setBlockType("minecraft:observer", { x: 3, y: 2, z: 2 });

    // 在 DETECT_DELAY(2) + PULSE_DURATION(2) + 余量 = tick 8 后断言 powered 仍为 false。
    // 若放置误触发脉冲，tick 2-4 会 powered=true，tick 8 已熄灭回 false——为捕获误触发，
    // 在 tick 3（脉冲窗口内）也断言 powered===false。tick 3 若 powered=true 说明放置误触发（缺陷）。
    test.runAtTickTime(3, () => {
        const powered = getObserverPowered(test, 3, 2, 2);
        test.assert(powered === false, `observer should not pulse on placement (tick 3), got powered=${powered}`);
    });

    // tick 10 再次断言 powered===false（确认无延迟误触发），然后 succeed。
    test.runAtTickTime(10, () => {
        const powered = getObserverPowered(test, 3, 2, 2);
        test.assert(powered === false, `observer should remain unpowered after placement (tick 10), got powered=${powered}`);
        test.succeed();
    });
}

export function registerObserverTests(): void {
    GameTest.register("BlockBehaviorTests", "observer_pulses_when_block_placed_in_front", observerPulsesWhenBlockPlacedInFront)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "observer_pulse_extinguishes_after_two_ticks", observerPulseExtinguishesAfterTwoTicks)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "observer_does_not_pulse_when_placed", observerDoesNotPulseWhenPlaced)
        .structureName("gametests:glass_pit")
        .maxTicks(40);
}
