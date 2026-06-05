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
#include "world/block/BlockRegistry.hpp"
#include "world/block/BlockSoundType.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/Material.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/copper/CopperBulbBlock.hpp"
#include "world/block/blocks/copper/WeatheringCopperBlock.hpp"

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
// 避雷针
// ============================================================================
Block* CopperBlocks::LIGHTNING_ROD = nullptr;

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
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:cut_copper_stairs"),
            CopperBlocks::CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::EXPOSED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:exposed_cut_copper_stairs"),
            CopperBlocks::EXPOSED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WEATHERED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:weathered_cut_copper_stairs"),
            CopperBlocks::WEATHERED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::OXIDIZED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:oxidized_cut_copper_stairs"),
            CopperBlocks::OXIDIZED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WAXED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:waxed_cut_copper_stairs"),
            CopperBlocks::WAXED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:waxed_exposed_cut_copper_stairs"),
            CopperBlocks::WAXED_EXPOSED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:waxed_weathered_cut_copper_stairs"),
            CopperBlocks::WAXED_WEATHERED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:waxed_oxidized_cut_copper_stairs"),
            CopperBlocks::WAXED_OXIDIZED_CUT_COPPER->defaultState(),
            copperStairSlabProps);

    // ============================================================================
    // 切制铜台阶（8个）
    // ============================================================================
    CopperBlocks::CUT_COPPER_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::EXPOSED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:exposed_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WEATHERED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:weathered_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::OXIDIZED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:oxidized_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:waxed_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:waxed_exposed_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:waxed_weathered_cut_copper_slab"), copperStairSlabProps);

    CopperBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB = &registry.registerBlock<blocks::SlabBlock>(
        ResourceLocation("minecraft:waxed_oxidized_cut_copper_slab"), copperStairSlabProps);

    // ============================================================================
    // 1.21 铜扩展：铜门（8个）
    // 铜门只能通过红石控制（类似铁门），所以第二个参数传 true
    // ============================================================================
    CopperBlocks::COPPER_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:copper_door"), copperDoorProps, true);

    CopperBlocks::EXPOSED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:exposed_copper_door"), copperDoorProps, true);

    CopperBlocks::WEATHERED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:weathered_copper_door"), copperDoorProps, true);

    CopperBlocks::OXIDIZED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:oxidized_copper_door"), copperDoorProps, true);

    CopperBlocks::WAXED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:waxed_copper_door"), copperDoorProps, true);

    CopperBlocks::WAXED_EXPOSED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_door"), copperDoorProps, true);

    CopperBlocks::WAXED_WEATHERED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_door"), copperDoorProps, true);

    CopperBlocks::WAXED_OXIDIZED_COPPER_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_door"), copperDoorProps, true);

    // ============================================================================
    // 1.21 铜扩展：铜活板门（8个）
    // 铜活板门只能通过红石控制（类似铁活板门），所以第二个参数传 true
    // ============================================================================
    CopperBlocks::COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::EXPOSED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:exposed_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::WEATHERED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:weathered_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::OXIDIZED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:oxidized_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::WAXED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:waxed_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_trapdoor"), copperDoorProps, true);

    CopperBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:waxed_oxidized_copper_trapdoor"), copperDoorProps, true);

    // ============================================================================
    // 1.21 铜扩展：铜格栅（8个）
    // 铜格栅是半透明方块，类似于铁栏杆但更现代，可氧化
    // ============================================================================
    auto* copperGrate = &registry.registerBlock<blocks::WeatheringCopperBlock>(
        ResourceLocation("minecraft:copper_grate"), copperGrateProps, BlockStateProperties::OxidationLevel::Unaffected);
    auto* exposedCopperGrate =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:exposed_copper_grate"),
            copperGrateProps,
            BlockStateProperties::OxidationLevel::Exposed);
    auto* weatheredCopperGrate =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:weathered_copper_grate"),
            copperGrateProps,
            BlockStateProperties::OxidationLevel::Weathered);
    auto* oxidizedCopperGrate =
        &registry.registerBlock<blocks::WeatheringCopperBlock>(ResourceLocation("minecraft:oxidized_copper_grate"),
            copperGrateProps,
            BlockStateProperties::OxidationLevel::Oxidized);

    // 设置氧化链
    copperGrate->setNextOxidationBlock(exposedCopperGrate);
    exposedCopperGrate->setNextOxidationBlock(weatheredCopperGrate);
    weatheredCopperGrate->setNextOxidationBlock(oxidizedCopperGrate);

    CopperBlocks::COPPER_GRATE = copperGrate;
    CopperBlocks::EXPOSED_COPPER_GRATE = exposedCopperGrate;
    CopperBlocks::WEATHERED_COPPER_GRATE = weatheredCopperGrate;
    CopperBlocks::OXIDIZED_COPPER_GRATE = oxidizedCopperGrate;

    CopperBlocks::WAXED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_EXPOSED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_exposed_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_WEATHERED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperBlock>(
        ResourceLocation("minecraft:waxed_weathered_copper_grate"), copperGrateProps);

    CopperBlocks::WAXED_OXIDIZED_COPPER_GRATE = &registry.registerBlock<blocks::WaxedCopperBlock>(
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
    // 避雷针（1.17）
    // 避雷针是方向性方块，后续需要添加 FACING 属性和 WATERLOGGED 属性
    // 目前先用 SimpleBlock 占位
    // ============================================================================
    CopperBlocks::LIGHTNING_ROD = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:lightning_rod"),
        BlockProperties(Material::IRON)
            .hardness(3.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .requiresTool()
            .notSolid()
            .soundType(BlockSoundTypes::COPPER));

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
}

} // namespace block_registry
} // namespace mc
