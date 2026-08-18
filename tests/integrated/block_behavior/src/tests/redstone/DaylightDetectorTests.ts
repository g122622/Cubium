// 日光探测器右键切换昼夜模式行为 GameTest。
//
// wiki tech_阳光探测器.txt#用途：日光探测器根据天空光照输出红石信号。右键切换「昼夜模式」
//   （INVERTED state）：默认模式（inverted=false）白天输出强信号；反相模式（inverted=true）
//   夜间输出强信号（信号反转）。右键不消耗手持物，仅翻转 INVERTED 并重算 power。
//
// C++ 链路：DaylightDetectorBlock（redstone/DaylightDetectorBlock.cpp）有 POWER_0_15 + INVERTED state。
//   - onBlockActivated（已补全）：调 toggleMode 翻转 INVERTED
//     并重算 power + _notifyNeighbors → return Success。不检查手持物（空手右键即可）。
//   - toggleMode（:92-105）：newInverted=!isInverted → withInverted + _calculateSignalStrength 重算 power
//     → setBlockState 写回 + _notifyNeighbors。
//   - 此前 DaylightDetectorBlock 未 override onBlockActivated（基类返 Pass），右键无法切换昼夜模式
//     ——生产 bug，已修复。
//
// 派发链路：SimulatedPlayer::useItemOnBlock 已补全 Block.use 前置分支（先 onBlockActivated，Pass 才
//   fallback onItemUse）。日光探测器 onBlockActivated 始终返 Success（不检查手持物），短路不 fallback。
//   用手持 stick 触发（onBlockActivated 不检查手持物，stick 不被消耗，仅作 useItemOnBlock 必需的
//   ItemStack 形参占位）。
//
// 测试覆盖（2 个场景，覆盖 wiki 右键切换昼夜模式核心行为，可跨服务端对比）：
//   1. 右键切到反相模式：放日光探测器（inverted=false）+ stick useItemOnBlock → inverted=true，返 true。
//   2. 再次右键切回：已 inverted=true + stick useItemOnBlock → inverted=false，返 true。
//
// 关键约束：
// 1. 日光探测器放固体上方（(3,1,1) stone 支撑 + (3,2,1) 日光探测器，minecraft:daylight_detector
//    默认 inverted=false）。
// 2. 读 inverted state 用 getState("inverted" as any) 绕过 BlockStateSuperset 白名单。
// 3. onBlockActivated 不检查手持物，用手持 stick 触发（stick 不被消耗，仅占位 useItemOnBlock 的
//    ItemStack 形参）。任何物品右键都切换。
// 4. toggleMode 会重算 power 并 setBlockState 写回（含 inverted 翻转），判定看 inverted 翻转。
//
// 不测「power 信号强度计算」：power 依赖天空光照/时间/天气（_calculateSignalStrength），GameTest
//   无头环境时间/光照非确定，跳过。TODO: 待时间/光照可控后补反相模式 power 反转测试。
// 不测「比较器/红石线输出」：涉红石传导链路，跳过。
//
// 跨服务端：日光探测器 daylight_detector 方块名两端一致，inverted state 行为两端一致。
//   基岩无 setBlockWithStates，本测试用 setBlockType 放默认 state（inverted=false），两端均可放；
//   右键切换 inverted 行为两端可对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_阳光探测器.txt#用途（右键切换昼夜模式/反相）
// Ref: DaylightDetectorBlock.cpp（onBlockActivated toggleMode→Success；toggleMode 翻转 INVERTED+重算 power）
// Ref: SimulatedPlayer.cpp（useItemOnBlock Block.use 前置分支）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 日光探测器 (3,2,1)，下方 (3,1,1) stone 支撑。

// 读取日光探测器 inverted state（boolean）。返回 null 表示读取失败或非日光探测器。
function getDaylightDetectorInverted(test: Test, x: number, y: number, z: number): boolean | null {
    const block = test.getBlock({ x, y, z });
    if (block === undefined) {
        return null;
    }
    const value = block?.permutation?.getState("inverted" as any);
    return typeof value === "boolean" ? value : null;
}

// 放支撑 + 日光探测器：(3,1,1) stone 支撑，(3,2,1) 日光探测器（默认 inverted=false）。
function placeDaylightDetector(test: Test): void {
    test.setBlockType("minecraft:stone", { x: 3, y: 1, z: 1 }); // 支撑
    test.setBlockType("minecraft:daylight_detector", { x: 3, y: 2, z: 1 }); // 日光探测器 inverted=false
}

// 场景 1：右键切到反相模式——放日光探测器（inverted=false）+ stick useItemOnBlock → inverted=true，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 日光探测器 inverted=false。
// onBlockActivated → toggleMode：newInverted=!false=true → withInverted(true) + 重算 power →
// setBlockState 写回 → return Success。
//
// 判定：useItemOnBlock 返 true（Success），inverted === true（切到反相模式）。
function daylightDetectorTogglesToInvertedMode(test: Test): void {
    placeDaylightDetector(test);
    test.assert(getDaylightDetectorInverted(test, 3, 2, 1) === false, `detector inverted should be false before, got ${getDaylightDetectorInverted(test, 3, 2, 1)}`);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 对日光探测器 useItemOnBlock stick → onBlockActivated toggleMode → inverted false→true → Success。
    const used = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(used, "useItemOnBlock should return true when toggling daylight detector to inverted mode");

    // 判定：inverted === true（切到反相/夜间模式）。
    test.assert(getDaylightDetectorInverted(test, 3, 2, 1) === true, `detector inverted should be true after toggle, got ${getDaylightDetectorInverted(test, 3, 2, 1)}`);

    test.succeed();
}

// 场景 2：再次右键切回——已 inverted=true + stick useItemOnBlock → inverted=false，返 true。
//
// 布局：(3,1,1) stone 支撑 + (3,2,1) 日光探测器（先切换使 inverted=true）。
// onBlockActivated → toggleMode：newInverted=!true=false → withInverted(false) + 重算 power →
// setBlockState 写回 → return Success。
//
// 判定：useItemOnBlock 返 true（Success），inverted === false（切回默认模式）。
function daylightDetectorTogglesBackToDefaultMode(test: Test): void {
    placeDaylightDetector(test);

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const stick = new ItemStack("minecraft:stick", 1);

    // 第一次右键：切到反相模式（inverted false→true）。
    const firstUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(firstUsed, "first toggle should succeed");
    test.assert(getDaylightDetectorInverted(test, 3, 2, 1) === true, `detector inverted should be true after first toggle, got ${getDaylightDetectorInverted(test, 3, 2, 1)}`);

    // 第二次右键：切回默认模式（inverted true→false）。
    const secondUsed = farmer.useItemOnBlock(
        stick as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        { x: 3, y: 2, z: 1 },
        Direction.Up,
    );
    test.assert(secondUsed, "useItemOnBlock should return true when toggling back to default mode");

    // 判定：inverted === false（切回默认/白天模式）。
    test.assert(getDaylightDetectorInverted(test, 3, 2, 1) === false, `detector inverted should be false after second toggle, got ${getDaylightDetectorInverted(test, 3, 2, 1)}`);

    test.succeed();
}

export function registerDaylightDetectorTests(): void {
    GameTest.register("BlockBehaviorTests", "daylight_detector_toggles_to_inverted_mode", daylightDetectorTogglesToInvertedMode)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
    GameTest.register("BlockBehaviorTests", "daylight_detector_toggles_back_to_default_mode", daylightDetectorTogglesBackToDefaultMode)
        .structureName("gametests:glass_pit")
        .maxTicks(60);
}
