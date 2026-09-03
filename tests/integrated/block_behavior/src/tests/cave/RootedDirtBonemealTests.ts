// 缠根泥土（rooted_dirt）骨粉行为 GameTest：验证骨粉在缠根泥土下方生成垂根。
//
// wiki tech_缠根泥土.txt：
//   "对缠根泥土使用骨粉会在其下方生成一团垂根。"
//   垂根（hanging_roots）是悬挂在方块下方的根系装饰，可被岩浆点燃（对齐 vanilla）。
//
// ============================ Java 版权威真相源 ============================
// net.minecraft.world.level.block.RootedDirtBlock（1.21.11）：
//   - isValidBonemealTarget(LevelReader, BlockPos, BlockState):
//       return levelReader.getBlockState(pos.below()).isAir();
//     —— 下方为空气时骨粉有效。
//   - isBonemealSuccess(Level, RandomSource, BlockPos, BlockState): return true;
//     —— 骨粉 100% 成功。
//   - performBonemeal(ServerLevel, RandomSource, BlockPos, BlockState):
//       serverLevel.setBlockAndUpdate(pos.below(), Blocks.HANGING_ROOTS.defaultBlockState());
//     —— 在下方放置垂根。
//   - getParticlePos(BlockPos): return pos.below();
//     —— 骨粉粒子从下方位置发出。
//
// ============================ Cubium 实现链路 ============================
// RootedDirtBlock（cave/RootedDirtBlock.cpp）继承 Block + IGrowable：
//   - canGrow: 下方为空气时返回 true（对齐 Java isValidBonemealTarget）。
//   - canUseBonemeal: 恒 true（对齐 Java isBonemealSuccess）。
//   - grow: 在下方空气位置放置 hanging_roots（对齐 Java performBonemeal）。
//     flags=3 同步派发邻居更新 + 客户端同步。
//
// BoneMealItem::onItemUse（src/common/item/items/special/BoneMealItem.cpp:61-123）：
//   - dynamic_cast<IGrowable> 取 RootedDirtBlock
//   - canGrow(下方 air) → canUseBonemeal(恒 true) → grow(下方放垂根)
//   - grow 同步 setBlockState(flags=3)，useItemOnBlock 返回后即可读
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit 坐标映射（见 TestTransform + MinecraftStructurePlacer::place）：
//   helper 相对坐标 (rx,ry,rz) → 结构内坐标 (rx, ry-1, rz)
//   （placeOrigin = origin + (0,1,0)，结构内 (0,0,0) 放在 placeOrigin）。
// glass_pit 结构布局（结构内坐标）：
//   Y=0: 全 glass（底面）
//   Y=1: 玻璃墙 + 5×5 air 空腔（X∈[1,5], Z∈[1,5]）
//   Y=2: 玻璃墙 + 5×5 air 空腔
//   Y=3: 玻璃墙 + 5×5 air 空腔
//   Y=4: 全 air（顶面）
// 故 helper Y=2 → 结构内 Y=1（air 空腔）；helper Y=3 → 结构内 Y=2（air 空腔）。
//
// 测试1 rooted_dirt_bonemeal_grows_hanging_roots（正向验证：下方空气→生成垂根）：
//   布局：(2,3,2) 放 rooted_dirt（骨粉目标），下方 (2,2,2) 为 air。
//   对 (2,3,2) useItemOnBlock 骨粉 → canGrow(下方 air) → true → grow → 下方 (2,2,2) 放置垂根。
//   断言：(2,2,2) 变为 minecraft:hanging_roots。
//
// 测试2 rooted_dirt_bonemeal_fails_when_below_blocked（反向验证：下方非空气→骨粉无效）：
//   布局：(4,2,2) 放 glass（阻塞下方），(4,3,2) 放 rooted_dirt（骨粉目标）。
//   对 (4,3,2) useItemOnBlock 骨粉 → canGrow(下方 glass 非空气) → false → 骨粉无效。
//   断言：(4,2,2) 仍为 minecraft:glass（未生成垂根）。
//
// 两个测试使用不同 X 坐标（测试1用 X=2，测试2用 X=4），避免 GameTest 框架不清场导致的状态泄漏。
//
// ============================ 跨服务端对比 ============================
// - rooted_dirt/hanging_roots typeId 两端一致（1.17 加入，1.21.11 已含）。
// - 缠根泥土骨粉生成垂根行为两端一致（wiki 明文：下方生成垂根）。
// - useItemOnBlock + getBlock typeId 判定为两端通用 API，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_缠根泥土.txt（骨粉在下方生成垂根）
// Ref: net/minecraft/world/level/block/RootedDirtBlock.java（isValidBonemealTarget, performBonemeal）
// Ref: src/common/world/block/blocks/cave/RootedDirtBlock.cpp（canGrow, grow）
// Ref: src/common/item/items/special/BoneMealItem.cpp:61-123（onItemUse → canGrow/canUseBonemeal/grow 链路）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 下方为空气时骨粉生成垂根。
// 布局：(2,3,2) 放 rooted_dirt（骨粉目标），下方 (2,2,2) 为 air。
// 对 (2,3,2) useItemOnBlock 骨粉 → canGrow(下方 air) → true → grow → 下方 (2,2,2) 放置垂根。
// 断言：(2,2,2) 变为 minecraft:hanging_roots。
function rootedDirtBonemealGrowsHangingRoots(test: Test): void {
    const ROOTED_DIRT = { x: 2, y: 3, z: 2 }; // 缠根泥土位置（骨粉目标）
    const BELOW = { x: 2, y: 2, z: 2 }; // 下方位置（垂根生成处）

    // (2,3,2) 放 rooted_dirt（骨粉目标）。glass_pit 的 (2,3,2) 默认为 air，满足放置条件。
    test.setBlockType("minecraft:rooted_dirt", ROOTED_DIRT);
    test.assert(
        getTypeId(test, ROOTED_DIRT) === "minecraft:rooted_dirt",
        `rooted_dirt should be at ${JSON.stringify(ROOTED_DIRT)}, got ${getTypeId(test, ROOTED_DIRT)}`,
    );
    // 确认下方为 air（canGrow 前置条件）。
    test.assert(
        getTypeId(test, BELOW) === "minecraft:air",
        `below ${JSON.stringify(BELOW)} should be air, got ${getTypeId(test, BELOW)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对缠根泥土 useItemOnBlock 骨粉 → BoneMealItem::onItemUse → canGrow → grow → 下方放置垂根。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        ROOTED_DIRT,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when bonemealing rooted_dirt, got used=${used}`);

    // 断言：下方生成垂根。
    test.assert(
        getTypeId(test, BELOW) === "minecraft:hanging_roots",
        `hanging_roots should grow at ${JSON.stringify(BELOW)} after bonemeal, got ${getTypeId(test, BELOW)} ` +
            `(rooted_dirt=${getTypeId(test, ROOTED_DIRT)}; ` +
            `if still air, grow may not place hanging_roots or canGrow may falsely fail)`,
    );

    test.succeed();
}

// 下方非空气时骨粉无效（canGrow 返回 false）。
// 布局：(4,2,2) 放 glass（阻塞下方），(4,3,2) 放 rooted_dirt（骨粉目标）。
// 对 (4,3,2) useItemOnBlock 骨粉 → canGrow(下方 glass 非空气) → false → 骨粉无效。
// 断言：(4,2,2) 仍为 minecraft:glass（未生成垂根）。
function rootedDirtBonemealFailsWhenBelowBlocked(test: Test): void {
    const ROOTED_DIRT = { x: 4, y: 3, z: 2 }; // 缠根泥土位置（骨粉目标）
    const BELOW = { x: 4, y: 2, z: 2 }; // 下方位置（被 glass 阻塞）

    // (4,2,2) 放 glass 阻塞下方（使 canGrow 下方空气检查失败）。
    test.setBlockType("minecraft:glass", BELOW);
    test.assert(
        getTypeId(test, BELOW) === "minecraft:glass",
        `glass should be at ${JSON.stringify(BELOW)}, got ${getTypeId(test, BELOW)}`,
    );

    // (4,3,2) 放 rooted_dirt（骨粉目标）。
    test.setBlockType("minecraft:rooted_dirt", ROOTED_DIRT);
    test.assert(
        getTypeId(test, ROOTED_DIRT) === "minecraft:rooted_dirt",
        `rooted_dirt should be at ${JSON.stringify(ROOTED_DIRT)}, got ${getTypeId(test, ROOTED_DIRT)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对缠根泥土 useItemOnBlock 骨粉 → canGrow(下方 glass 非空气) → false → 骨粉无效。
    // used 应为 false（BoneMealItem::onItemUse 返回 Fail）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        ROOTED_DIRT,
        Direction.Up,
    );

    // 断言：下方仍为 glass（未生成垂根）。
    // 注意：即使 used 为 true（骨粉被消耗但 grow 未生效），核心断言是下方未生成垂根。
    test.assert(
        getTypeId(test, BELOW) === "minecraft:glass",
        `glass should remain at ${JSON.stringify(BELOW)} when below is blocked, got ${getTypeId(test, BELOW)} ` +
            `(used=${used}; if hanging_roots, canGrow may falsely return true when below is not air)`,
    );

    test.succeed();
}

export function registerRootedDirtBonemealTests(): void {
    GameTest.register("BlockBehaviorTests", "rooted_dirt_bonemeal_grows_hanging_roots", rootedDirtBonemealGrowsHangingRoots)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "rooted_dirt_bonemeal_fails_when_below_blocked", rootedDirtBonemealFailsWhenBelowBlocked)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
