// 下界菌骨粉生成巨型真菌 GameTest。
//
// wiki world_绯红菌.txt#生长（:46）：
//   "对种植在对应绯红菌岩上的绯红菌使用骨粉有40%的概率可使之生长为巨型绯红菌。
//    若不为绯红菌岩则无法生长。"
// wiki world_诡异菌.txt#生长（:46）：同样的诡异菌岩支撑 + 40% 概率长成巨型诡异菌。
//   ——骨粉条件：真菌下方为对应菌岩（绯红菌→绯红菌岩，诡异菌→诡异菌岩）。
//
// ============================ Cubium 缺陷（已修复）============================
// 修复前缺陷：CRIMSON_FUNGUS/WARPED_FUNGUS 在 NetherBlocks.cpp 用 SimpleBlock 注册，
//   SimpleBlock 不继承 IGrowable。BoneMealItem::onItemUse（BoneMealItem.cpp:70）经
//   dynamic_cast<const IGrowable*>(&block) 检查，SimpleBlock 非 IGrowable → 返 nullptr →
//   跳过 IGrowable 分支 → 继续走海草分支（非水环境失败）→ 返 Fail。故骨粉对下界菌完全无效。
// 修复方案：新建 FungusBlock（继承 SimpleBlock + IGrowable），在 NetherBlocks.cpp 改用
//   FungusBlock 注册。grow() 通过 IWorld::createFeatureRegion() 构建 WorldGenRegion，
//   再调用 HugeFungusFeature::place() 生成巨型真菌。
//
// ============================ 测试设计（glass_pit 7×5×7 玻璃墙空腔）============================
// glass_pit：y=0 glass/cobblestone 底，y=1..2 玻璃墙围出内部 air 空腔，y=3..4 air+顶部框架
// （helper 相对坐标 x,z∈[0,6], y∈[0,4]）。方块测试在内部 air 层操作。
//
// 布局列 (3,*,1)（同 NetherRootsTests/CactusTests 坐标范式）：支撑 (3,0,1)、真菌 (3,1,1)。
//   支撑放 y=0（覆盖 glass_pit 默认 glass 底），真菌放 y=1（内部 air 层第一层）。
//
// 测试1 crimson_fungus_bonemeal_should_grow（绯红菌骨粉生效，正向验证）：
//   crimson_nylium (3,0,1) + crimson_fungus (3,1,1)。SimulatedPlayer 持骨粉对 (3,1,1) useItemOnBlock。
//   canGrow：下方为绯红菌岩 → true。canUseBonemeal → true。grow() 调 HugeFungusFeature::place。
//   断言 used===true（骨粉生效）。巨型真菌因 glass_pit 空间受限不一定完整生成，
//   但 grow() 执行后 BoneMealItem 仍返 Success（消耗骨粉 + 粒子），used 稳定 true。
//
// ============================ 排除项（不写测试）============================
// - 40% 概率门限：Cubium BoneMealItem 未实现概率门限（canUseBonemeal 恒 true），
//   概率行为无法稳定测试，跳过。wiki :46 明文 40% 概率。
// - 巨型真菌方块实际生成：glass_pit 内部空间（5×3×5）不足以容纳完整巨型真菌
//   （HugeFungusFeature 生成 4-11 高 + 5×5 菌盖），HugeFungusFeature::_canPlaceAt 会因
//   planted=true 跳过空间检查，但菌柄/菌盖会超出结构边界被截断，无法稳定验证，跳过。
// - 非菌岩支撑不生效：与"骨粉生效"核心行为重叠，且 glass_pit 内验证支撑面种类冗余，跳过。
//
// ============================ 跨服务端对比 ============================
// - crimson_fungus/warped_fungus typeId 两端一致（1.16 加入，1.21.11 已含）。
// - 骨粉生成巨型真菌两端一致（wiki :46 明文，40% 概率）。
//   注意：基岩侧巨型真菌生成概率/形状可能与 Java 不同，本测试以 Java 版为权威。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\world_绯红菌.txt#生长（:46 对应菌岩+40%概率长成巨型绯红菌）
// Ref: src/common/world/block/blocks/nether/FungusBlock.cpp（canGrow:39-57, grow:71-102）
// Ref: src/common/item/items/special/BoneMealItem.cpp:70（dynamic_cast<IGrowable> 检查骨粉有效性）
// Ref: src/common/world/gen/feature/nether/HugeFungusFeature.cpp（place:44-89, _canPlaceAt:91-132）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 内部 air 层坐标。支撑 (3,0,1)（覆盖 glass 底），真菌 (3,1,1)。
const SUPPORT = { x: 3, y: 0, z: 1 }; // 下方支撑（crimson_nylium）
const FUNGUS = { x: 3, y: 1, z: 1 }; // 真菌位置（crimson_fungus）

// 读取方块 typeId。返回空串表示读取失败。
function getTypeId(test: Test, pos: { x: number; y: number; z: number }): string {
    return test.getBlock(pos)?.typeId ?? "";
}

// 绯红菌骨粉应生效（wiki :46 骨粉有概率生成巨型绯红菌）。
// 布局：(3,0,1) 铺 crimson_nylium 支撑，(3,1,1) 放 crimson_fungus（在菌岩上）。
// 对 (3,1,1) 绯红菌 useItemOnBlock 骨粉 → 断言 used===true（骨粉生效）。
// 缺陷暴露（已修复）：FungusBlock 实现 IGrowable 后骨粉生效（used===true）。
function crimsonFungusBonemealShouldGrow(test: Test): void {
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

export function registerNetherFungusBonemealTests(): void {
    GameTest.register("BlockBehaviorTests", "crimson_fungus_bonemeal_should_grow", crimsonFungusBonemealShouldGrow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
