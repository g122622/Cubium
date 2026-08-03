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
 *
 */

#include "world/block/registry/ShelfBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/ShelfBlock.hpp"

namespace mc {
namespace block_registry {

// 传统木材书架
Block* ShelfBlocks::OAK_SHELF = nullptr;
Block* ShelfBlocks::SPRUCE_SHELF = nullptr;
Block* ShelfBlocks::BIRCH_SHELF = nullptr;
Block* ShelfBlocks::JUNGLE_SHELF = nullptr;
Block* ShelfBlocks::ACACIA_SHELF = nullptr;
Block* ShelfBlocks::DARK_OAK_SHELF = nullptr;

// 红树木书架
Block* ShelfBlocks::MANGROVE_SHELF = nullptr;

// 樱花木书架
Block* ShelfBlocks::CHERRY_SHELF = nullptr;

// 苍白橡木书架
Block* ShelfBlocks::PALE_OAK_SHELF = nullptr;

// 竹木书架
Block* ShelfBlocks::BAMBOO_SHELF = nullptr;

// 下界木质书架（不可燃）
Block* ShelfBlocks::CRIMSON_SHELF = nullptr;
Block* ShelfBlocks::WARPED_SHELF = nullptr;

void registerShelfBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ============================================================================
    // 传统木材书架
    // ============================================================================

    // 橡木书架
    ShelfBlocks::OAK_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:oak_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // 云杉木书架
    ShelfBlocks::SPRUCE_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:spruce_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // 白桦木书架
    ShelfBlocks::BIRCH_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:birch_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // 丛林木书架
    ShelfBlocks::JUNGLE_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:jungle_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // 金合欢木书架
    ShelfBlocks::ACACIA_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:acacia_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // 深色橡木书架
    ShelfBlocks::DARK_OAK_SHELF =
        &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:dark_oak_shelf"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::SHELF)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 红树木书架
    // ============================================================================

    // 红树木书架
    ShelfBlocks::MANGROVE_SHELF =
        &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:mangrove_shelf"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::SHELF)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 樱花木书架
    // ============================================================================

    // 樱花木书架
    ShelfBlocks::CHERRY_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:cherry_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // ============================================================================
    // 苍白橡木书架
    // ============================================================================

    // 苍白橡木书架
    ShelfBlocks::PALE_OAK_SHELF =
        &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:pale_oak_shelf"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::SHELF)
                .flammable()
                .ignitedByLava());

    // ============================================================================
    // 竹木书架
    // ============================================================================

    // 竹木书架
    ShelfBlocks::BAMBOO_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:bamboo_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .flammable()
            .ignitedByLava());

    // ============================================================================
    // 下界木质书架（不可燃，使用菌柄声音类型）
    // ============================================================================

    // 绯红菌书架 - 下界木质不可燃
    ShelfBlocks::CRIMSON_SHELF =
        &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:crimson_shelf"),
            BlockProperties(Material::WOOD)
                .hardness(2.0f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Axe)
                .soundType(BlockSoundTypes::SHELF)
                .ignitedByLava());

    // 诡异菌书架 - 下界木质不可燃
    ShelfBlocks::WARPED_SHELF = &registry.registerBlock<blocks::ShelfBlock>(ResourceLocation("minecraft:warped_shelf"),
        BlockProperties(Material::WOOD)
            .hardness(2.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Axe)
            .soundType(BlockSoundTypes::SHELF)
            .ignitedByLava());
}

} // namespace block_registry
} // namespace mc
