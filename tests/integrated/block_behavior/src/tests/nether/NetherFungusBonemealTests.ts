// 下界菌骨粉生成巨型真菌 GameTest。
//
// wiki world_绯红菌.txt#生长（:46）：
//   "对种植在对应绯红菌岩上的绯红菌使用骨粉有40%的概率可使之生长为巨型绯红菌。
//    若不为绯红菌岩则无法生长。"
// wiki world_诡异菌.txt#生长（:46）：同样的诡异菌岩支撑 + 40% 概率长成巨型诡异菌。
//   ——骨粉条件：真菌下方为对应菌岩（绯红菌→绯红菌岩，诡异菌→诡异菌岩）。
//
// ============================ Java 版权威真相源（1.21.11）============================
// net.minecraft.world.level.block.FungusBlock（implements BonemealableBlock）：
//   - isValidBonemealTarget(LevelReader, BlockPos, BlockState):
//       return levelReader.getBlockState(pos.below()).is(this.requiredBlock);
//     —— 下方必须为对应菌岩（绯红菌→绯红菌岩，诡异菌→诡异菌岩）。
//   - isBonemealSuccess(Level, RandomSource, BlockPos, BlockState): return random.nextFloat() < 0.4;
//     —— 40% 概率门限。
//   - performBonemeal(ServerLevel, RandomSource, BlockPos, BlockState):
//       this.getFeature(level).ifPresent(holder -> holder.value().place(...));
//     —— 调用 ConfiguredFeature<HugeFungusConfiguration> 生成巨型真菌。
//
// net.minecraft.world.item.BoneMealItem.useOn → growCrop:
//   - isValidBonemealTarget 通过 → 始终消耗骨粉 + 返回 SUCCESS。
//   - isBonemealSuccess 门限通过 → performBonemeal 执行生长。
//   - 门限失败时仍消耗骨粉 + 返回 SUCCESS，仅不触发生长。
//
// net.minecraft.world.level.levelgen.feature.HugeFungusFeature.place:
//   - 高度 i = Mth.nextInt(randomsource, 4, 13) = [4,13]。
//   - 1/12 概率双倍高度：if (randomsource.nextInt(12) == 0) i *= 2;
//   - 粗壮菌柄 flag = !planted && randomsource.nextFloat() < 0.06F。
//   - planted 时跳过高度上限检查（getGenDepth）。
//
// ============================ Cubium 实现链路 ============================
// FungusBlock（nether/FungusBlock.cpp）继承 SimpleBlock + IGrowable：
//   - canGrow: 下方为对应菌岩（绯红菌→绯红菌岩，诡异菌→诡异菌岩）。
//   - canUseBonemeal: random.nextFloat() < 0.4f（对齐 Java isBonemealSuccess 40% 门限）。
//   - grow: 通过 IWorld::createFeatureRegion() 构建 WorldGenRegion，
//     清除下界菌方块后调用 HugeFungusFeature::place()。
//
// BoneMealItem::onItemUse（src/common/item/items/special/BoneMealItem.cpp）：
//   - dynamic_cast<IGrowable> 取 FungusBlock。
//   - canGrow(下方对应菌岩) → canUseBonemeal(40% 门限) → grow(生成巨型真菌)。
//   - 软门限：canUseBonemeal 通过才 grow，但骨粉始终消耗 + 返 Success。
//
// HugeFungusFeature::place（src/common/world/gen/feature/nether/HugeFungusFeature.cpp）：
//   - 高度 4 + random.nextInt(10) = [4,13]（对齐 Java Mth.nextInt(4,13)）。
//   - 1/12 概率双倍高度。
//   - thickStem = !config.planted && random.nextFloat() < 0.06f（对齐 Java flag）。
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
// 故 helper Y=0 → 结构内 Y=-1（不存在，实际为 glass 底下方）；
//     helper Y=1 → 结构内 Y=0（glass 底）；
//     helper Y=2 → 结构内 Y=1（air 空腔首层）。
//
// 布局列（不同 X 避免状态泄漏，共用 SUPPORT=(x,0,1)、FUNGUS=(x,1,1) 范式）：
//   测试1 crimson_fungus_bonemeal_should_grow：X=3，绯红菌岩+绯红菌。
//   测试2 warped_fungus_bonemeal_should_grow：X=4，诡异菌岩+诡异菌。
//   测试3 crimson_fungus_bonemeal_fails_on_warped_nylium：X=5，诡异菌岩+绯红菌（canGrow false）。
//   测试4 crimson_fungus_bonemeal_fails_without_nylium：X=2，无下方菌岩（canGrow false）。
//
// ============================ 跨服务端对比 ============================
// - crimson_fungus/warped_fungus typeId 两端一致（1.16 加入，1.21.11 已含）。
// - 骨粉生成巨型真菌两端一致（wiki :46 明文，40% 概率）。
//   注意：基岩侧巨型真菌生成概率/形状可能与 Java 不同，本测试以 Java 版为权威。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_绯红菌.txt#生长（:46 对应菌岩+40%概率长成巨型绯红菌）
// Ref: net/minecraft/world/level/block/FungusBlock.java（isValidBonemealTarget, isBonemealSuccess, performBonemeal）
// Ref: net/minecraft/world/item/BoneMealItem.java（growCrop: 软门限消耗骨粉）
// Ref: net/minecraft/world/level/levelgen/feature/HugeFungusFeature.java（place: 高度[4,13], thickStem=planted门限）
// Ref: src/common/world/block/blocks/nether/FungusBlock.cpp（canGrow, canUseBonemeal, grow）
// Ref: src/common/item/items/special/BoneMealItem.cpp（onItemUse: canGrow/canUseBonemeal/grow 链路）
// Ref: src/common/world/gen/feature/nether/HugeFungusFeature.cpp（place: 高度[4,13], thickStem 门限）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 绯红菌骨粉应生效（wiki :46 骨粉有概率生成巨型绯红菌）。
// 布局：(3,0,1) 铺 crimson_nylium 支撑，(3,1,1) 放 crimson_fungus（在菌岩上）。
// 对 (3,1,1) 绯红菌 useItemOnBlock 骨粉 → 断言 used===true（骨粉生效）。
// 缺陷暴露（已修复）：FungusBlock 实现 IGrowable 后骨粉生效（used===true）。
function crimsonFungusBonemealShouldGrow(test: Test): void {
    const SUPPORT = { x: 3, y: 0, z: 1 }; // 下方支撑（crimson_nylium）
    const FUNGUS = { x: 3, y: 1, z: 1 }; // 真菌位置（crimson_fungus）

    // (3,0,1) 铺 crimson_nylium 作真菌下方支撑（canGrow 检查下方为绯红菌岩）。
    test.setBlockType("minecraft:crimson_nylium", SUPPORT);

    // (3,1,1) 放 crimson_fungus（在绯红菌岩上）。
    test.setBlockType("minecraft:crimson_fungus", FUNGUS);
    test.assert(
        getTypeId(test, FUNGUS) === "minecraft:crimson_fungus",
        `crimson_fungus should be at ${JSON.stringify(FUNGUS)}, got ${getTypeId(test, FUNGUS)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对绯红菌 useItemOnBlock 骨粉 → wiki :46 骨粉有概率生成巨型绯红菌，骨粉应生效（返 true）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FUNGUS,
        Direction.Up,
    );

    // 缺陷暴露（已修复）：FungusBlock 实现 IGrowable 后骨粉生效（used===true）。
    test.assert(
        used,
        `bonemeal on crimson_fungus should succeed (used=${used}). ` +
            `wiki:46 bonemeal may grow huge crimson fungus. ` +
            `Cubium defect (fixed): FungusBlock now implements IGrowable.`,
    );

    test.succeed();
}

// 诡异菌骨粉应生效（wiki :46 骨粉有概率生成巨型诡异菌）。
// 布局：(4,0,1) 铺 warped_nylium 支撑，(4,1,1) 放 warped_fungus（在菌岩上）。
// 对 (4,1,1) 诡异菌 useItemOnBlock 骨粉 → 断言 used===true（骨粉生效）。
// 缺陷暴露（已修复）：FungusBlock::canGrow 支持诡异菌岩，骨粉生效（used===true）。
function warpedFungusBonemealShouldGrow(test: Test): void {
    const SUPPORT = { x: 4, y: 0, z: 1 }; // 下方支撑（warped_nylium）
    const FUNGUS = { x: 4, y: 1, z: 1 }; // 真菌位置（warped_fungus）

    // (4,0,1) 铺 warped_nylium 作真菌下方支撑（canGrow 检查下方为诡异菌岩）。
    test.setBlockType("minecraft:warped_nylium", SUPPORT);

    // (4,1,1) 放 warped_fungus（在诡异菌岩上）。
    test.setBlockType("minecraft:warped_fungus", FUNGUS);
    test.assert(
        getTypeId(test, FUNGUS) === "minecraft:warped_fungus",
        `warped_fungus should be at ${JSON.stringify(FUNGUS)}, got ${getTypeId(test, FUNGUS)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对诡异菌 useItemOnBlock 骨粉 → wiki :46 骨粉有概率生成巨型诡异菌，骨粉应生效（返 true）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FUNGUS,
        Direction.Up,
    );

    // 缺陷暴露（已修复）：FungusBlock::canGrow 支持诡异菌岩，骨粉生效（used===true）。
    test.assert(
        used,
        `bonemeal on warped_fungus should succeed (used=${used}). ` +
            `wiki:46 bonemeal may grow huge warped fungus. ` +
            `Cubium defect (fixed): FungusBlock now implements IGrowable.`,
    );

    test.succeed();
}

// 绯红菌在诡异菌岩上 → canGrow 返回 false → 骨粉无效（used===false）。
// 布局：(5,0,1) 铺 warped_nylium（错误菌岩），(5,1,1) 放 crimson_fungus。
// 对 (5,1,1) 绯红菌 useItemOnBlock 骨粉 → canGrow(下方诡异菌岩，非绯红菌岩) → false → 骨粉无效。
// 缺陷暴露（已修复）：FungusBlock::canGrow 严格检查下方菌岩类型与真菌匹配。
function crimsonFungusBonemealFailsOnWarpedNylium(test: Test): void {
    const SUPPORT = { x: 5, y: 0, z: 1 }; // 下方支撑（warped_nylium，错误菌岩）
    const FUNGUS = { x: 5, y: 1, z: 1 }; // 真菌位置（crimson_fungus）

    // (5,0,1) 铺 warped_nylium 作真菌下方支撑（绯红菌需绯红菌岩，此处为错误菌岩）。
    test.setBlockType("minecraft:warped_nylium", SUPPORT);

    // (5,1,1) 放 crimson_fungus（在诡异菌岩上，类型不匹配）。
    test.setBlockType("minecraft:crimson_fungus", FUNGUS);
    test.assert(
        getTypeId(test, FUNGUS) === "minecraft:crimson_fungus",
        `crimson_fungus should be at ${JSON.stringify(FUNGUS)}, got ${getTypeId(test, FUNGUS)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对绯红菌 useItemOnBlock 骨粉 → canGrow(下方诡异菌岩非绯红菌岩) → false → 骨粉无效。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FUNGUS,
        Direction.Up,
    );

    // 断言：骨粉未生效（used===false）。
    // 注意：Java growCrop 中 isValidBonemealTarget(false) → 不消耗骨粉 + 返 PASS（即 used===false）。
    test.assert(
        !used,
        `bonemeal on crimson_fungus over warped_nylium should fail (used=${used}). ` +
            `wiki:46 crimson_fungus requires crimson_nylium below. ` +
            `Cubium defect (fixed): canGrow strictly checks nylium type matches fungus.`,
    );

    test.succeed();
}

// 下方无对应菌岩 → canGrow 返回 false → 骨粉无效（used===false）。
// 布局：(2,0,1) 铺 netherrack（非菌岩），(2,1,1) 放 crimson_fungus。
// 对 (2,1,1) 绯红菌 useItemOnBlock 骨粉 → canGrow(下方 netherrack，非菌岩) → false → 骨粉无效。
// 缺陷暴露（已修复）：FungusBlock::canGrow 严格检查下方为对应菌岩。
function crimsonFungusBonemealFailsWithoutNylium(test: Test): void {
    const SUPPORT = { x: 2, y: 0, z: 1 }; // 下方支撑（netherrack，非菌岩）
    const FUNGUS = { x: 2, y: 1, z: 1 }; // 真菌位置（crimson_fungus）

    // (2,0,1) 铺 netherrack 作真菌下方支撑（非菌岩，canGrow 应返回 false）。
    test.setBlockType("minecraft:netherrack", SUPPORT);

    // (2,1,1) 放 crimson_fungus（在下界岩上，非对应菌岩）。
    test.setBlockType("minecraft:crimson_fungus", FUNGUS);
    test.assert(
        getTypeId(test, FUNGUS) === "minecraft:crimson_fungus",
        `crimson_fungus should be at ${JSON.stringify(FUNGUS)}, got ${getTypeId(test, FUNGUS)}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对绯红菌 useItemOnBlock 骨粉 → canGrow(下方 netherrack 非菌岩) → false → 骨粉无效。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        FUNGUS,
        Direction.Up,
    );

    // 断言：骨粉未生效（used===false）。
    test.assert(
        !used,
        `bonemeal on crimson_fungus over netherrack should fail (used=${used}). ` +
            `wiki:46 crimson_fungus requires crimson_nylium below. ` +
            `Cubium defect (fixed): canGrow strictly checks below is matching nylium.`,
    );

    test.succeed();
}

export function registerNetherFungusBonemealTests(): void {
    GameTest.register("BlockBehaviorTests", "crimson_fungus_bonemeal_should_grow", crimsonFungusBonemealShouldGrow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register("BlockBehaviorTests", "warped_fungus_bonemeal_should_grow", warpedFungusBonemealShouldGrow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register(
        "BlockBehaviorTests",
        "crimson_fungus_bonemeal_fails_on_warped_nylium",
        crimsonFungusBonemealFailsOnWarpedNylium,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(80);
    GameTest.register(
        "BlockBehaviorTests",
        "crimson_fungus_bonemeal_fails_without_nylium",
        crimsonFungusBonemealFailsWithoutNylium,
    )
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
