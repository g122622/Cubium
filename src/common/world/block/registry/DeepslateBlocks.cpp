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

#include "world/block/registry/DeepslateBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/mob/InfestedBlock.hpp"
#include "world/block/blocks/mob/InfestedRotatedPillarBlock.hpp"
#include "world/block/blocks/redstone/RedstoneOreBlock.hpp"

namespace mc {
namespace block_registry {

// 深板岩基础方块
Block* DeepslateBlocks::DEEPSLATE = nullptr;
Block* DeepslateBlocks::COBBLED_DEEPSLATE = nullptr;
Block* DeepslateBlocks::POLISHED_DEEPSLATE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_BRICKS = nullptr;
Block* DeepslateBlocks::DEEPSLATE_TILES = nullptr;
Block* DeepslateBlocks::CHISELED_DEEPSLATE = nullptr;
Block* DeepslateBlocks::CRACKED_DEEPSLATE_BRICKS = nullptr;
Block* DeepslateBlocks::CRACKED_DEEPSLATE_TILES = nullptr;
Block* DeepslateBlocks::REINFORCED_DEEPSLATE = nullptr;

// 深板岩矿石变种
Block* DeepslateBlocks::DEEPSLATE_COAL_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_IRON_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_COPPER_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_GOLD_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_DIAMOND_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_LAPIS_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_EMERALD_ORE = nullptr;
Block* DeepslateBlocks::DEEPSLATE_REDSTONE_ORE = nullptr;

// 深板岩圆石建筑方块
Block* DeepslateBlocks::COBBLED_DEEPSLATE_STAIRS = nullptr;
Block* DeepslateBlocks::COBBLED_DEEPSLATE_SLAB = nullptr;
Block* DeepslateBlocks::COBBLED_DEEPSLATE_WALL = nullptr;

// 磨制深板岩建筑方块
Block* DeepslateBlocks::POLISHED_DEEPSLATE_STAIRS = nullptr;
Block* DeepslateBlocks::POLISHED_DEEPSLATE_SLAB = nullptr;
Block* DeepslateBlocks::POLISHED_DEEPSLATE_WALL = nullptr;

// 深板岩砖建筑方块
Block* DeepslateBlocks::DEEPSLATE_BRICK_STAIRS = nullptr;
Block* DeepslateBlocks::DEEPSLATE_BRICK_SLAB = nullptr;
Block* DeepslateBlocks::DEEPSLATE_BRICK_WALL = nullptr;

// 深板岩瓦建筑方块
Block* DeepslateBlocks::DEEPSLATE_TILE_STAIRS = nullptr;
Block* DeepslateBlocks::DEEPSLATE_TILE_SLAB = nullptr;
Block* DeepslateBlocks::DEEPSLATE_TILE_WALL = nullptr;

// 其他
Block* DeepslateBlocks::SMOOTH_BASALT = nullptr;
Block* DeepslateBlocks::INFESTED_DEEPSLATE = nullptr;

void registerDeepslateBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========== 深板岩基础方块 ==========
    // 深板岩 - RotatedPillarBlock，有轴属性
    DeepslateBlocks::DEEPSLATE = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:deepslate"),
        BlockProperties(Material::ROCK)
            .hardness(3.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(0)
            .requiresTool()
            .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩圆石
    DeepslateBlocks::COBBLED_DEEPSLATE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cobbled_deepslate"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::COBBLED_DEEPSLATE));

    // 磨制深板岩
    DeepslateBlocks::POLISHED_DEEPSLATE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_deepslate"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::POLISHED_DEEPSLATE));

    // 深板岩砖
    DeepslateBlocks::DEEPSLATE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE_BRICKS));

    // 深板岩瓦
    DeepslateBlocks::DEEPSLATE_TILES =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_tiles"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE_TILES));

    // 雕刻深板岩
    DeepslateBlocks::CHISELED_DEEPSLATE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:chiseled_deepslate"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE_BRICKS));

    // 裂纹深板岩砖
    DeepslateBlocks::CRACKED_DEEPSLATE_BRICKS =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cracked_deepslate_bricks"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE_BRICKS));

    // 裂纹深板岩瓦
    DeepslateBlocks::CRACKED_DEEPSLATE_TILES =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:cracked_deepslate_tiles"),
            BlockProperties(Material::ROCK)
                .hardness(3.5f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE_TILES));

    // 强化深板岩 - 极高硬度和抗性，无战利品表
    DeepslateBlocks::REINFORCED_DEEPSLATE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:reinforced_deepslate"),
            BlockProperties(Material::ROCK)
                .hardness(55.0f)
                .resistance(1200.0f)
                .soundType(BlockSoundTypes::DEEPSLATE)
                .noLootTable());

    // ========== 深板岩矿石变种 ==========
    BlockProperties deepslateOreProps = BlockProperties(Material::ROCK)
                                            .hardness(4.5f)
                                            .resistance(6.0f)
                                            .harvestTool(HarvestTool::Pickaxe)
                                            .requiresTool()
                                            .soundType(BlockSoundTypes::DEEPSLATE);

    // 深板岩煤矿石 - harvestLevel 0
    DeepslateBlocks::DEEPSLATE_COAL_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_coal_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(0)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩铁矿石 - harvestLevel 1
    DeepslateBlocks::DEEPSLATE_IRON_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_iron_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(1)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩铜矿石 - harvestLevel 1
    DeepslateBlocks::DEEPSLATE_COPPER_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_copper_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(1)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩金矿石 - harvestLevel 2
    DeepslateBlocks::DEEPSLATE_GOLD_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_gold_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩钻石矿石 - harvestLevel 2
    DeepslateBlocks::DEEPSLATE_DIAMOND_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_diamond_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩青金石矿石 - harvestLevel 1
    DeepslateBlocks::DEEPSLATE_LAPIS_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_lapis_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(1)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩绿宝石矿石 - harvestLevel 2
    DeepslateBlocks::DEEPSLATE_EMERALD_ORE =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:deepslate_emerald_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // 深板岩红石矿石 - RedstoneOreBlock, LIT属性控制发光
    DeepslateBlocks::DEEPSLATE_REDSTONE_ORE =
        &registry.registerBlock<blocks::RedstoneOreBlock>(ResourceLocation("minecraft:deepslate_redstone_ore"),
            BlockProperties(Material::ROCK)
                .hardness(4.5f)
                .resistance(3.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(2)
                .requiresTool()
                .soundType(BlockSoundTypes::DEEPSLATE));

    // ========== 深板岩圆石建筑方块 ==========
    BlockProperties cobbledDeepslateProps = BlockProperties(Material::ROCK)
                                                .hardness(3.0f)
                                                .resistance(6.0f)
                                                .harvestTool(HarvestTool::Pickaxe)
                                                .harvestLevel(0)
                                                .requiresTool()
                                                .soundType(BlockSoundTypes::COBBLED_DEEPSLATE);

    DeepslateBlocks::COBBLED_DEEPSLATE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:cobbled_deepslate_stairs"),
            DeepslateBlocks::COBBLED_DEEPSLATE->defaultState(),
            cobbledDeepslateProps);

    DeepslateBlocks::COBBLED_DEEPSLATE_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:cobbled_deepslate_slab"), cobbledDeepslateProps);

    DeepslateBlocks::COBBLED_DEEPSLATE_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:cobbled_deepslate_wall"), cobbledDeepslateProps);

    // ========== 磨制深板岩建筑方块 ==========
    BlockProperties polishedDeepslateProps = BlockProperties(Material::ROCK)
                                                 .hardness(3.5f)
                                                 .resistance(6.0f)
                                                 .harvestTool(HarvestTool::Pickaxe)
                                                 .harvestLevel(0)
                                                 .requiresTool()
                                                 .soundType(BlockSoundTypes::POLISHED_DEEPSLATE);

    DeepslateBlocks::POLISHED_DEEPSLATE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_deepslate_stairs"),
            DeepslateBlocks::POLISHED_DEEPSLATE->defaultState(),
            polishedDeepslateProps);

    DeepslateBlocks::POLISHED_DEEPSLATE_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:polished_deepslate_slab"), polishedDeepslateProps);

    DeepslateBlocks::POLISHED_DEEPSLATE_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:polished_deepslate_wall"), polishedDeepslateProps);

    // ========== 深板岩砖建筑方块 ==========
    BlockProperties deepslateBrickProps = BlockProperties(Material::ROCK)
                                              .hardness(3.5f)
                                              .resistance(6.0f)
                                              .harvestTool(HarvestTool::Pickaxe)
                                              .harvestLevel(0)
                                              .requiresTool()
                                              .soundType(BlockSoundTypes::DEEPSLATE_BRICKS);

    DeepslateBlocks::DEEPSLATE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:deepslate_brick_stairs"),
            DeepslateBlocks::DEEPSLATE_BRICKS->defaultState(),
            deepslateBrickProps);

    DeepslateBlocks::DEEPSLATE_BRICK_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:deepslate_brick_slab"), deepslateBrickProps);

    DeepslateBlocks::DEEPSLATE_BRICK_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:deepslate_brick_wall"), deepslateBrickProps);

    // ========== 深板岩瓦建筑方块 ==========
    BlockProperties deepslateTileProps = BlockProperties(Material::ROCK)
                                             .hardness(3.5f)
                                             .resistance(6.0f)
                                             .harvestTool(HarvestTool::Pickaxe)
                                             .harvestLevel(0)
                                             .requiresTool()
                                             .soundType(BlockSoundTypes::DEEPSLATE_TILES);

    DeepslateBlocks::DEEPSLATE_TILE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:deepslate_tile_stairs"),
            DeepslateBlocks::DEEPSLATE_TILES->defaultState(),
            deepslateTileProps);

    DeepslateBlocks::DEEPSLATE_TILE_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:deepslate_tile_slab"), deepslateTileProps);

    DeepslateBlocks::DEEPSLATE_TILE_WALL = &registry.registerBlock<blocks::WallBlock>(
        ResourceLocation("minecraft:deepslate_tile_wall"), deepslateTileProps);

    // ========== 其他 ==========
    // 平滑玄武岩
    DeepslateBlocks::SMOOTH_BASALT = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:smooth_basalt"),
        BlockProperties(Material::ROCK)
            .hardness(1.25f)
            .resistance(4.2f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(0)
            .requiresTool()
            .soundType(BlockSoundTypes::BASALT));

    // ========== 虫蚀深板岩 ==========
    // 虫蚀深板岩 - 蠹虫会从其中生成，带 AXIS 属性可绕 Y 轴旋转，使用深板岩音效
    DeepslateBlocks::INFESTED_DEEPSLATE =
        &registry.registerBlock<blocks::InfestedRotatedPillarBlock>(ResourceLocation("minecraft:infested_deepslate"),
            DeepslateBlocks::DEEPSLATE->blockId(),
            BlockProperties(Material::EARTH).hardness(0.0f).resistance(0.75f).soundType(BlockSoundTypes::DEEPSLATE));

    // 注册虫蚀映射
    blocks::InfestedBlock::registerInfestedBlock(
        DeepslateBlocks::DEEPSLATE->blockId(), DeepslateBlocks::INFESTED_DEEPSLATE->blockId());
}

} // namespace block_registry
} // namespace mc
