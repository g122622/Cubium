/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "world/block/registry/GardenBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/garden/BushPlantBlock.hpp"
#include "world/block/blocks/garden/CactusFlowerBlock.hpp"
#include "world/block/blocks/garden/DryVegetationBlock.hpp"
#include "world/block/blocks/garden/FireflyBushBlock.hpp"
#include "world/block/blocks/garden/FlowerBedBlock.hpp"
#include "world/block/blocks/garden/LeafLitterBlock.hpp"

namespace mc {
namespace block_registry {

// 野花
Block* GardenBlocks::WILDFLOWERS = nullptr;

// 枯叶
Block* GardenBlocks::LEAF_LITTER = nullptr;

// 干草系列
Block* GardenBlocks::SHORT_DRY_GRASS = nullptr;
Block* GardenBlocks::TALL_DRY_GRASS = nullptr;

// 仙人掌花
Block* GardenBlocks::CACTUS_FLOWER = nullptr;

// 萤火虫灌木（发光）
Block* GardenBlocks::FIREFLY_BUSH = nullptr;

// 灌木
Block* GardenBlocks::BUSH = nullptr;

void registerGardenBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 野花和枯叶
    // ============================================================================

    // 野花 - 地面装饰花，可堆叠放置（1-4朵），骨粉催熟
    // 使用 FlowerBedBlock 实现：FACING 属性（水平朝向）+ AMOUNT 属性（1-4 花朵数量）
    // 与粉红色花瓣（PINK_PETALS）共享同一方块类型 FlowerBedBlock
    // replaceable 标志允许同类型物品堆叠放置（isReplaceable 重写会检查物品类型）
    GardenBlocks::WILDFLOWERS =
        &registry.registerBlock<blocks::FlowerBedBlock>(ResourceLocation("minecraft:wildflowers"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .replaceable()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::PINK_PETALS));

    // 枯叶 - 地面装饰，可分段堆叠（1-4段）
    // 使用 LeafLitterBlock 实现：FACING 属性（水平朝向）+ SEGMENT_AMOUNT 属性（1-4 段数）
    // replaceable 标志允许同类型物品堆叠放置（isReplaceable 重写会检查物品类型）
    GardenBlocks::LEAF_LITTER =
        &registry.registerBlock<blocks::LeafLitterBlock>(ResourceLocation("minecraft:leaf_litter"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .replaceable()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 干草系列
    // ============================================================================

    // 矮干草 - 干草类植物，可生长在沙/陶瓦/泥土/耕地上（沙漠/恶地生物群系）
    GardenBlocks::SHORT_DRY_GRASS =
        &registry.registerBlock<blocks::DryVegetationBlock>(ResourceLocation("minecraft:short_dry_grass"),
            BlockProperties(Material::REPLACEABLE_PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::GRASS));

    // 高干草 - 干草类植物，可生长在沙/陶瓦/泥土/耕地上（沙漠/恶地生物群系）
    GardenBlocks::TALL_DRY_GRASS =
        &registry.registerBlock<blocks::DryVegetationBlock>(ResourceLocation("minecraft:tall_dry_grass"),
            BlockProperties(Material::REPLACEABLE_PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 仙人掌花
    // ============================================================================

    // 仙人掌花 - 生长在仙人掌上的花，可放置在仙人掌/耕地/实心顶面上
    GardenBlocks::CACTUS_FLOWER =
        &registry.registerBlock<blocks::CactusFlowerBlock>(ResourceLocation("minecraft:cactus_flower"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::CACTUS_FLOWER));

    // ============================================================================
    // 萤火虫灌木
    // ============================================================================

    // 萤火虫灌木 - 发光2级，继承 BushBlock 走默认 canSurvive（下方须 #dirt/耕地），
    // 修复世界生成时浮空于水面的 bug（此前注册为 SimpleBlock 致 canSurvive 终判失效）。
    GardenBlocks::FIREFLY_BUSH =
        &registry.registerBlock<blocks::FireflyBushBlock>(ResourceLocation("minecraft:firefly_bush"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::SWEET_BERRY_BUSH)
                .lightLevel(2));

    // ============================================================================
    // 灌木
    // ============================================================================

    // 灌木 - 通用装饰灌木，继承 BushBlock 走默认 canSurvive（下方须 #dirt/耕地），
    // 修复世界生成时浮空的 bug（此前注册为 SimpleBlock 致 canSurvive 终判失效）。
    // 类名 BushPlantBlock 用以区别植物基类 BushBlock。
    GardenBlocks::BUSH = &registry.registerBlock<blocks::BushPlantBlock>(ResourceLocation("minecraft:bush"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));
}

} // namespace block_registry
} // namespace mc
