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

#include "world/block/registry/TuffBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"

namespace mc {
namespace block_registry {

// 基础凝灰岩
Block* TuffBlocks::TUFF = nullptr;
Block* TuffBlocks::POLISHED_TUFF = nullptr;
Block* TuffBlocks::TUFF_BRICKS = nullptr;
Block* TuffBlocks::CHISELED_TUFF = nullptr;
Block* TuffBlocks::CHISELED_TUFF_BRICKS = nullptr;

// 凝灰岩楼梯、台阶、墙
Block* TuffBlocks::TUFF_STAIRS = nullptr;
Block* TuffBlocks::TUFF_SLAB = nullptr;
Block* TuffBlocks::TUFF_WALL = nullptr;

// 磨制凝灰岩楼梯、台阶、墙
Block* TuffBlocks::POLISHED_TUFF_STAIRS = nullptr;
Block* TuffBlocks::POLISHED_TUFF_SLAB = nullptr;
Block* TuffBlocks::POLISHED_TUFF_WALL = nullptr;

// 凝灰岩砖楼梯、台阶、墙
Block* TuffBlocks::TUFF_BRICK_STAIRS = nullptr;
Block* TuffBlocks::TUFF_BRICK_SLAB = nullptr;
Block* TuffBlocks::TUFF_BRICK_WALL = nullptr;

void registerTuffBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 凝灰岩基础方块
    // ============================================================================

    // 凝灰岩 - 粗糙的火山岩
    TuffBlocks::TUFF = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:tuff"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::TUFF));

    // 磨制凝灰岩
    TuffBlocks::POLISHED_TUFF = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_tuff"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::POLISHED_TUFF));

    // 凝灰岩砖
    TuffBlocks::TUFF_BRICKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:tuff_bricks"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .soundType(BlockSoundTypes::TUFF_BRICKS));

    // 雕刻凝灰岩
    TuffBlocks::CHISELED_TUFF = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_tuff"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::TUFF));

    // 雕刻凝灰岩砖
    TuffBlocks::CHISELED_TUFF_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_tuff_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::TUFF_BRICKS));

    // ============================================================================
    // 凝灰岩楼梯、台阶、墙
    // ============================================================================

    // 凝灰岩楼梯
    TuffBlocks::TUFF_STAIRS = &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:tuff_stairs"),
        TuffBlocks::TUFF->defaultState(),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::TUFF));

    // 凝灰岩台阶
    TuffBlocks::TUFF_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:tuff_slab"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::TUFF));

    // 凝灰岩墙
    TuffBlocks::TUFF_WALL = &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:tuff_wall"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .soundType(BlockSoundTypes::TUFF));

    // ============================================================================
    // 磨制凝灰岩楼梯、台阶、墙
    // ============================================================================

    // 磨制凝灰岩楼梯
    TuffBlocks::POLISHED_TUFF_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_tuff_stairs"),
            TuffBlocks::POLISHED_TUFF->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::POLISHED_TUFF));

    // 磨制凝灰岩台阶
    TuffBlocks::POLISHED_TUFF_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_tuff_slab"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::POLISHED_TUFF));

    // 磨制凝灰岩墙
    TuffBlocks::POLISHED_TUFF_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:polished_tuff_wall"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::POLISHED_TUFF));

    // ============================================================================
    // 凝灰岩砖楼梯、台阶、墙
    // ============================================================================

    // 凝灰岩砖楼梯
    TuffBlocks::TUFF_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:tuff_brick_stairs"),
            TuffBlocks::TUFF_BRICKS->defaultState(),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::TUFF_BRICKS));

    // 凝灰岩砖台阶
    TuffBlocks::TUFF_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:tuff_brick_slab"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::TUFF_BRICKS));

    // 凝灰岩砖墙
    TuffBlocks::TUFF_BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:tuff_brick_wall"),
            BlockProperties(Material::ROCK)
                .hardness(1.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .soundType(BlockSoundTypes::TUFF_BRICKS));
}

} // namespace block_registry
} // namespace mc
