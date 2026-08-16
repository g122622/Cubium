// 红石比较器右键切换比较/减法模式行为 GameTest。
//
// wiki mechanism_红石比较器.txt#（使用键切换）：比较器前端火把状态可由使用键切换——
//   - 火把熄灭 → 比较模式（compare）：比较主输入与侧输入，主输入≥侧输入则输出主输入，否则输出 0。
//   - 火把亮起 → 减法模式（subtract）：输出 = 主输入 − 侧输入（最小 0）。
//   右键在两种模式间切换。默认放置为比较模式（mode=compare）。
//
// C++ 链路：RedstoneComparatorBlock（redstone/RedstoneComparatorBlock.cpp）有 MODE state（ComparatorMode
//   枚举，默认 Compare）。
//   - onBlockActivated（:304）：!mayBuild → Pass；否则 newMode = (cur==Compare)?Subtract:Compare
//     （compare↔subtract 翻转）→ withMode setBlockState 写回 + 点击音效（subtract 音高 0.55，compare 0.5）
//     + updateState 立即触发状态检查 → return Success。不检查手持物（空手/任意物品右键都切模式）。
//   - MODE state 名 "mode"，值 "compare"/"subtract"（EnumProperty<ComparatorMode>，见 :58-84）。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。比较器 onBlockActivated 创造模式 mayBuild=true → 切模式返 Success 短路。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅占位 useItemOnBlock 的
//   ItemStack 形参）。
//
// 测试覆盖（2 个场景，覆盖 wiki 右键切换模式 + 翻转回比较模式核心确定行为）：
//   1. 右键切到减法模式：放比较器（mode=compare）+ stick useItemOnBlock → mode=subtract，返 true。
//   2. 再次右键切回比较模式：已 mode=subtract + stick useItemOnBlock → mode=compare，返 true。
//
// 关键约束：
// 1. 比较器 noCollision().notSolid()，Diode 无 canSurvive 自毁逻辑（同中继器），悬空放置不自毁。
//    仍放 (3,1,1) stone 支撑 + (3,2,1) 比较器，贴近真实放置且与同目录红石测试惯例一致。
// 2. setBlockType 放比较器带默认 state（facing=North, powered=false, mode=compare）。
//    右键切模式不依赖 facing，默认 North 即可。
// 3. 读 mode state 用 getState("mode" as any) 绕过 BlockStateSuperset 白名单。mode 是字符串枚举
//    （"compare"/"subtract"），非数字。
// 4. 创造模式 SimulatedPlayer mayBuild=true，onBlockActivated 走切模式分支（非 Pass）。
// 5. 切模式后 onBlockActivated 调 updateState（无信号输入时输出稳定为 0，不影响 mode state 断言）。
// 6. 切模式不消耗手持物（onBlockActivated 无 shrink），stick 可重复使用。
//
// 不测「比较/减法模式信号计算」：涉红石传导 + 容器/物品框信号源，复杂且本组聚焦右键切模式 state
//   变化，跳过。TODO: 待比较器信号源测试完善后补 compare/subtract 输出差异测试。
// 不测「朝向」：setBlockType 放默认 North，右键切模式不读 facing，跳过。
//
// 跨服务端：比较器 comparator 方块名两端一致，mode state 名两端一致（compare/subtract），右键切换
//   模式行为与 vanilla 一致。基岩无 setBlockWithStates，本测试用 setBlockType 放默认 state
//   （mode=compare），两端均可放；右键切 mode 行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mechanism_红石比较器.txt#（使用键切换前火把：比较↔减法模式）
// Ref: RedstoneComparatorBlock.cpp（onBlockActivated newMode 翻转+音效+updateState→Success；MODE_PROP state）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支，对齐网络层/vanilla Java 1.21）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 比较器 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取比较器 mode state（"compare" | "subtract"）。返回 null 表示读取失败或非比较器。
// mode state 名 "mode"，值 "compare"/"subtract"（EnumProperty<ComparatorMode>，见 RedstoneComparatorBlock.cpp:58-84）。
function getComparatorMode(test: Test, x: number, y: number, z: number): string | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("mode" as any);
    return typeof value === "string" ? value : null;
}

// 放支撑 + 比较器：(3,1,1) stone 支撑，(3,2,1) 比较器（minecraft:comparator 默认 mode=compare, facing=North）。
function placeComparator(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:comparator", { x: 3, y: 2, z: 1 }); // 比较器 mode=compare
}

// 场景 1：右键切到减法模式——放比较器（mode=compare）+ stick useItemOnBlock → mode=subtract，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 比较器 mode=compare。
// onBlockActivated：mayBuild=true → newMode=(compare==Compare)?Subtract=Subtract → withMode setBlockState
//   + 音效（0.55）+ updateState → return Success。
//
// 判定：useItemOnBlock 返 true（Success），mode === "subtract"（切到减法模式）。
function comparatorModeTogglesToSubtractOnUse(test: Test): void {
    placeComparator(test);
    test.assert(getComparatorMode(test, 3, 2, 1) === "compare", `comparator mode should be compare before, got ${getComparatorMode(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对比较器 useItemOnBlock stick → onBlockActivated newMode=Subtract → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when toggling comparator to subtract mode");

    // 判定：mode === "subtract"（切到减法模式）。
    test.assert(getComparatorMode(test, 3, 2, 1) === "subtract", `comparator mode should be subtract after toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：再次右键切回比较模式——已 mode=subtract + stick useItemOnBlock → mode=compare，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 比较器（先切换使 mode=subtract）。
// onBlockActivated：newMode=(subtract==Compare)?...:Compare=Compare → withMode setBlockState + 音效（0.5）
//   + updateState → return Success。
//
// 判定：useItemOnBlock 返 true（Success），mode === "compare"（切回比较模式）。
function comparatorModeTogglesBackToCompareOnUse(test: Test): void {
    placeComparator(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 第一次右键：切到减法模式（mode compare→subtract）。
    const firstUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstUsed, "first toggle should succeed");
    test.assert(getComparatorMode(test, 3, 2, 1) === "subtract", `comparator mode should be subtract after first toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    // 第二次右键：切回比较模式（mode subtract→compare）。
    const secondUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(secondUsed, "useItemOnBlock should return true when toggling comparator back to compare mode");

    // 判定：mode === "compare"（切回比较模式）。
    test.assert(getComparatorMode(test, 3, 2, 1) === "compare", `comparator mode should be compare after second toggle, got ${getComparatorMode(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerComparatorTests(): void {
    GameTest.register("BlockBehaviorTests", "comparator_mode_toggles_to_subtract_on_use", comparatorModeTogglesToSubtractOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "comparator_mode_toggles_back_to_compare_on_use", comparatorModeTogglesBackToCompareOnUse)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
