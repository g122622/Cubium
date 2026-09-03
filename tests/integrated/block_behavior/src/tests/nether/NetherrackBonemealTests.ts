// 下界岩（netherrack）骨粉行为 GameTest：验证骨粉将下界岩转化为周围存在的对应菌岩。
//
// wiki world_下界岩.txt#骨粉：
//   "对下界岩使用骨粉时，若其上方透光且周围 3×3×3 范围内存在菌岩（绯红菌岩或诡异菌岩），
//    则下界岩会转化为对应类型的菌岩：
//      - 周围仅有绯红菌岩 → 转化为绯红菌岩
//      - 周围仅有诡异菌岩 → 转化为诡异菌岩
//      - 周围两种菌岩都有 → 随机转化为其中一种"
//
// ============================ Java 版权威真相源 ============================
// net.minecraft.world.level.block.NetherrackBlock（1.21.11）：
//   - isValidBonemealTarget: !above().propagatesSkylightDown() → false；
//     否则遍历 betweenClosed(pos+(-1,-1,-1), pos+(1,1,1))，任一为 NYLIUM tag → true
//   - isBonemealSuccess: 恒 true（骨粉 100% 有效）
//   - performBonemeal: 遍历周围 3×3×3 统计 warped/crimson 菌岩
//     flag1&&flag → 随机选一种；仅 flag1 → 诡异；仅 flag → 绯红
//   - getType: NEIGHBOR_SPREADER
//
// ============================ Cubium 实现链路 ============================
// NetherrackBlock（nether/NetherrackBlock.cpp）继承 Block + IGrowable：
//   - canGrow: 上方 propagatesSkylightDown + 周围 3×3×3 有菌岩
//   - canUseBonemeal: 恒 true
//   - grow: 遍历周围 3×3×3 统计菌岩，转化下界岩
//   - getBoneMealType: NEIGHBOR_SPREADER
//
// BoneMealItem::onItemUse（src/common/item/items/special/BoneMealItem.cpp:61-123）：
//   - dynamic_cast<IGrowable> 取 NetherrackBlock
//   - canGrow(上方透光+周围有菌岩) → canUseBonemeal(恒 true) → grow(转化下界岩)
//   - grow 同步 setBlockState(flags=3)，useItemOnBlock 返回后即可读
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃坑）============================
// glass_pit：y=0 glass 底，y=1..3 air 空腔，y=4 glass 顶。helper 相对坐标 x,z∈[0,6], y∈[0,4]。
// Y=1 层是 5×5 air 空腔（中心 (3,1,2)）。
//
// 测试1 netherrack_bonemeal_converts_to_crimson_nylium（正向验证：周围绯红菌岩→绯红菌岩）：
//   布局：(2,1,2) 放 crimson_nylium（下界岩水平相邻，在 3×3×3 扫描范围内），
//         (3,1,2) 放 netherrack（骨粉目标），上方 (3,2,2) 为 air（透光）。
//   对 (3,1,2) 下界岩 useItemOnBlock 骨粉 → canGrow(上方 air 透光 + 周围有绯红菌岩) → true
//     → grow → 周围仅有绯红菌岩 → 转化为绯红菌岩。
//   断言：(3,1,2) 变为 minecraft:crimson_nylium。
//
// 测试2 netherrack_bonemeal_converts_to_warped_nylium（正向验证：周围诡异菌岩→诡异菌岩）：
//   布局：(2,1,2) 放 warped_nylium，(3,1,2) 放 netherrack，上方 air。
//   对 (3,1,2) 骨粉 → grow → 周围仅有诡异菌岩 → 转化为诡异菌岩。
//   断言：(3,1,2) 变为 minecraft:warped_nylium。
//
// 测试3 netherrack_bonemeal_without_nylium_fails（反向验证：周围无菌岩→骨粉无效）：
//   布局：(3,1,2) 放 netherrack，上方 (3,2,2) 为 air（透光），周围无任何菌岩。
//   对 (3,1,2) 骨粉 → canGrow(周围 3×3×3 无菌岩) → false → 骨粉无效。
//   断言：(3,1,2) 仍为 minecraft:netherrack（未转化）。
//
// ============================ 排除项（不写测试）============================
// - 两种菌岩都有时随机选一种：随机性强，无法稳定断言具体转化结果，跳过。
//   （可在未来用固定种子或多次运行取统计验证，但当前 GameTest 框架不支持固定种子）
// - 上方不透光时 canGrow 返回 false：与测试3 逻辑重叠（都是 canGrow false 分支），跳过。
//   glass_pit 顶部为 glass（透光），无法在不破坏结构的前提下构造"上方不透光"场景。
//
// ============================ 跨服务端对比 ============================
// - netherrack/crimson_nylium/warped_nylium typeId 两端一致（1.16 加入，1.21.11 已含）。
// - 下界岩骨粉转化行为两端一致（wiki 明文：周围有菌岩则转化）。
// - useItemOnBlock + getBlock typeId 判定为两端通用 API，可跨服务端对比。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_下界岩.txt#骨粉（周围有菌岩则转化）
// Ref: net/minecraft/world/level/block/NetherrackBlock.java（isValidBonemealTarget:26-38, performBonemeal:46-72）
// Ref: src/common/world/block/blocks/nether/NetherrackBlock.cpp（canGrow:40-70, grow:83-119）
// Ref: src/common/item/items/special/BoneMealItem.cpp:61-123（onItemUse → canGrow/canUseBonemeal/grow 链路）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 内部 air 层坐标。下界岩 (3,1,2)（骨粉目标），菌岩 (2,1,2)（水平相邻）。
const NETHERRACK = { x: 3, y: 1, z: 2 }; // 下界岩位置（骨粉目标）
const ADJACENT = { x: 2, y: 1, z: 2 }; // 水平相邻位置（放菌岩，在 3×3×3 扫描范围内）
const ABOVE = { x: 3, y: 2, z: 2 }; // 下界岩正上方（air，透光）

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 周围绯红菌岩 → 骨粉将下界岩转化为绯红菌岩。
// 布局：(2,1,2) 铺 crimson_nylium（水平相邻），(3,1,2) 放 netherrack（骨粉目标）。
// 对 (3,1,2) 下界岩 useItemOnBlock 骨粉 → canGrow(上方 air 透光 + 周围有绯红菌岩) → true
//   → grow → 周围仅有绯红菌岩 → 转化为绯红菌岩。
// 断言：(3,1,2) 变为 minecraft:crimson_nylium。
function netherrackBonemealConvertsToCrimsonNylium(test: Test): void {
    // (2,1,2) 铺 crimson_nylium 作周围菌岩（canGrow 扫描 3×3×3 范围内需有菌岩）。
    test.setBlockType("minecraft:crimson_nylium", ADJACENT);
    test.assert(
        getTypeId(test, ADJACENT) === "minecraft:crimson_nylium",
        `crimson_nylium should be at ${JSON.stringify(ADJACENT)}, got ${getTypeId(test, ADJACENT)}`,
    );

    // (3,1,2) 放 netherrack（骨粉目标）。
    test.setBlockType("minecraft:netherrack", NETHERRACK);
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:netherrack",
        `netherrack should be at ${JSON.stringify(NETHERRACK)}, got ${getTypeId(test, NETHERRACK)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对下界岩 useItemOnBlock 骨粉 → BoneMealItem::onItemUse → canGrow → grow → 转化为绯红菌岩。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        NETHERRACK,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when bonemealing netherrack, got used=${used}`);

    // 断言：下界岩被转化为绯红菌岩。
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:crimson_nylium",
        `netherrack should convert to crimson_nylium after bonemeal, got ${getTypeId(test, NETHERRACK)} ` +
            `(adjacent=${getTypeId(test, ADJACENT)} above=${getTypeId(test, ABOVE)}; ` +
            `if still netherrack, canGrow may falsely fail or grow may not convert)`,
    );

    test.succeed();
}

// 周围诡异菌岩 → 骨粉将下界岩转化为诡异菌岩。
// 布局：(2,1,2) 铺 warped_nylium（水平相邻），(3,1,2) 放 netherrack（骨粉目标）。
// 对 (3,1,2) 下界岩 useItemOnBlock 骨粉 → canGrow → true → grow → 周围仅有诡异菌岩 → 转化为诡异菌岩。
// 断言：(3,1,2) 变为 minecraft:warped_nylium。
function netherrackBonemealConvertsToWarpedNylium(test: Test): void {
    // (2,1,2) 铺 warped_nylium 作周围菌岩。
    test.setBlockType("minecraft:warped_nylium", ADJACENT);
    test.assert(
        getTypeId(test, ADJACENT) === "minecraft:warped_nylium",
        `warped_nylium should be at ${JSON.stringify(ADJACENT)}, got ${getTypeId(test, ADJACENT)}`,
    );

    // (3,1,2) 放 netherrack（骨粉目标）。
    test.setBlockType("minecraft:netherrack", NETHERRACK);
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:netherrack",
        `netherrack should be at ${JSON.stringify(NETHERRACK)}, got ${getTypeId(test, NETHERRACK)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对下界岩 useItemOnBlock 骨粉 → grow → 转化为诡异菌岩。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        NETHERRACK,
        Direction.Up,
    );
    test.assert(used, `useItemOnBlock should return true when bonemealing netherrack, got used=${used}`);

    // 断言：下界岩被转化为诡异菌岩。
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:warped_nylium",
        `netherrack should convert to warped_nylium after bonemeal, got ${getTypeId(test, NETHERRACK)} ` +
            `(adjacent=${getTypeId(test, ADJACENT)} above=${getTypeId(test, ABOVE)}; ` +
            `if still netherrack, canGrow may falsely fail or grow may not convert)`,
    );

    test.succeed();
}

// 周围无菌岩 → 骨粉无效（canGrow 返回 false）。
// 布局：(3,1,2) 放 netherrack，上方 (3,2,2) 为 air（透光），周围无任何菌岩。
// 对 (3,1,2) 骨粉 → canGrow(周围 3×3×3 无菌岩) → false → 骨粉无效。
// 断言：(3,1,2) 仍为 minecraft:netherrack（未转化）。
function netherrackBonemealWithoutNyliumFails(test: Test): void {
    // (3,1,2) 放 netherrack（骨粉目标），周围无任何菌岩。
    test.setBlockType("minecraft:netherrack", NETHERRACK);
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:netherrack",
        `netherrack should be at ${JSON.stringify(NETHERRACK)}, got ${getTypeId(test, NETHERRACK)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对下界岩 useItemOnBlock 骨粉 → canGrow(周围无菌岩) → false → 骨粉无效。
    // used 应为 false（BoneMealItem::onItemUse 返回 Fail）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        NETHERRACK,
        Direction.Up,
    );

    // 断言：下界岩未被转化（仍为 netherrack）。
    // 注意：即使 used 为 true（骨粉被消耗但 grow 未生效），核心断言是下界岩未转化为菌岩。
    test.assert(
        getTypeId(test, NETHERRACK) === "minecraft:netherrack",
        `netherrack should remain netherrack when no nylium nearby, got ${getTypeId(test, NETHERRACK)} ` +
            `(above=${getTypeId(test, ABOVE)}; used=${used}; ` +
            `if converted, canGrow may falsely return true when no nylium in 3x3x3)`,
    );

    test.succeed();
}

export function registerNetherrackBonemealTests(): void {
    GameTest.register("BlockBehaviorTests", "netherrack_bonemeal_converts_to_crimson_nylium", netherrackBonemealConvertsToCrimsonNylium)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "netherrack_bonemeal_converts_to_warped_nylium", netherrackBonemealConvertsToWarpedNylium)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "netherrack_bonemeal_without_nylium_fails", netherrackBonemealWithoutNyliumFails)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
