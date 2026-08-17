// 漏斗红石锁定行为 GameTest。
//
// wiki tech_漏斗.txt#红石元件：漏斗是红石元件，行为与多数红石元件相反——
//   - 未被激活时工作（开启，enabled=true，进行输出/吸取/捕捉）；
//   - 被激活时停止（关闭，enabled=false，不进行任何操作）。
// 激活方式：毗邻的开启电源（如红石块）、充能方块、指向漏斗的激活中继器/比较器/红石粉。
// 因此：红石充能 → enabled=false（关闭/锁定）；移除充能 → enabled=true（开启/解锁）。
//
// C++ 链路：HopperBlock（HopperBlock.cpp）有 facing（FACING_EXCEPT_UP，默认 Down）与
//   enabled（ENABLED bool，默认 true）两个 state。
//   - _updateState（:233-241）：`powered = RedstoneSystem::isBlockPowered(world, pos)`，
//     `enabled = !powered`（充能→禁用），若 enabled 变化则 setBlockState 写回（flags=2 无邻居更新防循环）。
//   - onBlockAdded（:89）+ neighborChanged（:94）都调 _updateState。
//   - isBlockPowered 委托 RedstonePower::isPowered→isIndirectlyPowered，遍历六方向强弱信号。
//   - 红石块（RedstoneBlock）getWeakPower/getStrongPower 全向 15，放置即充能，适合作测试电源。
//
// 电源选择：同 RedstoneLampTests/CopperBulbTests，用红石块（minecraft:redstone_block）。原因：
//   LeverBlock::onBlockAdded 空实现，拉杆放置不传播信号，GameTest 无右键 API 无法 toggle；
//   红石块无 state 切换、放置即全向充能，走 setBlockState flags=3 邻居 neighborChanged 链路，
//   能可靠触发漏斗 _updateState。
//
// 测试覆盖（3 个场景，覆盖 wiki 红石锁定核心行为，可跨服务端对比）：
//   1. 充能锁定：漏斗（默认 enabled=true）相邻放红石块 → enabled 翻转为 false（关闭/锁定）。
//   2. 断电解锁：漏斗已锁定（enabled=false），移除红石块 → enabled 翻回 true（开启/解锁）。
//   3. 断电后再次充能锁定：承接场景 2 终态（enabled=true），再放红石块 → enabled=false（验证可重复触发）。
//
// 关键约束：
// 1. 漏斗逻辑在 _updateState（onBlockAdded/neighborChanged 触发），放/移红石块走 setBlockState
//    flags=3 → 邻居漏斗 neighborChanged → _updateState → setBlockState 写回 enabled。同步触发，
//    用 pollUntilSucceed 轮询留余量防时序。
// 2. 移除红石块（→air）须是非 no-op 写入——先有红石块再设 air，保证真实状态变化派发邻居更新。
// 3. 读 enabled state 用 getState("enabled" as any) 绕过 BlockStateSuperset 白名单。
// 4. 场景 2/3 用 runAtTickTime 分阶段：先等锁定稳定，再移除/重放红石块，pollUntilSucceed 轮询最终 enabled。
//
// 不测「物品传输/比较器输出」：需 HopperEntity 方块实体 tick 逻辑 + 容器交互，链路复杂，跳过。
//   TODO: 可补 hopper_transfers_items_between_containers / hopper_comparator_output。
// 不测「朝向随放置面变化」：getStateForPlacement 依赖 clickedFace，GameTest setBlockType 不走放置上下文，
//   漏斗默认 facing=Down，朝向测试需 SimulatedPlayer 放置，跳过。
//
// 跨服务端：漏斗 enabled state 名两端一致（Java 式 bool），红石锁定行为两端一致。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_漏斗.txt#红石元件（未激活=开启/工作，激活=关闭/停止）
// Ref: HopperBlock.cpp（_updateState: enabled=!isBlockPowered，充能锁定/断电解锁）
// Ref: RedstoneSystem::isBlockPowered 委托 RedstonePower::isPowered（同红石灯/铜灯链路）
// Ref: RedstoneBlock.cpp（getWeakPower/getStrongPower 全向 15 持续电源）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../utils/test/poll.js";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// y=0 为 glass 底，y=1..2 为玻璃墙围出的 air 空腔。方块测试在内部 air 层操作。
// 测试用 (3,2,1) 作漏斗位、(4,2,1) 作红石块电源位，水平相邻，均在 air 空腔内。

// 读取漏斗 enabled state（bool）。返回 null 表示读取失败或非漏斗。
// enabled=true 表示开启（工作），enabled=false 表示关闭（被红石锁定）。
function getHopperEnabled(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("enabled" as any);
    return typeof value === "boolean" ? value : null;
}

// 场景 1：漏斗（默认 enabled=true）相邻放红石块 → 充能锁定，enabled 翻转为 false。
//
// 布局：(3,2,1) 放漏斗（默认 enabled=true，未充能=开启=工作），(4,2,1) 放红石块（水平相邻，全向充能 15）。
// 放红石块走 setBlockState flags=3 → 邻居漏斗 neighborChanged → _updateState →
// isBlockPowered(红石块 weakPower 15)>0=true → enabled=!true=false（关闭/锁定）。
//
// 判定：pollUntilSucceed 轮询 enabled===false（_updateState 同步触发，留余量防时序）。
function hopperLocksWhenPowered(test: Test): void {
    // (3,2,1) 放漏斗（默认 enabled=true，开启/工作）。
    test.setBlockType("minecraft:hopper", { x: 3, y: 2, z: 1 });

    // (4,2,1) 放红石块（水平相邻漏斗，getWeakPower 全向 15）。放红石块 flags=3 → 邻居漏斗
    // neighborChanged → _updateState → isBlockPowered=true → enabled=false（关闭/锁定）。
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 轮询断言 enabled === false（_updateState 同步锁定，pollUntilSucceed 留余量）。
    pollUntilSucceed(
        test,
        () => getHopperEnabled(test, 3, 2, 1) === false,
        {
            startTick: 2,
            interval: 4,
            maxTick: 40,
            onTimeout: () => {
                test.assert(false, `hopper enabled: should be false when powered (locked), got ${getHopperEnabled(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 2：漏斗已锁定（enabled=false），移除红石块 → 断电解锁，enabled 翻回 true。
//
// 布局：(3,2,1) 漏斗 + (4,2,1) 红石块（漏斗已因充能锁定 enabled=false），再 (4,2,1) 设 air。
// air 放置向邻居漏斗 neighborChanged → _updateState → isBlockPowered(air 邻居全 0)=false →
// enabled=!false=true（开启/解锁）。
//
// 判定：pollUntilSucceed 轮询 enabled===true（断电解锁，恢复开启）。
function hopperUnlocksWhenPowerRemoved(test: Test): void {
    // (3,2,1) 放漏斗，（4,2,1）放红石块（漏斗因充能锁定 enabled=false）。
    test.setBlockType("minecraft:hopper", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 等待漏斗锁定（enabled=false）后再移除电源——用 runAtTickTime 在锁定稳定后移除红石块。
    test.runAtTickTime(5, () => {
        // 确认漏斗已锁定（若未锁定说明充能锁定链路异常，断言失败暴露缺陷）。
        if (getHopperEnabled(test, 3, 2, 1) !== false) {
            test.assert(false, `hopper should be locked before power removal, got enabled=${getHopperEnabled(test, 3, 2, 1)}`);
            return;
        }
        // (4,2,1) 设 air 移除红石块（红石块→air 真实状态变化，派发邻居更新）。air 放置向邻居漏斗
        // neighborChanged → _updateState → isBlockPowered=false → enabled=true（开启/解锁）。
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 enabled === true（断电解锁，恢复开启）。startTick 留移除后余量。
    pollUntilSucceed(
        test,
        () => getHopperEnabled(test, 3, 2, 1) === true,
        {
            startTick: 12,
            interval: 4,
            maxTick: 50,
            onTimeout: () => {
                test.assert(false, `hopper enabled: should be true after power removed (unlocked), got ${getHopperEnabled(test, 3, 2, 1)}`);
            },
        },
    );
}

// 场景 3：漏斗解锁（enabled=true）后再次充能 → 再次锁定 enabled=false（验证可重复触发）。
//
// 布局：承接场景 2 终态——漏斗 enabled=true（电源已移除）。再 (4,2,1) 放红石块。
// 放红石块向邻居漏斗 neighborChanged → _updateState → isBlockPowered=true → enabled=false（再次锁定）。
//
// 判定：pollUntilSucceed 轮询 enabled===false（再次充能锁定）。
// 此场景验证漏斗锁定/解锁可重复触发（_updateState 每次 neighborChanged 都重算）。
function hopperRelocksWhenRepowered(test: Test): void {
    // 阶段 1：放漏斗 + 红石块（充能锁定，enabled: true→false）。
    test.setBlockType("minecraft:hopper", { x: 3, y: 2, z: 1 });
    test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });

    // 阶段 2：tick 5 移除红石块（断电解锁，enabled: false→true）。
    test.runAtTickTime(5, () => {
        if (getHopperEnabled(test, 3, 2, 1) !== false) {
            test.assert(false, `hopper should be locked after first power, got enabled=${getHopperEnabled(test, 3, 2, 1)}`);
            return;
        }
        test.setBlockType("minecraft:air", { x: 4, y: 2, z: 1 });
    });

    // 阶段 3：tick 12 重放红石块（再次充能锁定，enabled: true→false）。
    test.runAtTickTime(12, () => {
        // 确认已解锁（enabled=true），否则断电解锁链路异常。
        if (getHopperEnabled(test, 3, 2, 1) !== true) {
            test.assert(false, `hopper should be unlocked before re-power, got enabled=${getHopperEnabled(test, 3, 2, 1)}`);
            return;
        }
        // 重放红石块 → 再次充能 → enabled=false（再次锁定）。
        test.setBlockType("minecraft:redstone_block", { x: 4, y: 2, z: 1 });
    });

    // 轮询断言 enabled === false（再次充能锁定）。startTick 留第二次充能后余量。
    pollUntilSucceed(
        test,
        () => getHopperEnabled(test, 3, 2, 1) === false,
        {
            startTick: 16,
            interval: 4,
            maxTick: 60,
            onTimeout: () => {
                test.assert(false, `hopper enabled: should relock false when repowered, got ${getHopperEnabled(test, 3, 2, 1)}`);
            },
        },
    );
}

export function registerHopperTests(): void {
    GameTest.register("BlockBehaviorTests", "hopper_locks_when_powered", hopperLocksWhenPowered)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "hopper_unlocks_when_power_removed", hopperUnlocksWhenPowerRemoved)
        .structureName("gametests:glass_pit")
        .maxTicks(100);
    GameTest.register("BlockBehaviorTests", "hopper_relocks_when_repowered", hopperRelocksWhenRepowered)
        .structureName("gametests:glass_pit")
        .maxTicks(120);
}
