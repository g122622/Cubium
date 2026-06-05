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

#include "world/block/registry/TrailsBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/vegetation/DoublePlantBlock.hpp"

namespace mc {
namespace block_registry {

// 可疑方块（考古）
Block* TrailsBlocks::SUSPICIOUS_SAND = nullptr;
Block* TrailsBlocks::SUSPICIOUS_GRAVEL = nullptr;

// 雕书架
Block* TrailsBlocks::CHISELED_BOOKSHELF = nullptr;

// 饰纹陶罐
Block* TrailsBlocks::DECORATED_POT = nullptr;

// 监守者蛋
Block* TrailsBlocks::SNIFFER_EGG = nullptr;

// 粉红色花瓣
Block* TrailsBlocks::PINK_PETALS = nullptr;

// 火把花
Block* TrailsBlocks::TORCHFLOWER = nullptr;

// 瓶草
Block* TrailsBlocks::PITCHER_PLANT = nullptr;

// 作物
Block* TrailsBlocks::TORCHFLOWER_CROP = nullptr;
Block* TrailsBlocks::PITCHER_CROP = nullptr;

void registerTrailsBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 可疑方块（考古）
    // ============================================================================

    // 可疑的沙 - 受重力影响，可以被刷子刷出物品
    TrailsBlocks::SUSPICIOUS_SAND = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:suspicious_sand"),
        BlockProperties(Material::SAND).hardness(0.25f).resistance(0.25f).soundType(BlockSoundTypes::SUSPICIOUS_SAND));

    // 可疑的沙砾 - 受重力影响，可以被刷子刷出物品
    TrailsBlocks::SUSPICIOUS_GRAVEL =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:suspicious_gravel"),
            BlockProperties(Material::SAND)
                .hardness(0.25f)
                .resistance(0.25f)
                .soundType(BlockSoundTypes::SUSPICIOUS_GRAVEL));

    // ============================================================================
    // 雕书架
    // ============================================================================

    // 雕书架 - 可以放置书，可被红石比较器检测
    TrailsBlocks::CHISELED_BOOKSHELF =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_bookshelf"),
            BlockProperties(Material::WOOD)
                .hardness(1.5f)
                .resistance(1.5f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::CHISELED_BOOKSHELF)
                .flammable());

    // ============================================================================
    // 饰纹陶罐
    // ============================================================================

    // 饰纹陶罐 - 由陶片合成，无碰撞箱
    TrailsBlocks::DECORATED_POT = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:decorated_pot"),
        BlockProperties(Material::DECORATION)
            .hardness(0.0f)
            .resistance(0.0f)
            .soundType(BlockSoundTypes::DECORATED_POT)
            .noCollision());

    // ============================================================================
    // 监守者蛋
    // ============================================================================

    // 监守者蛋 - 可孵化出监守者生物
    TrailsBlocks::SNIFFER_EGG = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:sniffer_egg"),
        BlockProperties(Material::EARTH).hardness(0.5f).resistance(0.5f).soundType(BlockSoundTypes::SNIFFER_EGG));

    // ============================================================================
    // 粉红色花瓣
    // ============================================================================

    // 粉红色花瓣 - 樱花林生物群系的装饰植物
    TrailsBlocks::PINK_PETALS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pink_petals"),
        BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::PINK_PETALS));

    // ============================================================================
    // 火把花
    // ============================================================================

    // 火把花 - 由监守者发现的古代植物
    TrailsBlocks::TORCHFLOWER = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:torchflower"),
        BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::GRASS));

    // ============================================================================
    // 瓶草
    // ============================================================================

    // 瓶草 - 双层植物
    TrailsBlocks::PITCHER_PLANT =
        &registry.registerBlock<blocks::DoublePlantBlock>(ResourceLocation("minecraft:pitcher_plant"),
            BlockProperties(Material::PLANT).noCollision().notSolid().soundType(BlockSoundTypes::CROP));

    // ============================================================================
    // 作物方块
    // ============================================================================

    // 火把花作物 - 火把花的作物形态
    TrailsBlocks::TORCHFLOWER_CROP =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:torchflower_crop"),
            BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP));

    // 瓶草作物 - 瓶草的作物形态
    TrailsBlocks::PITCHER_CROP = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:pitcher_crop"),
        BlockProperties(Material::PLANT).noCollision().notSolid().hardness(0.0f).soundType(BlockSoundTypes::CROP));
}

} // namespace block_registry
} // namespace mc
