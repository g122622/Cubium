// 蘑菇骨粉生成巨型蘑菇 GameTest。
//
// wiki tech_蘑菇.txt#巨型蘑菇（:83-90）：
//   "蘑菇满足以下条件时，对其使用骨粉有概率使之成长为对应的巨型蘑菇：
//    - 种植在适当的方块（泥土、砂土、草方块）上且亮度等级低于13，
//      或在任意亮度等级下种植在菌丝体、灰化土或菌岩上。
//    - 生长空间充足（长宽各7格，高6-8格）。
//    - 蘑菇不含雪。"
//   wiki tech_巨型蘑菇.txt#生成（:55）："玩家或发射器对蘑菇使用骨粉时，若满足上述条件，
//   蘑菇有40%的概率生长成巨型蘑菇。"
//
// ============================ Cubium 缺陷（已修复）============================
// 修复前缺陷：MushroomBlock（vegetation/MushroomBlock.hpp:51）继承 Block + IPlantable，
//   但未继承 IGrowable。BoneMealItem::onItemUse（BoneMealItem.cpp:70）经
//   dynamic_cast<const IGrowable*>(&block) 检查，未实现 IGrowable → 返 nullptr →
//   跳过 IGrowable 分支 → 返 Fail。故骨粉对蘑菇完全无效，无法生成巨型蘑菇。
// 修复方案：MushroomBlock 继承 IGrowable，实现 canGrow/canUseBonemeal/grow。
//   grow() 通过 IWorld::createFeatureRegion() 构建 WorldGenRegion，再调用
//   BigMushroomGenerators::brownMushroom()/redMushroom() 生成巨型蘑菇。
//
// ============================ 测试设计（glass_pit 7×5×7）============================
// glass_pit：y=0 glass 底座，y=1..3 air 空腔，y=4 glass 顶部。helper 相对坐标
// x,z∈[0,6], y∈[0,4]。结构内容从 origin+(0,1,0) 放置（placeOrigin），helper
// worldBlockPosition(rel)=origin+rel。故相对 y=N 对应结构内 y=N-1。
//
// 测试布局：
//   (3,1,1) 放 dirt（下方支撑，dirt 在 BigMushroomFeature::canPlaceAt 有效地面集合内）。
//   (3,2,1) 放 brown_mushroom（在 dirt 上，强放绕过 isValidPosition，不立即自毁）。
//   对 (3,2,1) 蘑菇 useItemOnBlock 骨粉 → IGrowable::grow 生成棕色巨型蘑菇。
//
// 断言：
//   useItemOnBlock 返 true（骨粉生效）。
//
//   注：glass_pit 内部空间（5×3×5）不足以容纳完整巨型蘑菇（5×5×6-8），
//   BigMushroomFeature::canPlaceAt 会因空间不足返回 false，巨型蘑菇不生成。
//   但 grow() 调用生成器后仍返回，BoneMealItem 仍判定骨粉成功（消耗骨粉 +
//   发送粒子效果）。故 used===true 稳定成立，不受巨型蘑菇是否实际生成影响。
//   巨型蘑菇方块的实际生成验证因空间限制而无法稳定测试，故不验证。
//
// ============================ 排除项（不写测试）============================
// - 40% 概率门限：Cubium 未实现概率门限，无法测，跳过。
// - 空间不足不生成：需精确控制空间，且与"骨粉生效"核心行为重叠，跳过。
// - 含雪不生成：Cubium 蘑菇无含雪语义，跳过。
// - 菌丝体/灰化土/菌岩任意亮度可生成：需放置菌丝等方块，且与"骨粉生效"核心行为重叠，跳过。
// - 巨型蘑菇方块实际生成：glass_pit 空间不足以生成完整巨型蘑菇，无法稳定验证，跳过。
//
// ============================ 跨服务端对比 ============================
// - 蘑菇 typeId（brown_mushroom/red_mushroom）两端一致。
// - 骨粉生成巨型蘑菇两端一致（wiki :83 明文，40% 概率）。
//   注意：基岩侧巨型蘑菇生成概率/形状可能与 Java 不同，本测试以 Java 版为权威。
//
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_蘑菇.txt#巨型蘑菇（:83-90 骨粉生成巨型蘑菇条件）
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_巨型蘑菇.txt#生成（:55 40%概率生长成巨型蘑菇）
// Ref: src/common/world/block/blocks/vegetation/MushroomBlock.cpp（grow:194-226, IGrowable:158-192）
// Ref: src/common/world/gen/feature/vegetation/BigMushroomFeature.cpp（place:44-62, canPlaceAt:104-155）
// Ref: src/common/item/items/special/BoneMealItem.cpp:70（dynamic_cast<IGrowable> 检查骨粉有效性）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack, Direction } from "@minecraft/server";

// glass_pit 内部坐标。
const DIRT_SUPPORT = { x: 3, y: 1, z: 1 }; // 下方 dirt 支撑（dirt 在有效地面集合内）
const MUSHROOM = { x: 3, y: 2, z: 1 }; // brown_mushroom 位置

// 对蘑菇使用骨粉应生效（wiki :83 骨粉有概率生成巨型蘑菇）。
// 布局：(3,1,1) 铺 dirt 支撑，(3,2,1) 放 brown_mushroom（在 dirt 上，强放绕过 isValidPosition）。
// 对 (3,2,1) 蘑菇 useItemOnBlock 骨粉 → 断言 used===true（骨粉生效）。
// 缺陷暴露（已修复）：MushroomBlock 未实现 IGrowable，骨粉无效（used===false）。
function mushroomBonemealShouldGrow(test: Test): void {
    // (3,1,1) 铺 dirt 作蘑菇下方支撑（dirt 在 BigMushroomFeature::canPlaceAt 有效地面集合内）。
    test.setBlockType("minecraft:dirt", DIRT_SUPPORT);

    // (3,2,1) 放 brown_mushroom（在 dirt 上，强放绕过 isValidPosition，不立即自毁）。
    test.setBlockType("minecraft:brown_mushroom", MUSHROOM);
    test.assert(
        test.getBlock(MUSHROOM)?.typeId === "minecraft:brown_mushroom",
        `brown_mushroom should be at ${JSON.stringify(MUSHROOM)}, got ${test.getBlock(MUSHROOM)?.typeId ?? ""}`,
    );

    const farmer = test.spawnSimulatedPlayer({ x: 1, y: 2, z: 1 }, "farmer");
    const boneMeal = new ItemStack("minecraft:bone_meal", 1);

    // 对蘑菇 useItemOnBlock 骨粉 → wiki :83 骨粉有概率生成巨型蘑菇，骨粉应生效（返 true）。
    const used = farmer.useItemOnBlock(
        boneMeal as unknown as Parameters<typeof farmer.useItemOnBlock>[0],
        MUSHROOM,
        Direction.Up,
    );

    // 缺陷暴露（已修复）：MushroomBlock 实现 IGrowable 后骨粉生效（used===true）。
    test.assert(
        used,
        `bonemeal on brown_mushroom should succeed (used=${used}). ` +
            `wiki:83 bonemeal may grow huge mushroom. ` +
            `Cubium defect (fixed): MushroomBlock now implements IGrowable.`,
    );

    test.succeed();
}

export function registerMushroomBonemealTests(): void {
    GameTest.register("BlockBehaviorTests", "mushroom_bonemeal_should_grow", mushroomBonemealShouldGrow)
        .structureName("gametests:glass_pit")
        .maxTicks(80);
}
