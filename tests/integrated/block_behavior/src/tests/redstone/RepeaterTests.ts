// 红石中继器右键切换延迟档位行为 GameTest。
//
// wiki tech_红石中继器.txt#延迟信号：红石中继器共有 4 个档位，延迟分别为 2/4/6/8 游戏刻，默认 1 档
//   （2gt）。可对中继器按下使用键改变档位：每次点击 +1 档（延迟 +2gt），最大 4 档（8gt），再点击
//   重置为 1 档。循环顺序 1→2→3→4→1。
//
// C++ 链路：RedstoneRepeaterBlock（redstone/RedstoneRepeaterBlock.cpp）有 DELAY_1_4 state（默认 1）。
//   - onBlockActivated（:144）：!mayBuild → Pass；isLockedState → Pass（锁定时不调档）；否则
//     newDelay = (currentDelay % MAX_DELAY) + 1（1→2→3→4→1）→ withDelay setBlockState 写回 +
//     点击音效 → return Success。不检查手持物（空手/任意物品右键都切档）。
//   - DELAY state 名 "delay"（Properties.hpp DELAY_1_4 = IntegerProperty("delay",1,4)）。
//   - 锁定（LOCKED=true）时右键返 Pass 不切档——本组不构造锁定场景（需侧面二极管信号，复杂）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。中继器 onBlockActivated 创造模式 mayBuild=true + 未锁定 → 切档返 Success 短路。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的
//   ItemStack 形参）。
//
// 测试覆盖（2 个场景，覆盖 wiki 右键切档 + 循环重置核心确定行为）：
//   1. 右键切档递增：放中继器（delay=1）+ stick useItemOnBlock → delay=2，返 true。
//   2. 循环重置到 1 档：连续 4 次 useItemOnBlock（delay 1→2→3→4→1）→ 第 4 次后 delay=1（4 档重置）。
//
// 关键约束：
// 1. 中继器 noCollision().notSolid()，Diode 无 canSurvive 自毁逻辑（onBlockAdded 仅通知邻居，
//    updatePostPlacement 仅调度 powered 更新），悬空放置不自毁。但仍放 (3,1,1) stone 支撑 + (3,2,1)
//    中继器，贴近真实放置且与同目录红石测试惯例一致。
// 2. setBlockType 放中继器带默认 state（facing=North, powered=false, delay=1, locked=false）。
//    右键切档不依赖 facing，默认 North 即可。
// 3. 读 delay state 用 getState("delay" as any) 绕过 BlockStateSuperset 白名单。
// 4. 创造模式 SimulatedPlayer mayBuild=true，onBlockActivated 走切档分支（非 Pass）。locked=false（
//    无侧面信号），不触发 isLockedState 守卫。
// 5. 切档不消耗手持物（onBlockActivated 无 shrink），stick 可重复使用；循环场景每次仍 new ItemStack
//    重新设入选中槽（与重生锚充能范式一致，避免选中槽状态漂移）。
// 6. 切档后 setBlockState(flags=3) 触发邻居更新 + 计划刻，但无信号输入 shouldPower=false=isPowered，
//    不调度亮灭，delay state 稳定可断言。
//
// 不测「锁定时右键不切档」：构造锁定需侧面二极管信号链路，复杂且涉红石传导，跳过。TODO: 待锁定
//   链路测试完善后补。
// 不测「延迟时序（2/4/6/8gt）」：涉计划刻 + 红石传导链路，非本组聚焦（右键切档 state 变化），跳过。
// 不测「朝向」：setBlockType 放默认 North，右键切档不读 facing，跳过。
//
// 跨服务端：中继器 repeater 方块名两端一致，delay state 名两端一致，右键切档 1→2→3→4→1 循环行为
//   与 vanilla 一致。基岩无 setBlockWithStates，本测试用 setBlockType 放默认 state（delay=1），
//   两端均可放；右键切 delay 行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_红石中继器.txt#延迟信号（4 档 2/4/6/8gt，右键 +1，4 档重置 1 档）
// Ref: RedstoneRepeaterBlock.cpp（onBlockActivated newDelay=(cur%4)+1 切档+音效→Success；DELAY_1_4 state）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 中继器 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取中继器 delay state（number 1-4）。返回 null 表示读取失败或非中继器。
// delay state 名 "delay"（Java 命名，见 Properties.hpp DELAY_1_4 = IntegerProperty("delay",1,4)）。
function getRepeaterDelay(test: Test, x: number, y: number, z: number): number | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("delay" as any);
    return typeof value === "number" ? value : null;
}

// 放支撑 + 中继器：(3,1,1) stone 支撑，(3,2,1) 中继器（minecraft:repeater 默认 delay=1, facing=North）。
function placeRepeater(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:repeater", { x: 3, y: 2, z: 1 }); // 中继器 delay=1
}

// 场景 1：右键切档递增——放中继器（delay=1）+ stick useItemOnBlock → delay=2，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 中继器 delay=1。
// onBlockActivated：mayBuild=true + locked=false → newDelay=(1%4)+1=2 → withDelay(2) setBlockState → Success。
//
// 判定：useItemOnBlock 返 true（Success），delay === 2（切到 2 档）。
function repeaterDelayCyclesUpOnUse(test: Test): void {
    placeRepeater(test);
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should be 1 before, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对中继器 useItemOnBlock stick → onBlockActivated newDelay=(1%4)+1=2 → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when cycling repeater delay up");

    // 判定：delay === 2（切到 2 档）。
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 2, `repeater delay should be 2 after one click, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：循环重置到 1 档——连续 4 次 useItemOnBlock（delay 1→2→3→4→1）→ 第 4 次后 delay=1。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 中继器 delay=1（已放）。
// 每次右键 newDelay=(cur%4)+1：1→2→3→4→1。第 4 次从 4 档重置回 1 档（4 档后循环回起点）。
//
// 判定：4 次点击后 delay === 1（4 档循环重置回 1 档）。
function repeaterDelayResetsAfterMaxOnUse(test: Test): void {
    placeRepeater(test);
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should be 1 before, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");

    // 连续 4 次右键切档：delay 1→2→3→4→1。每次 new ItemStack 重新设入选中槽（防选中槽漂移）。
    const expectedSequence = [2, 3, 4, 1]; // 1→2→3→4→1
    for (let i = 0; i < 4; ++i) {
        const stick = new ItemStack("minecraft:stick", 1);
        const used = farmer.useItemOnBlock(
            stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
            { x: 3, y: 2, z: 1 },
            Direction.Up,
        );
        test.assert(used, `useItemOnBlock should return true on click #${i + 1} (delay cycle)`);
        const expected = expectedSequence[i];
        test.assert(getRepeaterDelay(test, 3, 2, 1) === expected, `repeater delay should be ${expected} after click #${i + 1}, got ${getRepeaterDelay(test, 3, 2, 1)}`);
    }

    // 判定：delay === 1（4 档循环重置回 1 档）。
    test.assert(getRepeaterDelay(test, 3, 2, 1) === 1, `repeater delay should reset to 1 after 4 clicks, got ${getRepeaterDelay(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerRepeaterTests(): void {
    GameTest.register("BlockBehaviorTests", "repeater_delay_cycles_up_on_use", repeaterDelayCyclesUpOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "repeater_delay_resets_after_max_on_use", repeaterDelayResetsAfterMaxOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
