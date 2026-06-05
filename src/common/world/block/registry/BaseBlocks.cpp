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

#include "world/block/registry/BaseBlocks.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/AirBlock.hpp"
#include "world/block/blocks/FallingBlock.hpp"
#include "world/block/blocks/RotatedPillarBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/dirt/SpreadableSnowyDirtBlock.hpp"
#include "world/block/blocks/ice/IceBlock.hpp"
#include "world/block/blocks/ice/SnowBlock.hpp"
#include "world/block/fluid/FluidRegistry.hpp"
#include "world/block/fluid/FluidTags.hpp"
#include "world/block/fluid/fluids/LavaFluid.hpp"
#include "world/block/fluid/fluids/WaterFluid.hpp"

namespace mc {
namespace block_registry {

// 基础方块
Block* BaseBlocks::AIR = nullptr;
Block* BaseBlocks::CAVE_AIR = nullptr;
Block* BaseBlocks::VOID_AIR = nullptr;
Block* BaseBlocks::STONE = nullptr;
Block* BaseBlocks::GRASS_BLOCK = nullptr;
Block* BaseBlocks::DIRT = nullptr;
Block* BaseBlocks::COBBLESTONE = nullptr;
Block* BaseBlocks::OAK_PLANKS = nullptr;
Block* BaseBlocks::WATER = nullptr;
Block* BaseBlocks::LAVA = nullptr;
Block* BaseBlocks::BEDROCK = nullptr;
Block* BaseBlocks::SAND = nullptr;
Block* BaseBlocks::GRAVEL = nullptr;

// 石头变种
Block* BaseBlocks::GRANITE = nullptr;
Block* BaseBlocks::POLISHED_GRANITE = nullptr;
Block* BaseBlocks::DIORITE = nullptr;
Block* BaseBlocks::POLISHED_DIORITE = nullptr;
Block* BaseBlocks::ANDESITE = nullptr;
Block* BaseBlocks::POLISHED_ANDESITE = nullptr;

// 泥土变种
Block* BaseBlocks::COARSE_DIRT = nullptr;
Block* BaseBlocks::PODZOL = nullptr;

// 砂岩系列
Block* BaseBlocks::SANDSTONE = nullptr;
Block* BaseBlocks::CHISELED_SANDSTONE = nullptr;
Block* BaseBlocks::CUT_SANDSTONE = nullptr;
Block* BaseBlocks::RED_SANDSTONE = nullptr;

// 矿石方块
Block* BaseBlocks::GOLD_ORE = nullptr;
Block* BaseBlocks::IRON_ORE = nullptr;
Block* BaseBlocks::COAL_ORE = nullptr;
Block* BaseBlocks::DIAMOND_ORE = nullptr;
Block* BaseBlocks::DIAMOND_BLOCK = nullptr;
Block* BaseBlocks::EMERALD_ORE = nullptr;
Block* BaseBlocks::LAPIS_ORE = nullptr;
Block* BaseBlocks::REDSTONE_ORE = nullptr;
Block* BaseBlocks::COPPER_ORE = nullptr;

// 下界矿石
Block* BaseBlocks::NETHER_QUARTZ_ORE = nullptr;
Block* BaseBlocks::NETHER_GOLD_ORE = nullptr;
Block* BaseBlocks::ANCIENT_DEBRIS = nullptr;

// 矿物方块
Block* BaseBlocks::COAL_BLOCK = nullptr;
Block* BaseBlocks::GOLD_BLOCK = nullptr;
Block* BaseBlocks::IRON_BLOCK = nullptr;
Block* BaseBlocks::LAPIS_BLOCK = nullptr;
Block* BaseBlocks::EMERALD_BLOCK = nullptr;
Block* BaseBlocks::REDSTONE_BLOCK = nullptr;
Block* BaseBlocks::NETHERITE_BLOCK = nullptr;

// 原木和树叶
Block* BaseBlocks::OAK_LOG = nullptr;
Block* BaseBlocks::OAK_WOOD = nullptr;
Block* BaseBlocks::OAK_LEAVES = nullptr;
Block* BaseBlocks::SPRUCE_LOG = nullptr;
Block* BaseBlocks::SPRUCE_WOOD = nullptr;
Block* BaseBlocks::BIRCH_LOG = nullptr;
Block* BaseBlocks::BIRCH_WOOD = nullptr;
Block* BaseBlocks::JUNGLE_LOG = nullptr;
Block* BaseBlocks::JUNGLE_WOOD = nullptr;
Block* BaseBlocks::ACACIA_LOG = nullptr;
Block* BaseBlocks::ACACIA_WOOD = nullptr;
Block* BaseBlocks::DARK_OAK_LOG = nullptr;
Block* BaseBlocks::DARK_OAK_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_OAK_LOG = nullptr;
Block* BaseBlocks::STRIPPED_SPRUCE_LOG = nullptr;
Block* BaseBlocks::STRIPPED_BIRCH_LOG = nullptr;
Block* BaseBlocks::STRIPPED_JUNGLE_LOG = nullptr;
Block* BaseBlocks::STRIPPED_ACACIA_LOG = nullptr;
Block* BaseBlocks::STRIPPED_DARK_OAK_LOG = nullptr;
Block* BaseBlocks::STRIPPED_OAK_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_SPRUCE_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_BIRCH_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_JUNGLE_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_ACACIA_WOOD = nullptr;
Block* BaseBlocks::STRIPPED_DARK_OAK_WOOD = nullptr;
Block* BaseBlocks::SPRUCE_LEAVES = nullptr;
Block* BaseBlocks::BIRCH_LEAVES = nullptr;
Block* BaseBlocks::JUNGLE_LEAVES = nullptr;
Block* BaseBlocks::ACACIA_LEAVES = nullptr;
Block* BaseBlocks::DARK_OAK_LEAVES = nullptr;

// 木板变种
Block* BaseBlocks::SPRUCE_PLANKS = nullptr;
Block* BaseBlocks::BIRCH_PLANKS = nullptr;
Block* BaseBlocks::JUNGLE_PLANKS = nullptr;
Block* BaseBlocks::ACACIA_PLANKS = nullptr;
Block* BaseBlocks::DARK_OAK_PLANKS = nullptr;

// 其他基础方块
Block* BaseBlocks::SNOW = nullptr;
Block* BaseBlocks::SNOW_BLOCK = nullptr;
Block* BaseBlocks::ICE = nullptr;
Block* BaseBlocks::GLASS = nullptr;
Block* BaseBlocks::NETHERRACK = nullptr;
Block* BaseBlocks::GLOWSTONE = nullptr;
Block* BaseBlocks::END_STONE = nullptr;
Block* BaseBlocks::OBSIDIAN = nullptr;

void registerBaseBlocks()
{
    auto& registry = BlockRegistry::instance();

    // 首先初始化流体注册表（确保流体先于方块注册）
    fluid::FluidRegistry::instance().initialize();
    fluid::FluidTags::initialize();

    // 空气
    AIR = &registry.registerBlock<AirBlock>(ResourceLocation("minecraft:air"),
        BlockProperties(Material::AIR)
            .noCollision()
            .notSolid()
            .replaceable()
            .opacity(0)
            .propagatesSkylightDown()
            .noLootTable());

    CAVE_AIR = &registry.registerBlock<AirBlock>(ResourceLocation("minecraft:cave_air"),
        BlockProperties(Material::AIR)
            .noCollision()
            .notSolid()
            .replaceable()
            .opacity(0)
            .propagatesSkylightDown()
            .noLootTable());

    VOID_AIR = &registry.registerBlock<AirBlock>(ResourceLocation("minecraft:void_air"),
        BlockProperties(Material::AIR)
            .noCollision()
            .notSolid()
            .replaceable()
            .opacity(0)
            .propagatesSkylightDown()
            .noLootTable());

    // 石头
    STONE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:stone"),
        BlockProperties(Material::ROCK)
            .hardness(1.5f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(0)
            .requiresTool());

    // 草方块
    GRASS_BLOCK = &registry.registerBlock<blocks::GrassBlock>(ResourceLocation("minecraft:grass_block"),
        BlockProperties(Material::EARTH).hardness(0.6f).soundType(BlockSoundTypes::GRASS));

    // 泥土
    DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:dirt"), BlockProperties(Material::EARTH).hardness(0.5f));

    // 圆石
    COBBLESTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cobblestone"), BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f));

    // 橡木木板
    OAK_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:oak_planks"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable());

    // 水
    {
        fluid::Fluid* waterFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::WATER_ID);
        if (waterFluid != nullptr) {
            auto* flowingWater = dynamic_cast<fluid::FlowingFluid*>(waterFluid);
            if (flowingWater != nullptr) {
                WATER = &registry.registerBlock<block::LiquidBlock>(ResourceLocation("minecraft:water"),
                    *flowingWater,
                    BlockProperties(Material::WATER).noCollision().notSolid().opacity(0));
            }
        }
    }

    // 岩浆
    {
        fluid::Fluid* lavaFluid = fluid::FluidRegistry::instance().getFluid(fluid::FluidRegistry::LAVA_ID);
        if (lavaFluid != nullptr) {
            auto* flowingLava = dynamic_cast<fluid::FlowingFluid*>(lavaFluid);
            if (flowingLava != nullptr) {
                LAVA = &registry.registerBlock<block::LiquidBlock>(ResourceLocation("minecraft:lava"),
                    *flowingLava,
                    BlockProperties(Material::LAVA).noCollision().notSolid().lightLevel(15));
            }
        }
    }

    // 基岩
    BEDROCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:bedrock"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 沙子
    SAND = &registry.registerBlock<blocks::FallingBlock>(
        ResourceLocation("minecraft:sand"), BlockProperties(Material::SAND).hardness(0.5f));

    // 砾石
    GRAVEL = &registry.registerBlock<blocks::FallingBlock>(
        ResourceLocation("minecraft:gravel"), BlockProperties(Material::SAND).hardness(0.6f));

    // 雪层
    SNOW = &registry.registerBlock<blocks::SnowBlock>(
        ResourceLocation("minecraft:snow"), BlockProperties(Material::SNOW).hardness(0.2f).notSolid().noCollision());

    // 雪块
    SNOW_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:snow_block"), BlockProperties(Material::SNOW).hardness(0.2f));

    // 冰
    ICE = &registry.registerBlock<blocks::IceBlock>(ResourceLocation("minecraft:ice"),
        BlockProperties(Material::ICE).hardness(0.5f).notSolid().opacity(2).propagatesSkylightDown());

    // 玻璃
    GLASS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:glass"),
        BlockProperties(Material::GLASS).hardness(0.3f).notSolid().opacity(0).propagatesSkylightDown());

    // 下界岩
    NETHERRACK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:netherrack"), BlockProperties(Material::ROCK).hardness(0.4f));

    // 荧石
    GLOWSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:glowstone"), BlockProperties(Material::GLASS).hardness(0.3f).lightLevel(15));

    // 末地石
    END_STONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:end_stone"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(9.0f));

    // 黑曜石
    OBSIDIAN = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:obsidian"), BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f));

    // ========== 石头变种 ==========
    GRANITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:granite"), BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    POLISHED_GRANITE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_granite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    DIORITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diorite"), BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    POLISHED_DIORITE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_diorite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    ANDESITE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:andesite"), BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));
    POLISHED_ANDESITE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:polished_andesite"),
        BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f));

    // ========== 泥土变种 ==========
    COARSE_DIRT = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coarse_dirt"), BlockProperties(Material::EARTH).hardness(0.5f));
    PODZOL = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:podzol"), BlockProperties(Material::EARTH).hardness(0.5f));

    // ========== 砂岩系列 ==========
    SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:sandstone"), BlockProperties(Material::ROCK).hardness(0.8f));
    CHISELED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:chiseled_sandstone"), BlockProperties(Material::ROCK).hardness(0.8f));
    CUT_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:cut_sandstone"), BlockProperties(Material::ROCK).hardness(0.8f));
    RED_SANDSTONE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:red_sandstone"), BlockProperties(Material::ROCK).hardness(0.8f));

    // ========== 矿石方块 ==========
    GOLD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    IRON_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    COAL_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:coal_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    DIAMOND_ORE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:diamond_ore"),
        BlockProperties(Material::ROCK)
            .hardness(3.0f)
            .resistance(3.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(2)
            .requiresTool());
    DIAMOND_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:diamond_block"), BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f));
    EMERALD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    LAPIS_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    REDSTONE_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    COPPER_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:copper_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    NETHER_QUARTZ_ORE = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:nether_quartz_ore"),
        BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    NETHER_GOLD_ORE = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:nether_gold_ore"), BlockProperties(Material::ROCK).hardness(3.0f).resistance(3.0f));
    ANCIENT_DEBRIS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:ancient_debris"),
        BlockProperties(Material::ROCK).hardness(50.0f).resistance(1200.0f));

    // ========== 矿物方块 ==========
    COAL_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:coal_block"),
        BlockProperties(Material::ROCK)
            .hardness(5.0f)
            .resistance(6.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(1)
            .requiresTool());
    GOLD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:gold_block"), BlockProperties(Material::IRON).hardness(3.0f).resistance(6.0f));
    IRON_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:iron_block"), BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f));
    LAPIS_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:lapis_block"), BlockProperties(Material::IRON).hardness(3.0f).resistance(3.0f));
    EMERALD_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:emerald_block"), BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f));
    REDSTONE_BLOCK = &registry.registerBlock<SimpleBlock>(
        ResourceLocation("minecraft:redstone_block"), BlockProperties(Material::IRON).hardness(5.0f).resistance(6.0f));
    NETHERITE_BLOCK = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:netherite_block"),
        BlockProperties(Material::IRON)
            .hardness(50.0f)
            .resistance(1200.0f)
            .harvestTool(HarvestTool::Pickaxe)
            .harvestLevel(3)
            .requiresTool());

    // ========== 原木和树叶 ==========
    BlockProperties logProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable();
    BlockProperties leavesProps =
        BlockProperties(Material::LEAVES).hardness(0.2f).flammable().notSolid().opacity(1).propagatesSkylightDown();

    OAK_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:oak_log"), logProps);
    OAK_LEAVES = &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:oak_leaves"), leavesProps);
    OAK_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:oak_wood"), logProps);
    SPRUCE_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:spruce_wood"), logProps);
    BIRCH_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:birch_wood"), logProps);
    JUNGLE_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:jungle_wood"), logProps);
    ACACIA_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:acacia_wood"), logProps);
    DARK_OAK_WOOD = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:dark_oak_wood"), logProps);

    STRIPPED_OAK_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_oak_log"), logProps);
    STRIPPED_SPRUCE_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_spruce_log"), logProps);
    STRIPPED_BIRCH_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_birch_log"), logProps);
    STRIPPED_JUNGLE_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_jungle_log"), logProps);
    STRIPPED_ACACIA_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_acacia_log"), logProps);
    STRIPPED_DARK_OAK_LOG =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_dark_oak_log"), logProps);

    STRIPPED_OAK_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_oak_wood"), logProps);
    STRIPPED_SPRUCE_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_spruce_wood"), logProps);
    STRIPPED_BIRCH_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_birch_wood"), logProps);
    STRIPPED_JUNGLE_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_jungle_wood"), logProps);
    STRIPPED_ACACIA_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_acacia_wood"), logProps);
    STRIPPED_DARK_OAK_WOOD =
        &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:stripped_dark_oak_wood"), logProps);

    SPRUCE_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:spruce_log"), logProps);
    SPRUCE_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:spruce_leaves"), leavesProps);
    BIRCH_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:birch_log"), logProps);
    BIRCH_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:birch_leaves"), leavesProps);
    JUNGLE_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:jungle_log"), logProps);
    JUNGLE_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:jungle_leaves"), leavesProps);
    ACACIA_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:acacia_log"), logProps);
    ACACIA_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:acacia_leaves"), leavesProps);
    DARK_OAK_LOG = &registry.registerBlock<RotatedPillarBlock>(ResourceLocation("minecraft:dark_oak_log"), logProps);
    DARK_OAK_LEAVES =
        &registry.registerBlock<blocks::LeavesBlock>(ResourceLocation("minecraft:dark_oak_leaves"), leavesProps);

    // ========== 木板变种 ==========
    BlockProperties planksProps = BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable();
    SPRUCE_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:spruce_planks"), planksProps);
    BIRCH_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:birch_planks"), planksProps);
    JUNGLE_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:jungle_planks"), planksProps);
    ACACIA_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:acacia_planks"), planksProps);
    DARK_OAK_PLANKS = &registry.registerBlock<SimpleBlock>(ResourceLocation("minecraft:dark_oak_planks"), planksProps);
}

} // namespace block_registry
} // namespace mc
