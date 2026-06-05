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
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/SimpleBlock.hpp"

namespace mc {
namespace block_registry {

// 眼花系列（发光花朵）
Block* GardenBlocks::OPEN_EYEBLOSSOM = nullptr;
Block* GardenBlocks::CLOSED_EYEBLOSSOM = nullptr;

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
    // 眼花系列（1.21.2新增的发光花朵）
    // ============================================================================

    // 开放的眼花 - 发光15级
    GardenBlocks::OPEN_EYEBLOSSOM = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:open_eyeblossom"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::EYEBLOSSOM)
            .lightLevel(15));

    // 闭合的眼花 - 不发光
    GardenBlocks::CLOSED_EYEBLOSSOM =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:closed_eyeblossom"),
            BlockProperties(Material::PLANT)
                .noCollision()
                .notSolid()
                .hardness(0.0f)
                .resistance(0.0f)
                .soundType(BlockSoundTypes::EYEBLOSSOM));

    // ============================================================================
    // 野花和枯叶
    // ============================================================================

    // 野花 - 地面装饰花
    GardenBlocks::WILDFLOWERS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:wildflowers"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));

    // 枯叶 - 地面装饰
    GardenBlocks::LEAF_LITTER = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:leaf_litter"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 干草系列
    // ============================================================================

    // 矮干草
    GardenBlocks::SHORT_DRY_GRASS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:short_dry_grass"),
        BlockProperties(Material::REPLACEABLE_PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));

    // 高干草
    GardenBlocks::TALL_DRY_GRASS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:tall_dry_grass"),
        BlockProperties(Material::REPLACEABLE_PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 仙人掌花
    // ============================================================================

    // 仙人掌花 - 生长在仙人掌上的花
    GardenBlocks::CACTUS_FLOWER = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cactus_flower"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 萤火虫灌木
    // ============================================================================

    // 萤火虫灌木 - 发光2级
    GardenBlocks::FIREFLY_BUSH = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:firefly_bush"),
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

    // 灌木 - 通用灌木方块
    GardenBlocks::BUSH = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:bush"),
        BlockProperties(Material::PLANT)
            .noCollision()
            .notSolid()
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::GRASS));
}

} // namespace block_registry
} // namespace mc
