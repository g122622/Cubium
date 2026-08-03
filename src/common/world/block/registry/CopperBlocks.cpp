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

#include "world/block/registry/CopperBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/LightningRodBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/copper/CopperBulbBlock.hpp"
#include "world/block/blocks/copper/CopperChestBlock.hpp"
#include "world/block/blocks/copper/CopperGolemStatueBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperBarsBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperChainBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperDoorBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperGrateBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperLanternBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperSlabBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperStairBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperTrapDoorBlock.hpp"
#include "world/block/blocks/copper/WeatheringLightningRodBlock.hpp"

namespace mc {
namespace block_registry {

// ============================================================================
// 铜块系列（4个氧化等级 + 4个涂蜡变种 = 8个）
// ============================================================================
Block* CopperBlocks::COPPER_BLOCK = nullptr;
Block* CopperBlocks::EXPOSED_COPPER = nullptr;
Block* CopperBlocks::WEATHERED_COPPER = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER = nullptr;
Block* CopperBlocks::WAXED_COPPER_BLOCK = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER = nullptr;

// ============================================================================
// 切制铜系列（8个）
// ============================================================================
Block* CopperBlocks::CUT_COPPER = nullptr;
Block* CopperBlocks::EXPOSED_CUT_COPPER = nullptr;
Block* CopperBlocks::WEATHERED_CUT_COPPER = nullptr;
Block* CopperBlocks::OXIDIZED_CUT_COPPER = nullptr;
Block* CopperBlocks::WAXED_CUT_COPPER = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_CUT_COPPER = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_CUT_COPPER = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_CUT_COPPER = nullptr;

// ============================================================================
// 切制铜楼梯（8个）
// ============================================================================
Block* CopperBlocks::CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::EXPOSED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::WEATHERED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::OXIDIZED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::WAXED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS = nullptr;

// ============================================================================
// 切制铜台阶（8个）
// ============================================================================
Block* CopperBlocks::CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::EXPOSED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::WEATHERED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::OXIDIZED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::WAXED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB = nullptr;

// ============================================================================
// 1.21 铜扩展：铜门（8个）
// ============================================================================
Block* CopperBlocks::COPPER_DOOR = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_DOOR = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_DOOR = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_DOOR = nullptr;
Block* CopperBlocks::WAXED_COPPER_DOOR = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_DOOR = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_DOOR = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_DOOR = nullptr;

// ============================================================================
// 1.21 铜扩展：铜活板门（8个）
// ============================================================================
Block* CopperBlocks::COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::WAXED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR = nullptr;

// ============================================================================
// 1.21 铜扩展：铜格栅（8个）
// ============================================================================
Block* CopperBlocks::COPPER_GRATE = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_GRATE = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_GRATE = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_GRATE = nullptr;
Block* CopperBlocks::WAXED_COPPER_GRATE = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_GRATE = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_GRATE = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_GRATE = nullptr;

// ============================================================================
// 1.21 铜扩展：铜灯（8个）
// ============================================================================
Block* CopperBlocks::COPPER_BULB = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_BULB = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_BULB = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_BULB = nullptr;
Block* CopperBlocks::WAXED_COPPER_BULB = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_BULB = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_BULB = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_BULB = nullptr;

// ============================================================================
// 1.21 铜扩展：凿制铜（8个）
// ============================================================================
Block* CopperBlocks::CHISELED_COPPER = nullptr;
Block* CopperBlocks::EXPOSED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::WEATHERED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::OXIDIZED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::WAXED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_CHISELED_COPPER = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_CHISELED_COPPER = nullptr;

// ============================================================================
// 1.21 铜扩展：铜栏杆（8个）
// ============================================================================
Block* CopperBlocks::COPPER_BARS = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_BARS = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_BARS = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_BARS = nullptr;
Block* CopperBlocks::WAXED_COPPER_BARS = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_BARS = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_BARS = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_BARS = nullptr;

// ============================================================================
// 1.21 铜扩展：铜链（8个）
// ============================================================================
Block* CopperBlocks::COPPER_CHAIN = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::WAXED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_CHAIN = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_CHAIN = nullptr;

// ============================================================================
// 1.21 铜扩展：铜灯笼（8个）
// ============================================================================
Block* CopperBlocks::COPPER_LANTERN = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::WAXED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_LANTERN = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_LANTERN = nullptr;

// ============================================================================
// 避雷针（1.17 基础 + 1.21 铜扩展氧化变种）
// ============================================================================
Block* CopperBlocks::LIGHTNING_ROD = nullptr;
Block* CopperBlocks::EXPOSED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::WEATHERED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::OXIDIZED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::WAXED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_LIGHTNING_ROD = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_LIGHTNING_ROD = nullptr;

// ============================================================================
// 1.21.11 铜傀儡雕像（8个）
// ============================================================================
Block* CopperBlocks::COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::WAXED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE = nullptr;

// ============================================================================
// 1.21.11 铜箱子（8个）
// ============================================================================
Block* CopperBlocks::COPPER_CHEST = nullptr;
Block* CopperBlocks::EXPOSED_COPPER_CHEST = nullptr;
Block* CopperBlocks::WEATHERED_COPPER_CHEST = nullptr;
Block* CopperBlocks::OXIDIZED_COPPER_CHEST = nullptr;
Block* CopperBlocks::WAXED_COPPER_CHEST = nullptr;
Block* CopperBlocks::WAXED_EXPOSED_COPPER_CHEST = nullptr;
Block* CopperBlocks::WAXED_WEATHERED_COPPER_CHEST = nullptr;
Block* CopperBlocks::WAXED_OXIDIZED_COPPER_CHEST = nullptr;

// ============================================================================
// 粗矿块
// ============================================================================
Block* CopperBlocks::RAW_IRON_BLOCK = nullptr;
Block* CopperBlocks::RAW_COPPER_BLOCK = nullptr;
Block* CopperBlocks::RAW_GOLD_BLOCK = nullptr;

void registerCopperBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 铜块基础属性: Material::IRON, 硬度3.0, 抗爆6.0, 镐, 声音类型COPPER
    BlockProperties copperBlockProps = BlockProperties(Material::IRON)
                                           .hardness(3.0f)
                                           .resistance(6.0f)
                                           .harvestTool(HarvestTool::Pickaxe)
                                           .requiresTool()
                                           .soundType(BlockSoundTypes::COPPER);

    // 切制铜属性（与铜块相同）
    BlockProperties cutCopperProps = BlockProperties(Material::IRON)
                                         .hardness(3.0f)
                                         .resistance(6.0f)
                                         .harvestTool(HarvestTool::Pickaxe)
                                         .requiresTool()
                                         .soundType(BlockSoundTypes::COPPER);

    // 楼梯和台阶属性（与切制铜相同）
    BlockProperties copperStairSlabProps = BlockProperties(Material::IRON)
                                               .hardness(3.0f)
                                               .resistance(6.0f)
                                               .harvestTool(HarvestTool::Pickaxe)
                                               .requiresTool()
                                               .soundType(BlockSoundTypes::COPPER);

    // 铜门/活板门属性: Material::IRON, 硬度3.0, 抗爆6.0, notSolid
    BlockProperties copperDoorProps = BlockProperties(Material::IRON)
                                          .hardness(3.0f)
                                          .resistance(6.0f)
                                          .harvestTool(HarvestTool::Pickaxe)
                                          .requiresTool()
                                          .notSolid()
                                          .soundType(BlockSoundTypes::COPPER);

    // 铜格栅属性: Material::IRON, 硬度3.0, 抗爆6.0, notSolid
    BlockProperties copperGrateProps = BlockProperties(Material::IRON)
                                           .hardness(3.0f)
                                           .resistance(6.0f)
                                           .harvestTool(HarvestTool::Pickaxe)
                                           .requiresTool()
                                           .notSolid()
                                           .soundType(BlockSoundTypes::COPPER_GRATE);

    // 铜灯属性: Material::IRON, 硬度3.0, 抗爆6.0
    BlockProperties copperBulbProps = BlockProperties(Material::IRON)
                                          .hardness(3.0f)
                                          .resistance(6.0f)
                                          .harvestTool(HarvestTool::Pickaxe)
                                          .requiresTool()
                                          .soundType(BlockSoundTypes::COPPER_BULB);

    // 凿制铜属性（与铜块相同）
    BlockProperties chiseledCopperProps = BlockProperties(Material::IRON)
                                              .hardness(3.0f)
                                              .resistance(6.0f)
                                              .harvestTool(HarvestTool::Pickaxe)
                                              .requiresTool()
                                              .soundType(BlockSoundTypes::COPPER);

    // ============================================================================
    // 铜块系列（8个）
    // 前4个使用WeatheringCopperBlock可氧化，后4个使用WaxedCopperBlock不可氧化
    // ============================================================================
    auto* copperBlock = &registry.registerBlock<blocks::WeatheringCopperBlock>(
        ResourceLocation("minecraft:copper_block"), copperBlockProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopper = &registry.registerBlock<blocks::WeatheringCopperBlock>(
        ResourceLocation("minecraft:exposed_copper"), copperBlockProps, BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:weathered_copper"),
            copperBlockProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:oxidized_copper"),
            copperBlockProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链: copper_block -> exposed_copper -> weathered_copper -> oxidized_copper
    copperBlock->setNextOxidationBlock(exposedCopper);
    exposedCopper->setNextOxidationBlock(weatheredCopper);
    weatheredCopper->setNextOxidationBlock(oxidizedCopper);
    // oxidized_copper 的 m_nextOxidationBlock 保持 nullptr（最高等级）

    // 设置反向氧化链（用于斧头刮削）
    exposedCopper->setPreviousOxidationBlock(copperBlock);
    weatheredCopper->setPreviousOxidationBlock(exposedCopper);
    oxidizedCopper->setPreviousOxidationBlock(weatheredCopper);
    // copper_block 的 m_previousOxidationBlock 保持 nullptr（最低等级）

    CopperBlocks::COPPER_BLOCK = copperBlock;
    CopperBlocks::EXPOSED_COPPER = exposedCopper;
    CopperBlocks::WEATHERED_COPPER = weatheredCopper;
    CopperBlocks::OXIDIZED_COPPER = oxidizedCopper;

    // 涂蜡铜块
    CopperBlocks::WAXED_COPPER_BLOCK = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_copper_block"), copperBlockProps);

    CopperBlocks::WAXED_EXPOSED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper"), copperBlockProps);

    CopperBlocks::WAXED_WEATHERED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper"), copperBlockProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper"), copperBlockProps);

    // ============================================================================
    // 切制铜系列（8个）
    // ============================================================================
    auto* cutCopper = &registry.registerBlock<blocks::WeatheringCopperBlock>(
        ResourceLocation("minecraft:cut_copper"), cutCopperProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCutCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:exposed_cut_copper"),
            cutCopperProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCutCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:weathered_cut_copper"),
            cutCopperProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCutCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:oxidized_cut_copper"),
            cutCopperProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链
    cutCopper->setNextOxidationBlock(exposedCutCopper);
    exposedCutCopper->setNextOxidationBlock(weatheredCutCopper);
    weatheredCutCopper->setNextOxidationBlock(oxidizedCutCopper);

    // 设置反向氧化链（用于斧头刮削）
    exposedCutCopper->setPreviousOxidationBlock(cutCopper);
    weatheredCutCopper->setPreviousOxidationBlock(exposedCutCopper);
    oxidizedCutCopper->setPreviousOxidationBlock(weatheredCutCopper);

    CopperBlocks::CUT_COPPER = cutCopper;
    CopperBlocks::EXPOSED_CUT_COPPER = exposedCutCopper;
    CopperBlocks::WEATHERED_CUT_COPPER = weatheredCutCopper;
    CopperBlocks::OXIDIZED_CUT_COPPER = oxidizedCutCopper;

    CopperBlocks::WAXED_CUT_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_cut_copper"), cutCopperProps);

    CopperBlocks::WAXED_EXPOSED_CUT_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_exposed_cut_copper"), cutCopperProps);

    CopperBlocks::WAXED_WEATHERED_CUT_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_weathered_cut_copper"), cutCopperProps);

    CopperBlocks::WAXED_OXIDIZED_CUT_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_oxidized_cut_copper"), cutCopperProps);

    // ============================================================================
    // 切制铜楼梯（8个）
    // ============================================================================
    CopperBlocks::CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::WeatheringCopperStairBlock>(ResourceLocation("minecraft:cut_copper_stairs"),
            CopperBlocks::CUT_COPPER->defaultState(),
            copperStairSlabProps,
            BlockStateProperties::OxidationLevel::Unaffected);

    CopperBlocks::EXPOSED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WeatheringCopperStairBlock>(
        ResourceLocation("minecraft:exposed_cut_copper_stairs"),
        CopperBlocks::EXPOSED_CUT_COPPER->defaultState(),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Exposed);

    CopperBlocks::WEATHERED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WeatheringCopperStairBlock>(
        ResourceLocation("minecraft:weathered_cut_copper_stairs"),
        CopperBlocks::WEATHERED_CUT_COPPER->defaultState(),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Weathered);

    CopperBlocks::OXIDIZED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WeatheringCopperStairBlock>(
        ResourceLocation("minecraft:oxidized_cut_copper_stairs"),
        CopperBlocks::OXIDIZED_CUT_COPPER->defaultState(),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Oxidized);

    CopperBlocks::WAXED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::WaxedCopperStairBlock>(ResourceLocation("minecraft:waxed_cut_copper_stairs"),
            CopperBlocks::WAXED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WaxedCopperStairBlock>(
        ResourceLocation("minecraft:waxed_exposed_cut_copper_stairs"),
        CopperBlocks::WAXED_EXPOSED_CUT_COPPER->defaultState(),
        copperStairSlabProps);

    CopperBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WaxedCopperStairBlock>(
        ResourceLocation("minecraft:waxed_weathered_cut_copper_stairs"),
        CopperBlocks::WAXED_WEATHERED_CUT_COPPER->defaultState(),
        copperStairSlabProps);

    CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS = &registry.registerBlock<blocks::WaxedCopperStairBlock>(
        ResourceLocation("minecraft:waxed_oxidized_cut_copper_stairs"),
        CopperBlocks::WAXED_OXIDIZED_CUT_COPPER->defaultState(),
        copperStairSlabProps);

    // ============================================================================
    // 切制铜台阶（8个）
    // ============================================================================
    CopperBlocks::CUT_COPPER_SLAB =
        &registry.registerBlock<blocks::WeatheringCopperSlabBlock>(ResourceLocation("minecraft:cut_copper_slab"),
            copperStairSlabProps,
            BlockStateProperties::OxidationLevel::Unaffected);

    CopperBlocks::EXPOSED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WeatheringCopperSlabBlock>(
        ResourceLocation("minecraft:exposed_cut_copper_slab"),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Exposed);

    CopperBlocks::WEATHERED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WeatheringCopperSlabBlock>(
        ResourceLocation("minecraft:weathered_cut_copper_slab"),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Weathered);

    CopperBlocks::OXIDIZED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WeatheringCopperSlabBlock>(
        ResourceLocation("minecraft:oxidized_cut_copper_slab"),
        copperStairSlabProps,
        BlockStateProperties::OxidationLevel::Oxidized);

    CopperBlocks::WAXED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WaxedCopperSlabBlock>(
        ResourceLocation("minecraft:waxed_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WaxedCopperSlabBlock>(
        ResourceLocation("minecraft:waxed_exposed_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WaxedCopperSlabBlock>(
        ResourceLocation("minecraft:waxed_weathered_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::WaxedCopperSlabBlock>(
        ResourceLocation("minecraft:waxed_oxidized_cut_copper_slab"), copperStairSlabProps);

    // 设置切制铜楼梯氧化链
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::CUT_COPPER_STAIRS)
        ->setNextOxidationBlock(CopperBlocks::EXPOSED_CUT_COPPER_STAIRS);
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::EXPOSED_CUT_COPPER_STAIRS)
        ->setNextOxidationBlock(CopperBlocks::WEATHERED_CUT_COPPER_STAIRS);
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::WEATHERED_CUT_COPPER_STAIRS)
        ->setNextOxidationBlock(CopperBlocks::OXIDIZED_CUT_COPPER_STAIRS);

    // 设置切制铜楼梯反向氧化链（用于斧头刮削）
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::EXPOSED_CUT_COPPER_STAIRS)
        ->setPreviousOxidationBlock(CopperBlocks::CUT_COPPER_STAIRS);
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::WEATHERED_CUT_COPPER_STAIRS)
        ->setPreviousOxidationBlock(CopperBlocks::EXPOSED_CUT_COPPER_STAIRS);
    static_cast<blocks::WeatheringCopperStairBlock*>(CopperBlocks::OXIDIZED_CUT_COPPER_STAIRS)
        ->setPreviousOxidationBlock(CopperBlocks::WEATHERED_CUT_COPPER_STAIRS);

    // 设置切制铜台阶氧化链
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::CUT_COPPER_SLAB)
        ->setNextOxidationBlock(CopperBlocks::EXPOSED_CUT_COPPER_SLAB);
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::EXPOSED_CUT_COPPER_SLAB)
        ->setNextOxidationBlock(CopperBlocks::WEATHERED_CUT_COPPER_SLAB);
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::WEATHERED_CUT_COPPER_SLAB)
        ->setNextOxidationBlock(CopperBlocks::OXIDIZED_CUT_COPPER_SLAB);

    // 设置切制铜台阶反向氧化链（用于斧头刮削）
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::EXPOSED_CUT_COPPER_SLAB)
        ->setPreviousOxidationBlock(CopperBlocks::CUT_COPPER_SLAB);
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::WEATHERED_CUT_COPPER_SLAB)
        ->setPreviousOxidationBlock(CopperBlocks::EXPOSED_CUT_COPPER_SLAB);
    static_cast<blocks::WeatheringCopperSlabBlock*>(CopperBlocks::OXIDIZED_CUT_COPPER_SLAB)
        ->setPreviousOxidationBlock(CopperBlocks::WEATHERED_CUT_COPPER_SLAB);

    // ============================================================================
    // 1.21 铜扩展：铜门（8个）
    // 铜门只能通过红石控制（类似铁门），且可氧化
    // 未涂蜡使用WeatheringCopperDoorBlock，涂蜡使用WaxedCopperDoorBlock
    // ============================================================================
    auto* copperDoor = &registry.registerBlock<blocks::WeatheringCopperDoorBlock>(
        ResourceLocation("minecraft:copper_door"), copperDoorProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperDoor =
        &registry.registerBlock<blocks::WeatheringCopperDoorBlock>(ResourceLocation("minecraft:exposed_copper_door"),
            copperDoorProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperDoor =
        &registry.registerBlock<blocks::WeatheringCopperDoorBlock>(ResourceLocation("minecraft:weathered_copper_door"),
            copperDoorProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperDoor =
        &registry.registerBlock<blocks::WeatheringCopperDoorBlock>(ResourceLocation("minecraft:oxidized_copper_door"),
            copperDoorProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置铜门氧化链
    copperDoor->setNextOxidationBlock(exposedCopperDoor);
    exposedCopperDoor->setNextOxidationBlock(weatheredCopperDoor);
    weatheredCopperDoor->setNextOxidationBlock(oxidizedCopperDoor);

    // 设置铜门反向氧化链（用于斧头刮削）
    exposedCopperDoor->setPreviousOxidationBlock(copperDoor);
    weatheredCopperDoor->setPreviousOxidationBlock(exposedCopperDoor);
    oxidizedCopperDoor->setPreviousOxidationBlock(weatheredCopperDoor);

    CopperBlocks::COPPER_DOOR = copperDoor;
    CopperBlocks::EXPOSED_COPPER_DOOR = exposedCopperDoor;
    CopperBlocks::WEATHERED_COPPER_DOOR = weatheredCopperDoor;
    CopperBlocks::OXIDIZED_COPPER_DOOR = oxidizedCopperDoor;

    // 涂蜡铜门
    CopperBlocks::WAXED_COPPER_DOOR = &registry.registerBlock<blocks::WaxedCopperDoorBlock>(
        ResourceLocation("minecraft:waxed_copper_door"), copperDoorProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_DOOR = &registry.registerBlock<blocks::WaxedCopperDoorBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_door"), copperDoorProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_DOOR = &registry.registerBlock<blocks::WaxedCopperDoorBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_door"), copperDoorProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_DOOR = &registry.registerBlock<blocks::WaxedCopperDoorBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_door"), copperDoorProps);

    // ============================================================================
    // 1.21 铜扩展：铜活板门（8个）
    // 铜活板门只能通过红石控制（类似铁活板门），且可氧化
    // 未涂蜡使用WeatheringCopperTrapDoorBlock，涂蜡使用WaxedCopperTrapDoorBlock
    // ============================================================================
    auto* copperTrapdoor =
        &registry.registerBlock<blocks::WeatheringCopperTrapDoorBlock>(ResourceLocation("minecraft:copper_trapdoor"),
            copperDoorProps,
            BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperTrapdoor = &registry.registerBlock<blocks::WeatheringCopperTrapDoorBlock>(
        ResourceLocation("minecraft:exposed_copper_trapdoor"),
        copperDoorProps,
        BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperTrapdoor = &registry.registerBlock<blocks::WeatheringCopperTrapDoorBlock>(
        ResourceLocation("minecraft:weathered_copper_trapdoor"),
        copperDoorProps,
        BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperTrapdoor = &registry.registerBlock<blocks::WeatheringCopperTrapDoorBlock>(
        ResourceLocation("minecraft:oxidized_copper_trapdoor"),
        copperDoorProps,
        BlockStateProperties::OxidationLevel::Oxidized);

    // 设置铜活板门氧化链
    copperTrapdoor->setNextOxidationBlock(exposedCopperTrapdoor);
    exposedCopperTrapdoor->setNextOxidationBlock(weatheredCopperTrapdoor);
    weatheredCopperTrapdoor->setNextOxidationBlock(oxidizedCopperTrapdoor);

    // 设置铜活板门反向氧化链（用于斧头刮削）
    exposedCopperTrapdoor->setPreviousOxidationBlock(copperTrapdoor);
    weatheredCopperTrapdoor->setPreviousOxidationBlock(exposedCopperTrapdoor);
    oxidizedCopperTrapdoor->setPreviousOxidationBlock(weatheredCopperTrapdoor);

    CopperBlocks::COPPER_TRAPDOOR = copperTrapdoor;
    CopperBlocks::EXPOSED_COPPER_TRAPDOOR = exposedCopperTrapdoor;
    CopperBlocks::WEATHERED_COPPER_TRAPDOOR = weatheredCopperTrapdoor;
    CopperBlocks::OXIDIZED_COPPER_TRAPDOOR = oxidizedCopperTrapdoor;

    // 涂蜡铜活板门
    CopperBlocks::WAXED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::WaxedCopperTrapDoorBlock>(
        ResourceLocation("minecraft:waxed_copper_trapdoor"), copperDoorProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::WaxedCopperTrapDoorBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_trapdoor"), copperDoorProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::WaxedCopperTrapDoorBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_trapdoor"), copperDoorProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::WaxedCopperTrapDoorBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_trapdoor"), copperDoorProps);

    // ============================================================================
    // 1.21 铜扩展：铜格栅（8个）
    // 铜格栅是半透明方块，类似于铁栏杆但更现代，可氧化
    // ============================================================================
    auto* copperGrate = &registry.registerBlock<blocks::WeatheringCopperGrateBlock>(
        ResourceLocation("minecraft:copper_grate"), copperGrateProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperGrate =
        &registry.registerBlock<blocks::WeatheringCopperGrateBlock>(ResourceLocation("minecraft:exposed_copper_grate"),
            copperGrateProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperGrate = &registry.registerBlock<blocks::WeatheringCopperGrateBlock>(
        ResourceLocation("minecraft:weathered_copper_grate"),
        copperGrateProps,
        BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperGrate =
        &registry.registerBlock<blocks::WeatheringCopperGrateBlock>(ResourceLocation("minecraft:oxidized_copper_grate"),
            copperGrateProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链
    copperGrate->setNextOxidationBlock(exposedCopperGrate);
    exposedCopperGrate->setNextOxidationBlock(weatheredCopperGrate);
    weatheredCopperGrate->setNextOxidationBlock(oxidizedCopperGrate);

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperGrate->setPreviousOxidationBlock(copperGrate);
    weatheredCopperGrate->setPreviousOxidationBlock(exposedCopperGrate);
    oxidizedCopperGrate->setPreviousOxidationBlock(weatheredCopperGrate);

    CopperBlocks::COPPER_GRATE = copperGrate;
    CopperBlocks::EXPOSED_COPPER_GRATE = exposedCopperGrate;
    CopperBlocks::WEATHERED_COPPER_GRATE = weatheredCopperGrate;
    CopperBlocks::OXIDIZED_COPPER_GRATE = oxidizedCopperGrate;

    CopperBlocks::WAXED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperGrateBlock>(
        ResourceLocation("minecraft:waxed_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperGrateBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperGrateBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperGrateBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_grate"), copperGrateProps);

    // ============================================================================
    // 1.21 铜扩展：铜灯（8个）
    // 铜灯是红石可控光源，在红石信号上升沿切换LIT状态
    // 未涂蜡的使用CopperBulbBlock（可氧化），涂蜡的使用WaxedCopperBulbBlock
    // ============================================================================
    CopperBlocks::COPPER_BULB = &registry.registerBlock<blocks::CopperBulbBlock>(
        ResourceLocation("minecraft:copper_bulb"), copperBulbProps, BlockStateProperties::OxidationLevel::Unaffected);

    CopperBlocks::EXPOSED_COPPER_BULB =
        &registry.registerBlock<blocks::CopperBulbBlock>(ResourceLocation("minecraft:exposed_copper_bulb"),
            copperBulbProps,
            BlockStateProperties::OxidationLevel::Exposed);

    CopperBlocks::WEATHERED_COPPER_BULB =
        &registry.registerBlock<blocks::CopperBulbBlock>(ResourceLocation("minecraft:weathered_copper_bulb"),
            copperBulbProps,
            BlockStateProperties::OxidationLevel::Weathered);

    CopperBlocks::OXIDIZED_COPPER_BULB =
        &registry.registerBlock<blocks::CopperBulbBlock>(ResourceLocation("minecraft:oxidized_copper_bulb"),
            copperBulbProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置铜灯氧化链
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::COPPER_BULB)
        ->setNextOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::EXPOSED_COPPER_BULB));
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::EXPOSED_COPPER_BULB)
        ->setNextOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::WEATHERED_COPPER_BULB));
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::WEATHERED_COPPER_BULB)
        ->setNextOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::OXIDIZED_COPPER_BULB));

    // 设置铜灯反向氧化链（用于斧头刮削）
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::EXPOSED_COPPER_BULB)
        ->setPreviousOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::COPPER_BULB));
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::WEATHERED_COPPER_BULB)
        ->setPreviousOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::EXPOSED_COPPER_BULB));
    static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::OXIDIZED_COPPER_BULB)
        ->setPreviousOxidationBlock(static_cast<blocks::WeatheringCopperBlock*>(CopperBlocks::WEATHERED_COPPER_BULB));

    CopperBlocks::WAXED_COPPER_BULB = &registry.registerBlock<blocks::WaxedCopperBulbBlock>(
        ResourceLocation("minecraft:waxed_copper_bulb"), copperBulbProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_BULB = &registry.registerBlock<blocks::WaxedCopperBulbBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_bulb"), copperBulbProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_BULB = &registry.registerBlock<blocks::WaxedCopperBulbBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_bulb"), copperBulbProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_BULB = &registry.registerBlock<blocks::WaxedCopperBulbBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_bulb"), copperBulbProps);

    // ============================================================================
    // 1.21 铜扩展：凿制铜（8个）
    // ============================================================================
    auto* chiseledCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:chiseled_copper"),
            chiseledCopperProps,
            BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedChiseledCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:exposed_chiseled_copper"),
            chiseledCopperProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredChiseledCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:weathered_chiseled_copper"),
            chiseledCopperProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedChiseledCopper =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:oxidized_chiseled_copper"),
            chiseledCopperProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链
    chiseledCopper->setNextOxidationBlock(exposedChiseledCopper);
    exposedChiseledCopper->setNextOxidationBlock(weatheredChiseledCopper);
    weatheredChiseledCopper->setNextOxidationBlock(oxidizedChiseledCopper);

    // 设置反向氧化链（用于斧头刮削）
    exposedChiseledCopper->setPreviousOxidationBlock(chiseledCopper);
    weatheredChiseledCopper->setPreviousOxidationBlock(exposedChiseledCopper);
    oxidizedChiseledCopper->setPreviousOxidationBlock(weatheredChiseledCopper);

    CopperBlocks::CHISELED_COPPER = chiseledCopper;
    CopperBlocks::EXPOSED_CHISELED_COPPER = exposedChiseledCopper;
    CopperBlocks::WEATHERED_CHISELED_COPPER = weatheredChiseledCopper;
    CopperBlocks::OXIDIZED_CHISELED_COPPER = oxidizedChiseledCopper;

    CopperBlocks::WAXED_CHISELED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_chiseled_copper"), chiseledCopperProps);

    CopperBlocks::WAXED_EXPOSED_CHISELED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_exposed_chiseled_copper"), chiseledCopperProps);

    CopperBlocks::WAXED_WEATHERED_CHISELED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_weathered_chiseled_copper"), chiseledCopperProps);

    CopperBlocks::WAXED_OXIDIZED_CHISELED_COPPER = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_oxidized_chiseled_copper"), chiseledCopperProps);

    // ============================================================================
    // 1.21 铜扩展：铜栏杆（8个）
    // ============================================================================
    auto copperBarsProps =
        BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).notSolid().soundType(BlockSoundTypes::COPPER);

    auto* copperBars = &registry.registerBlock<blocks::WeatheringCopperBarsBlock>(
        ResourceLocation("minecraft:copper_bars"), copperBarsProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperBars =
        &registry.registerBlock<blocks::WeatheringCopperBarsBlock>(ResourceLocation("minecraft:exposed_copper_bars"),
            copperBarsProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperBars =
        &registry.registerBlock<blocks::WeatheringCopperBarsBlock>(ResourceLocation("minecraft:weathered_copper_bars"),
            copperBarsProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperBars =
        &registry.registerBlock<blocks::WeatheringCopperBarsBlock>(ResourceLocation("minecraft:oxidized_copper_bars"),
            copperBarsProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    copperBars->setNextOxidationBlock(exposedCopperBars);
    exposedCopperBars->setNextOxidationBlock(weatheredCopperBars);
    weatheredCopperBars->setNextOxidationBlock(oxidizedCopperBars);

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperBars->setPreviousOxidationBlock(copperBars);
    weatheredCopperBars->setPreviousOxidationBlock(exposedCopperBars);
    oxidizedCopperBars->setPreviousOxidationBlock(weatheredCopperBars);

    CopperBlocks::COPPER_BARS = copperBars;
    CopperBlocks::EXPOSED_COPPER_BARS = exposedCopperBars;
    CopperBlocks::WEATHERED_COPPER_BARS = weatheredCopperBars;
    CopperBlocks::OXIDIZED_COPPER_BARS = oxidizedCopperBars;

    CopperBlocks::WAXED_COPPER_BARS = &registry.registerBlock<blocks::WaxedCopperBarsBlock>(
        ResourceLocation("minecraft:waxed_copper_bars"), copperBarsProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_BARS = &registry.registerBlock<blocks::WaxedCopperBarsBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_bars"), copperBarsProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_BARS = &registry.registerBlock<blocks::WaxedCopperBarsBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_bars"), copperBarsProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_BARS = &registry.registerBlock<blocks::WaxedCopperBarsBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_bars"), copperBarsProps);

    // ============================================================================
    // 1.21 铜扩展：铜链（8个）
    // ============================================================================
    auto copperChainProps =
        BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).notSolid().soundType(BlockSoundTypes::CHAIN);

    auto* copperChain = &registry.registerBlock<blocks::WeatheringCopperChainBlock>(
        ResourceLocation("minecraft:copper_chain"), copperChainProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperChain =
        &registry.registerBlock<blocks::WeatheringCopperChainBlock>(ResourceLocation("minecraft:exposed_copper_chain"),
            copperChainProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperChain = &registry.registerBlock<blocks::WeatheringCopperChainBlock>(
        ResourceLocation("minecraft:weathered_copper_chain"),
        copperChainProps,
        BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperChain =
        &registry.registerBlock<blocks::WeatheringCopperChainBlock>(ResourceLocation("minecraft:oxidized_copper_chain"),
            copperChainProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    copperChain->setNextOxidationBlock(exposedCopperChain);
    exposedCopperChain->setNextOxidationBlock(weatheredCopperChain);
    weatheredCopperChain->setNextOxidationBlock(oxidizedCopperChain);

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperChain->setPreviousOxidationBlock(copperChain);
    weatheredCopperChain->setPreviousOxidationBlock(exposedCopperChain);
    oxidizedCopperChain->setPreviousOxidationBlock(weatheredCopperChain);

    CopperBlocks::COPPER_CHAIN = copperChain;
    CopperBlocks::EXPOSED_COPPER_CHAIN = exposedCopperChain;
    CopperBlocks::WEATHERED_COPPER_CHAIN = weatheredCopperChain;
    CopperBlocks::OXIDIZED_COPPER_CHAIN = oxidizedCopperChain;

    CopperBlocks::WAXED_COPPER_CHAIN = &registry.registerBlock<blocks::WaxedCopperChainBlock>(
        ResourceLocation("minecraft:waxed_copper_chain"), copperChainProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_CHAIN = &registry.registerBlock<blocks::WaxedCopperChainBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_chain"), copperChainProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_CHAIN = &registry.registerBlock<blocks::WaxedCopperChainBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_chain"), copperChainProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_CHAIN = &registry.registerBlock<blocks::WaxedCopperChainBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_chain"), copperChainProps);

    // ============================================================================
    // 1.21 铜扩展：铜灯笼（8个）
    // ============================================================================
    auto copperLanternProps =
        BlockProperties(Material::IRON).hardness(3.5f).resistance(3.5f).notSolid().soundType(BlockSoundTypes::LANTERN);

    auto* copperLantern =
        &registry.registerBlock<blocks::WeatheringCopperLanternBlock>(ResourceLocation("minecraft:copper_lantern"),
            copperLanternProps,
            BlockStateProperties::OxidationLevel::Unaffected,
            15);
    auto* exposedCopperLantern = &registry.registerBlock<blocks::WeatheringCopperLanternBlock>(
        ResourceLocation("minecraft:exposed_copper_lantern"),
        copperLanternProps,
        BlockStateProperties::OxidationLevel::Exposed,
        15);
    auto* weatheredCopperLantern = &registry.registerBlock<blocks::WeatheringCopperLanternBlock>(
        ResourceLocation("minecraft:weathered_copper_lantern"),
        copperLanternProps,
        BlockStateProperties::OxidationLevel::Weathered,
        15);
    auto* oxidizedCopperLantern = &registry.registerBlock<blocks::WeatheringCopperLanternBlock>(
        ResourceLocation("minecraft:oxidized_copper_lantern"),
        copperLanternProps,
        BlockStateProperties::OxidationLevel::Oxidized,
        15);

    copperLantern->setNextOxidationBlock(exposedCopperLantern);
    exposedCopperLantern->setNextOxidationBlock(weatheredCopperLantern);
    weatheredCopperLantern->setNextOxidationBlock(oxidizedCopperLantern);

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperLantern->setPreviousOxidationBlock(copperLantern);
    weatheredCopperLantern->setPreviousOxidationBlock(exposedCopperLantern);
    oxidizedCopperLantern->setPreviousOxidationBlock(weatheredCopperLantern);

    CopperBlocks::COPPER_LANTERN = copperLantern;
    CopperBlocks::EXPOSED_COPPER_LANTERN = exposedCopperLantern;
    CopperBlocks::WEATHERED_COPPER_LANTERN = weatheredCopperLantern;
    CopperBlocks::OXIDIZED_COPPER_LANTERN = oxidizedCopperLantern;

    CopperBlocks::WAXED_COPPER_LANTERN = &registry.registerBlock<blocks::WaxedCopperLanternBlock>(
        ResourceLocation("minecraft:waxed_copper_lantern"), copperLanternProps, 15);

    CopperBlocks::WAXED_EXPOSED_COPPER_LANTERN = &registry.registerBlock<blocks::WaxedCopperLanternBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_lantern"), copperLanternProps, 15);

    CopperBlocks::WAXED_WEATHERED_COPPER_LANTERN = &registry.registerBlock<blocks::WaxedCopperLanternBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_lantern"), copperLanternProps, 15);

    CopperBlocks::WAXED_OXIDIZED_COPPER_LANTERN = &registry.registerBlock<blocks::WaxedCopperLanternBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_lantern"), copperLanternProps, 15);

    // ============================================================================
    // 避雷针（1.17 基础 + 1.21 铜扩展氧化变种）
    // MC 1.21+ 避雷针新增氧化变种：4个可氧化 + 4个涂蜡 = 8个变种
    // 基础 lightning_rod 仍为 LightningRodBlock（不参与氧化链），
    // exposed/weathered/oxidized 为 WeatheringLightningRodBlock（可氧化），
    // waxed 系列为 WaxedLightningRodBlock（不氧化）。
    // 氧化链：lightning_rod -> exposed_lightning_rod -> weathered_lightning_rod -> oxidized_lightning_rod
    // ============================================================================

    // 避雷针基础属性: Material::IRON, 硬度3.0, 抗爆6.0, notSolid, 声音类型COPPER
    BlockProperties lightningRodProps = BlockProperties(Material::IRON)
                                            .hardness(3.0f)
                                            .resistance(6.0f)
                                            .harvestTool(HarvestTool::Pickaxe)
                                            .requiresTool()
                                            .notSolid()
                                            .soundType(BlockSoundTypes::COPPER);

    // 基础避雷针（未氧化，不参与氧化链 - MC 原版 LIGHTNING_ROD 是普通 LightningRodBlock）
    CopperBlocks::LIGHTNING_ROD = &registry.registerBlock<blocks::LightningRodBlock>(
        ResourceLocation("minecraft:lightning_rod"), lightningRodProps);

    // 可氧化避雷针变种（Exposed, Weathered, Oxidized）
    auto* exposedLightningRod = &registry.registerBlock<blocks::WeatheringLightningRodBlock>(
        ResourceLocation("minecraft:exposed_lightning_rod"),
        lightningRodProps,
        BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredLightningRod = &registry.registerBlock<blocks::WeatheringLightningRodBlock>(
        ResourceLocation("minecraft:weathered_lightning_rod"),
        lightningRodProps,
        BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedLightningRod = &registry.registerBlock<blocks::WeatheringLightningRodBlock>(
        ResourceLocation("minecraft:oxidized_lightning_rod"),
        lightningRodProps,
        BlockStateProperties::OxidationLevel::Oxidized);

    // 设置避雷针氧化链: lightning_rod -> exposed_lightning_rod -> weathered_lightning_rod -> oxidized_lightning_rod
    // 注意：MC 原版中基础 lightning_rod 虽然不实现 WeatheringCopper，
    // 但它仍然处于氧化链的 Unaffected 位置，即 exposed 的前驱是 lightning_rod。
    // 本项目中 lightning_rod 是普通 LightningRodBlock（不实现 IOxidizableBlock），
    // 所以它没有 setNextOxidationBlock 方法。氧化链从 exposed 开始：
    exposedLightningRod->setNextOxidationBlock(weatheredLightningRod);
    weatheredLightningRod->setNextOxidationBlock(oxidizedLightningRod);
    // oxidized 的 m_nextOxidationBlock 保持 nullptr

    // 设置避雷针反向氧化链（用于斧头刮削）
    exposedLightningRod->setPreviousOxidationBlock(CopperBlocks::LIGHTNING_ROD);
    weatheredLightningRod->setPreviousOxidationBlock(exposedLightningRod);
    oxidizedLightningRod->setPreviousOxidationBlock(weatheredLightningRod);

    CopperBlocks::EXPOSED_LIGHTNING_ROD = exposedLightningRod;
    CopperBlocks::WEATHERED_LIGHTNING_ROD = weatheredLightningRod;
    CopperBlocks::OXIDIZED_LIGHTNING_ROD = oxidizedLightningRod;

    // 涂蜡避雷针变种
    CopperBlocks::WAXED_LIGHTNING_ROD = &registry.registerBlock<blocks::WaxedLightningRodBlock>(
        ResourceLocation("minecraft:waxed_lightning_rod"), lightningRodProps);

    CopperBlocks::WAXED_EXPOSED_LIGHTNING_ROD = &registry.registerBlock<blocks::WaxedLightningRodBlock>(
        ResourceLocation("minecraft:waxed_exposed_lightning_rod"), lightningRodProps);

    CopperBlocks::WAXED_WEATHERED_LIGHTNING_ROD = &registry.registerBlock<blocks::WaxedLightningRodBlock>(
        ResourceLocation("minecraft:waxed_weathered_lightning_rod"), lightningRodProps);

    CopperBlocks::WAXED_OXIDIZED_LIGHTNING_ROD = &registry.registerBlock<blocks::WaxedLightningRodBlock>(
        ResourceLocation("minecraft:waxed_oxidized_lightning_rod"), lightningRodProps);

    // ============================================================================
    // 粗矿块（1.17）
    // 粗铁矿: 硬度5.0, 抗爆6.0, 镐, harvestLevel 1 (石镐)
    // 粗铜矿: 硬度5.0, 抗爆6.0, 镐, harvestLevel 1 (石镐)
    // 粗金矿: 硬度5.0, 抗爆6.0, 镐, harvestLevel 2 (铁镐)
    // ============================================================================
    CopperBlocks::RAW_IRON_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:raw_iron_block"),
        BlockProperties(Material::IRON)
            .hardness(5.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(1)
            .requiresTool());

    CopperBlocks::RAW_COPPER_BLOCK =
        &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:raw_copper_block"),
            BlockProperties(Material::IRON)
                .hardness(5.0f)
                .resistance(6.0f)
                .harvestTool(HarvestTool::Pickaxe)
                .harvestLevel(1)
                .requiresTool());

    CopperBlocks::RAW_GOLD_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:raw_gold_block"),
        BlockProperties(Material::IRON)
            .hardness(5.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(2)
            .requiresTool());

    // ============================================================================
    // 1.21.11 铜傀儡雕像（8个）
    // 铜傀儡雕像是一种装饰性方块，具有 4 种姿态（Standing/Sitting/Running/Star），
    // 玩家右键点击循环切换姿态，红石比较器输出 POSE.ordinal()+1 (1-4)。
    // 拥有方块实体（CopperGolemStatueBlockEntity）用于保存 CUSTOM_NAME 组件。
    //
    // 类层次结构（与 MC Java 一致）：
    // - CopperGolemStatueBlock：基础类（Unaffected 等级 + 涂蜡变种）
    //   不实现 IOxidizableBlock，不参与随机 tick
    // - WeatheringCopperGolemStatueBlock：继承 CopperGolemStatueBlock + IOxidizableBlock
    //   用于 Exposed/Weathered/Oxidized 等级，会在随机 tick 中尝试氧化
    //
    // 氧化链：copper_golem_statue -> exposed_copper_golem_statue ->
    //         weathered_copper_golem_statue -> oxidized_copper_golem_statue
    // ============================================================================
    BlockProperties copperGolemStatueProps = BlockProperties(Material::IRON)
                                                 .hardness(3.0f)
                                                 .resistance(6.0f)
                                                 .harvestTool(HarvestTool::Pickaxe)
                                                 .requiresTool()
                                                 .soundType(BlockSoundTypes::COPPER);

    // 基础铜傀儡雕像（Unaffected 等级，不氧化但处于氧化链最低位）
    auto* copperGolemStatue = &registry.registerBlock<blocks::CopperGolemStatueBlock>(
        ResourceLocation("minecraft:copper_golem_statue"), copperGolemStatueProps);

    // 可氧化铜傀儡雕像变种（Exposed/Weathered/Oxidized）
    auto* exposedCopperGolemStatue = &registry.registerBlock<blocks::WeatheringCopperGolemStatueBlock>(
        ResourceLocation("minecraft:exposed_copper_golem_statue"),
        copperGolemStatueProps,
        BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperGolemStatue = &registry.registerBlock<blocks::WeatheringCopperGolemStatueBlock>(
        ResourceLocation("minecraft:weathered_copper_golem_statue"),
        copperGolemStatueProps,
        BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperGolemStatue = &registry.registerBlock<blocks::WeatheringCopperGolemStatueBlock>(
        ResourceLocation("minecraft:oxidized_copper_golem_statue"),
        copperGolemStatueProps,
        BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链：copper_golem_statue -> exposed -> weathered -> oxidized
    // 注意：基础 copper_golem_statue 是 CopperGolemStatueBlock（不实现 IOxidizableBlock），
    // 所以它没有 setNextOxidationBlock 方法。氧化链从 exposed 开始：
    exposedCopperGolemStatue->setNextOxidationBlock(weatheredCopperGolemStatue);
    weatheredCopperGolemStatue->setNextOxidationBlock(oxidizedCopperGolemStatue);
    // oxidized 的 m_nextOxidationBlock 保持 nullptr（最高等级）

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperGolemStatue->setPreviousOxidationBlock(copperGolemStatue);
    weatheredCopperGolemStatue->setPreviousOxidationBlock(exposedCopperGolemStatue);
    oxidizedCopperGolemStatue->setPreviousOxidationBlock(weatheredCopperGolemStatue);
    // copper_golem_statue 的 m_previousOxidationBlock 保持 nullptr（最低等级）

    CopperBlocks::COPPER_GOLEM_STATUE = copperGolemStatue;
    CopperBlocks::EXPOSED_COPPER_GOLEM_STATUE = exposedCopperGolemStatue;
    CopperBlocks::WEATHERED_COPPER_GOLEM_STATUE = weatheredCopperGolemStatue;
    CopperBlocks::OXIDIZED_COPPER_GOLEM_STATUE = oxidizedCopperGolemStatue;

    // 涂蜡铜傀儡雕像变种（不氧化）
    CopperBlocks::WAXED_COPPER_GOLEM_STATUE = &registry.registerBlock<blocks::CopperGolemStatueBlock>(
        ResourceLocation("minecraft:waxed_copper_golem_statue"), copperGolemStatueProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE = &registry.registerBlock<blocks::CopperGolemStatueBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_golem_statue"), copperGolemStatueProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE = &registry.registerBlock<blocks::CopperGolemStatueBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_golem_statue"), copperGolemStatueProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE = &registry.registerBlock<blocks::CopperGolemStatueBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_golem_statue"), copperGolemStatueProps);

    // ============================================================================
    // 1.21.11 铜箱子（8个）
    // 铜箱子是容器方块，27 格容量，支持双箱合并（54 格）。拥有方块实体（复用
    // BlockEntityType::Chest 与 ChestEntity）。
    //
    // 类层次结构（与 MC Java 1.21.11 一致）：
    // - CopperChestBlock：基础类（Unaffected 等级 + 涂蜡变种），不实现 IOxidizableBlock
    // - WeatheringCopperChestBlock：继承 CopperChestBlock + IOxidizableBlock
    //   用于 Exposed/Weathered/Oxidized 等级
    // - WaxedCopperChestBlock：继承 CopperChestBlock，重写 isWaxed() 返回 true
    //
    // 氧化链：copper_chest -> exposed_copper_chest ->
    //         weathered_copper_chest -> oxidized_copper_chest
    //
    // 与铜傀儡雕像不同：铜箱子不使用 OXIDATION 方块状态属性，每个氧化等级是独立的方块类型
    // （与 MC Java 1.21.11 一致）。m_oxidationLevel 成员变量仅用于双箱合并时比较氧化等级。
    //
    // 特殊行为：
    // - 双箱合并允许跨氧化等级与涂蜡状态（chestCanConnectTo 检查 COPPER_CHESTS 标签）
    // - 氧化/涂蜡/除蜡/刮削时保留方块实体（shouldChangedStateKeepBlockEntity 返回 true）
    // - 随机 tick 氧化时跳过 RIGHT 部分和正在被打开的箱子
    // ============================================================================

    // 铜箱子基础属性：与普通箱子一致（WOOD 材质，硬度 2.5，可燃），但使用铜的声音类型
    BlockProperties copperChestProps =
        BlockProperties(Material::WOOD).hardness(2.5f).resistance(2.5f).notSolid().soundType(BlockSoundTypes::COPPER);

    // 开合音效按氧化等级映射（与 MC Java 1.21.11 一致）：
    // - Unaffected/Exposed + 涂蜡变体 -> block.copper_chest.open/close
    // - Weathered + 涂蜡变体 -> block.copper_chest_weathered.open/close
    // - Oxidized + 涂蜡变体 -> block.copper_chest_oxidized.open/close
    // 涂蜡变体复用对应氧化等级的声音（在构造时传入相同的声音引用）。

    // 基础铜箱子（Unaffected 等级，不氧化但处于氧化链最低位）
    auto* copperChest = &registry.registerBlock<blocks::CopperChestBlock>(ResourceLocation("minecraft:copper_chest"),
        copperChestProps,
        BlockStateProperties::OxidationLevel::Unaffected,
        SoundEvents::BLOCK_COPPER_CHEST_OPEN,
        SoundEvents::BLOCK_COPPER_CHEST_CLOSE);

    // 可氧化铜箱子变种（Exposed/Weathered/Oxidized）
    // Exposed 复用 Unaffected 的 block.copper_chest.open/close 声音（与 MC Java 一致）
    auto* exposedCopperChest =
        &registry.registerBlock<blocks::WeatheringCopperChestBlock>(ResourceLocation("minecraft:exposed_copper_chest"),
            copperChestProps,
            BlockStateProperties::OxidationLevel::Exposed,
            SoundEvents::BLOCK_COPPER_CHEST_OPEN,
            SoundEvents::BLOCK_COPPER_CHEST_CLOSE);
    auto* weatheredCopperChest = &registry.registerBlock<blocks::WeatheringCopperChestBlock>(
        ResourceLocation("minecraft:weathered_copper_chest"),
        copperChestProps,
        BlockStateProperties::OxidationLevel::Weathered,
        SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_OPEN,
        SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_CLOSE);
    auto* oxidizedCopperChest =
        &registry.registerBlock<blocks::WeatheringCopperChestBlock>(ResourceLocation("minecraft:oxidized_copper_chest"),
            copperChestProps,
            BlockStateProperties::OxidationLevel::Oxidized,
            SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_OPEN,
            SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_CLOSE);

    // 设置氧化链：copper_chest -> exposed_copper_chest -> weathered_copper_chest -> oxidized_copper_chest
    // 注意：基础 copper_chest 是 CopperChestBlock（不实现 IOxidizableBlock），
    // 所以它没有 setNextOxidationBlock 方法。氧化链从 exposed 开始：
    exposedCopperChest->setNextOxidationBlock(weatheredCopperChest);
    weatheredCopperChest->setNextOxidationBlock(oxidizedCopperChest);
    // oxidized 的 m_nextOxidationBlock 保持 nullptr（最高等级）

    // 设置反向氧化链（用于斧头刮削）
    exposedCopperChest->setPreviousOxidationBlock(copperChest);
    weatheredCopperChest->setPreviousOxidationBlock(exposedCopperChest);
    oxidizedCopperChest->setPreviousOxidationBlock(weatheredCopperChest);
    // copper_chest 的 m_previousOxidationBlock 保持 nullptr（最低等级）

    CopperBlocks::COPPER_CHEST = copperChest;
    CopperBlocks::EXPOSED_COPPER_CHEST = exposedCopperChest;
    CopperBlocks::WEATHERED_COPPER_CHEST = weatheredCopperChest;
    CopperBlocks::OXIDIZED_COPPER_CHEST = oxidizedCopperChest;

    // 涂蜡铜箱子变种（不氧化，使用 WaxedCopperChestBlock）
    // 涂蜡变体复用对应氧化等级的声音事件（与 MC Java 一致）
    CopperBlocks::WAXED_COPPER_CHEST =
        &registry.registerBlock<blocks::WaxedCopperChestBlock>(ResourceLocation("minecraft:waxed_copper_chest"),
            copperChestProps,
            BlockStateProperties::OxidationLevel::Unaffected,
            SoundEvents::BLOCK_COPPER_CHEST_OPEN,
            SoundEvents::BLOCK_COPPER_CHEST_CLOSE);

    CopperBlocks::WAXED_EXPOSED_COPPER_CHEST =
        &registry.registerBlock<blocks::WaxedCopperChestBlock>(ResourceLocation("minecraft:waxed_exposed_copper_chest"),
            copperChestProps,
            BlockStateProperties::OxidationLevel::Exposed,
            SoundEvents::BLOCK_COPPER_CHEST_OPEN,
            SoundEvents::BLOCK_COPPER_CHEST_CLOSE);

    CopperBlocks::WAXED_WEATHERED_COPPER_CHEST = &registry.registerBlock<blocks::WaxedCopperChestBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_chest"),
        copperChestProps,
        BlockStateProperties::OxidationLevel::Weathered,
        SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_OPEN,
        SoundEvents::BLOCK_COPPER_CHEST_WEATHERED_CLOSE);

    CopperBlocks::WAXED_OXIDIZED_COPPER_CHEST = &registry.registerBlock<blocks::WaxedCopperChestBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_chest"),
        copperChestProps,
        BlockStateProperties::OxidationLevel::Oxidized,
        SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_OPEN,
        SoundEvents::BLOCK_COPPER_CHEST_OXIDIZED_CLOSE);
}

} // namespace block_registry
} // namespace mc
