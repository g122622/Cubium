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

#include "world/block/registry/BuildingVariantBlocks.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/Material.hpp"
#include "world/block/BlockRegistry.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/FenceGateBlock.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/block/blocks/mob/SpawnerBlock.hpp"
#include "world/block/blocks/special/BarrierBlock.hpp"
#include "world/block/blocks/special/ChainCommandBlock.hpp"
#include "world/block/blocks/special/CommandBlock.hpp"
#include "world/block/blocks/special/JigsawBlock.hpp"
#include "world/block/blocks/special/RepeatingCommandBlock.hpp"
#include "world/block/blocks/special/StructureBlock.hpp"
#include "world/block/blocks/special/StructureVoidBlock.hpp"
#include "world/block/registry/BaseBlocks.hpp"
#include "world/block/registry/BuildingBlocks.hpp"

namespace mc {
namespace block_registry {

// ============================================================================
// 静态成员初始化
// ============================================================================

// 楼梯
Block* BuildingVariantBlocks::OAK_STAIRS = nullptr;
Block* BuildingVariantBlocks::SPRUCE_STAIRS = nullptr;
Block* BuildingVariantBlocks::BIRCH_STAIRS = nullptr;
Block* BuildingVariantBlocks::JUNGLE_STAIRS = nullptr;
Block* BuildingVariantBlocks::ACACIA_STAIRS = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_STAIRS = nullptr;
Block* BuildingVariantBlocks::STONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::COBBLESTONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::SANDSTONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::SMOOTH_SANDSTONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::GRANITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::POLISHED_GRANITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::DIORITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::POLISHED_DIORITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::ANDESITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::POLISHED_ANDESITE_STAIRS = nullptr;
Block* BuildingVariantBlocks::BRICK_STAIRS = nullptr;
Block* BuildingVariantBlocks::MOSSY_COBBLESTONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::QUARTZ_STAIRS = nullptr;
Block* BuildingVariantBlocks::SMOOTH_QUARTZ_STAIRS = nullptr;
Block* BuildingVariantBlocks::PURPUR_STAIRS = nullptr;
Block* BuildingVariantBlocks::RED_SANDSTONE_STAIRS = nullptr;
Block* BuildingVariantBlocks::SMOOTH_RED_SANDSTONE_STAIRS = nullptr;

// 台阶
Block* BuildingVariantBlocks::OAK_SLAB = nullptr;
Block* BuildingVariantBlocks::SPRUCE_SLAB = nullptr;
Block* BuildingVariantBlocks::BIRCH_SLAB = nullptr;
Block* BuildingVariantBlocks::JUNGLE_SLAB = nullptr;
Block* BuildingVariantBlocks::ACACIA_SLAB = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_SLAB = nullptr;
Block* BuildingVariantBlocks::STONE_SLAB = nullptr;
Block* BuildingVariantBlocks::COBBLESTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::SMOOTH_SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::GRANITE_SLAB = nullptr;
Block* BuildingVariantBlocks::POLISHED_GRANITE_SLAB = nullptr;
Block* BuildingVariantBlocks::DIORITE_SLAB = nullptr;
Block* BuildingVariantBlocks::POLISHED_DIORITE_SLAB = nullptr;
Block* BuildingVariantBlocks::ANDESITE_SLAB = nullptr;
Block* BuildingVariantBlocks::POLISHED_ANDESITE_SLAB = nullptr;
Block* BuildingVariantBlocks::BRICK_SLAB = nullptr;
Block* BuildingVariantBlocks::MOSSY_COBBLESTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::QUARTZ_SLAB = nullptr;
Block* BuildingVariantBlocks::SMOOTH_QUARTZ_SLAB = nullptr;
Block* BuildingVariantBlocks::PURPUR_SLAB = nullptr;
Block* BuildingVariantBlocks::RED_SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::SMOOTH_RED_SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::CUT_SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::CUT_RED_SANDSTONE_SLAB = nullptr;
Block* BuildingVariantBlocks::SMOOTH_STONE_SLAB = nullptr;
Block* BuildingVariantBlocks::PETRIFIED_OAK_SLAB = nullptr;

// 墙
Block* BuildingVariantBlocks::COBBLESTONE_WALL = nullptr;
Block* BuildingVariantBlocks::STONE_BRICK_WALL = nullptr;
Block* BuildingVariantBlocks::MOSSY_COBBLESTONE_WALL = nullptr;
Block* BuildingVariantBlocks::BRICK_WALL = nullptr;
Block* BuildingVariantBlocks::PRISMARINE_WALL = nullptr;
Block* BuildingVariantBlocks::SANDSTONE_WALL = nullptr;
Block* BuildingVariantBlocks::RED_SANDSTONE_WALL = nullptr;
Block* BuildingVariantBlocks::QUARTZ_WALL = nullptr;
Block* BuildingVariantBlocks::PURPUR_WALL = nullptr;
Block* BuildingVariantBlocks::GRANITE_WALL = nullptr;
Block* BuildingVariantBlocks::DIORITE_WALL = nullptr;
Block* BuildingVariantBlocks::ANDESITE_WALL = nullptr;

// 栅栏
Block* BuildingVariantBlocks::OAK_FENCE = nullptr;
Block* BuildingVariantBlocks::SPRUCE_FENCE = nullptr;
Block* BuildingVariantBlocks::BIRCH_FENCE = nullptr;
Block* BuildingVariantBlocks::JUNGLE_FENCE = nullptr;
Block* BuildingVariantBlocks::ACACIA_FENCE = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_FENCE = nullptr;

// 门和栅栏门
Block* BuildingVariantBlocks::OAK_DOOR = nullptr;
Block* BuildingVariantBlocks::SPRUCE_DOOR = nullptr;
Block* BuildingVariantBlocks::BIRCH_DOOR = nullptr;
Block* BuildingVariantBlocks::JUNGLE_DOOR = nullptr;
Block* BuildingVariantBlocks::ACACIA_DOOR = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_DOOR = nullptr;
Block* BuildingVariantBlocks::CRIMSON_DOOR = nullptr;
Block* BuildingVariantBlocks::WARPED_DOOR = nullptr;
Block* BuildingVariantBlocks::IRON_DOOR = nullptr;
Block* BuildingVariantBlocks::OAK_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::SPRUCE_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::BIRCH_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::JUNGLE_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::ACACIA_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::CRIMSON_FENCE_GATE = nullptr;
Block* BuildingVariantBlocks::WARPED_FENCE_GATE = nullptr;

// 活板门
Block* BuildingVariantBlocks::OAK_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::SPRUCE_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::BIRCH_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::JUNGLE_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::ACACIA_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::DARK_OAK_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::CRIMSON_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::WARPED_TRAPDOOR = nullptr;
Block* BuildingVariantBlocks::IRON_TRAPDOOR = nullptr;

// 染色玻璃板 (16色)
Block* BuildingVariantBlocks::WHITE_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::ORANGE_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::MAGENTA_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::LIGHT_BLUE_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::YELLOW_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::LIME_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::PINK_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::GRAY_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::LIGHT_GRAY_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::CYAN_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::PURPLE_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::BLUE_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::BROWN_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::GREEN_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::RED_STAINED_GLASS_PANE = nullptr;
Block* BuildingVariantBlocks::BLACK_STAINED_GLASS_PANE = nullptr;

// 特殊方块
Block* BuildingVariantBlocks::SPAWNER = nullptr;
Block* BuildingVariantBlocks::STRUCTURE_BLOCK = nullptr;
Block* BuildingVariantBlocks::STRUCTURE_VOID = nullptr;
Block* BuildingVariantBlocks::JIGSAW = nullptr;
Block* BuildingVariantBlocks::BARRIER = nullptr;
Block* BuildingVariantBlocks::COMMAND_BLOCK = nullptr;
Block* BuildingVariantBlocks::REPEATING_COMMAND_BLOCK = nullptr;
Block* BuildingVariantBlocks::CHAIN_COMMAND_BLOCK = nullptr;

// ============================================================================
// 楼梯、台阶、墙、栅栏、门、栅栏门、活板门、染色玻璃板、特殊方块注册
// ============================================================================
void registerBuildingVariantBlocks()
{
    auto& registry = BlockRegistry::instance();

    // ========== 楼梯 ==========
    // 橡木楼梯
    BuildingVariantBlocks::OAK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:oak_stairs"),
            BaseBlocks::OAK_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 云杉木楼梯
    BuildingVariantBlocks::SPRUCE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:spruce_stairs"),
            BaseBlocks::SPRUCE_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 白桦木楼梯
    BuildingVariantBlocks::BIRCH_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:birch_stairs"),
            BaseBlocks::BIRCH_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 丛林木楼梯
    BuildingVariantBlocks::JUNGLE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:jungle_stairs"),
            BaseBlocks::JUNGLE_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 金合欢木楼梯
    BuildingVariantBlocks::ACACIA_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:acacia_stairs"),
            BaseBlocks::ACACIA_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 深色橡木楼梯
    BuildingVariantBlocks::DARK_OAK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:dark_oak_stairs"),
            BaseBlocks::DARK_OAK_PLANKS->defaultState(),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 石头楼梯
    BuildingVariantBlocks::STONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:stone_stairs"),
            BaseBlocks::STONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 圆石楼梯
    BuildingVariantBlocks::COBBLESTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:cobblestone_stairs"),
            BaseBlocks::COBBLESTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砂岩楼梯
    BuildingVariantBlocks::SANDSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:sandstone_stairs"),
            BaseBlocks::SANDSTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(0.8f).harvestTool(HarvestTool::Pickaxe));

    // 平滑砂岩楼梯
    BuildingVariantBlocks::SMOOTH_SANDSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:smooth_sandstone_stairs"),
            BaseBlocks::SMOOTH_SANDSTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 花岗岩楼梯
    BuildingVariantBlocks::GRANITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:granite_stairs"),
            BaseBlocks::GRANITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制花岗岩楼梯
    BuildingVariantBlocks::POLISHED_GRANITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_granite_stairs"),
            BaseBlocks::POLISHED_GRANITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 闪长岩楼梯
    BuildingVariantBlocks::DIORITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:diorite_stairs"),
            BaseBlocks::DIORITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制闪长岩楼梯
    BuildingVariantBlocks::POLISHED_DIORITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_diorite_stairs"),
            BaseBlocks::POLISHED_DIORITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 安山岩楼梯
    BuildingVariantBlocks::ANDESITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:andesite_stairs"),
            BaseBlocks::ANDESITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制安山岩楼梯
    BuildingVariantBlocks::POLISHED_ANDESITE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:polished_andesite_stairs"),
            BaseBlocks::POLISHED_ANDESITE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砖楼梯
    BuildingVariantBlocks::BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:brick_stairs"),
            BuildingBlocks::BRICKS->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 苔石楼梯
    BuildingVariantBlocks::MOSSY_COBBLESTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:mossy_cobblestone_stairs"),
            BuildingBlocks::MOSSY_COBBLESTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 石英楼梯
    BuildingVariantBlocks::QUARTZ_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:quartz_stairs"),
            BuildingBlocks::QUARTZ_BLOCK->defaultState(),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(0.8f).harvestTool(HarvestTool::Pickaxe));

    // 平滑石英楼梯
    BuildingVariantBlocks::SMOOTH_QUARTZ_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:smooth_quartz_stairs"),
            BuildingBlocks::SMOOTH_QUARTZ->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 紫珀楼梯
    BuildingVariantBlocks::PURPUR_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:purpur_stairs"),
            BuildingBlocks::PURPUR_BLOCK->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 红砂岩楼梯
    BuildingVariantBlocks::RED_SANDSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:red_sandstone_stairs"),
            BaseBlocks::RED_SANDSTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(0.8f).harvestTool(HarvestTool::Pickaxe));

    // 平滑红砂岩楼梯
    BuildingVariantBlocks::SMOOTH_RED_SANDSTONE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:smooth_red_sandstone_stairs"),
            BaseBlocks::SMOOTH_RED_SANDSTONE->defaultState(),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 海晶楼梯
    BuildingBlocks::PRISMARINE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:prismarine_stairs"),
            BuildingBlocks::PRISMARINE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::PRISMARINE_BRICK_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:prismarine_brick_stairs"),
            BuildingBlocks::PRISMARINE_BRICKS->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::DARK_PRISMARINE_STAIRS =
        &registry.registerBlock<blocks::StairsBlock>(ResourceLocation("minecraft:dark_prismarine_stairs"),
            BuildingBlocks::DARK_PRISMARINE->defaultState(),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // ========== 台阶 ==========
    // 橡木台阶
    BuildingVariantBlocks::OAK_SLAB = &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:oak_slab"),
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 云杉木台阶
    BuildingVariantBlocks::SPRUCE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:spruce_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 白桦木台阶
    BuildingVariantBlocks::BIRCH_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:birch_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 丛林木台阶
    BuildingVariantBlocks::JUNGLE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:jungle_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 金合欢木台阶
    BuildingVariantBlocks::ACACIA_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:acacia_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 深色橡木台阶
    BuildingVariantBlocks::DARK_OAK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:dark_oak_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 石头台阶
    BuildingVariantBlocks::STONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:stone_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 圆石台阶
    BuildingVariantBlocks::COBBLESTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:cobblestone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砂岩台阶
    BuildingVariantBlocks::SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 平滑砂岩台阶
    BuildingVariantBlocks::SMOOTH_SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:smooth_sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 花岗岩台阶
    BuildingVariantBlocks::GRANITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:granite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制花岗岩台阶
    BuildingVariantBlocks::POLISHED_GRANITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_granite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 闪长岩台阶
    BuildingVariantBlocks::DIORITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:diorite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制闪长岩台阶
    BuildingVariantBlocks::POLISHED_DIORITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_diorite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 安山岩台阶
    BuildingVariantBlocks::ANDESITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:andesite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 磨制安山岩台阶
    BuildingVariantBlocks::POLISHED_ANDESITE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:polished_andesite_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砖台阶
    BuildingVariantBlocks::BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:brick_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 苔石台阶
    BuildingVariantBlocks::MOSSY_COBBLESTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:mossy_cobblestone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 石英台阶
    BuildingVariantBlocks::QUARTZ_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:quartz_slab"),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(0.8f).harvestTool(HarvestTool::Pickaxe));

    // 平滑石英台阶
    BuildingVariantBlocks::SMOOTH_QUARTZ_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:smooth_quartz_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 紫珀台阶
    BuildingVariantBlocks::PURPUR_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:purpur_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 红砂岩台阶
    BuildingVariantBlocks::RED_SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:red_sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(0.8f).harvestTool(HarvestTool::Pickaxe));

    // 平滑红砂岩台阶
    BuildingVariantBlocks::SMOOTH_RED_SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:smooth_red_sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 切制砂岩台阶
    BuildingVariantBlocks::CUT_SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:cut_sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 切制红砂岩台阶
    BuildingVariantBlocks::CUT_RED_SANDSTONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:cut_red_sandstone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 平滑石头台阶
    BuildingVariantBlocks::SMOOTH_STONE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:smooth_stone_slab"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 石化橡木台阶
    BuildingVariantBlocks::PETRIFIED_OAK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:petrified_oak_slab"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).flammable().ignitedByLava());

    // 海晶台阶
    BuildingBlocks::PRISMARINE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:prismarine_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::PRISMARINE_BRICK_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:prismarine_brick_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));
    BuildingBlocks::DARK_PRISMARINE_SLAB =
        &registry.registerBlock<blocks::SlabBlock>(ResourceLocation("minecraft:dark_prismarine_slab"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // ========== 墙 ==========
    // 圆石墙
    BuildingVariantBlocks::COBBLESTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:cobblestone_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 石砖墙
    BuildingVariantBlocks::STONE_BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:stone_brick_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 苔石墙
    BuildingVariantBlocks::MOSSY_COBBLESTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:mossy_cobblestone_wall"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砖墙
    BuildingVariantBlocks::BRICK_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:brick_wall"),
            BlockProperties(Material::ROCK).hardness(2.0f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 海晶石墙
    BuildingVariantBlocks::PRISMARINE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:prismarine_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 砂岩墙
    BuildingVariantBlocks::SANDSTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:sandstone_wall"),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 红砂岩墙
    BuildingVariantBlocks::RED_SANDSTONE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:red_sandstone_wall"),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 石英墙
    BuildingVariantBlocks::QUARTZ_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:quartz_wall"),
            BlockProperties(Material::ROCK).hardness(0.8f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 紫珀墙
    BuildingVariantBlocks::PURPUR_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:purpur_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 花岗岩墙
    BuildingVariantBlocks::GRANITE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:granite_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 闪长岩墙
    BuildingVariantBlocks::DIORITE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:diorite_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // 安山岩墙
    BuildingVariantBlocks::ANDESITE_WALL =
        &registry.registerBlock<blocks::WallBlock>(ResourceLocation("minecraft:andesite_wall"),
            BlockProperties(Material::ROCK).hardness(1.5f).resistance(6.0f).harvestTool(HarvestTool::Pickaxe));

    // ========== 栅栏 ==========
    // 橡木栅栏
    BuildingVariantBlocks::OAK_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:oak_fence"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable().ignitedByLava());

    // 其他主世界木材栅栏（云杉、白桦、丛林、金合欢、深色橡木）
    BlockProperties woodFenceProps =
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable().ignitedByLava();

    BuildingVariantBlocks::SPRUCE_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:spruce_fence"), woodFenceProps);
    BuildingVariantBlocks::BIRCH_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:birch_fence"), woodFenceProps);
    BuildingVariantBlocks::JUNGLE_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:jungle_fence"), woodFenceProps);
    BuildingVariantBlocks::ACACIA_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:acacia_fence"), woodFenceProps);
    BuildingVariantBlocks::DARK_OAK_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:dark_oak_fence"), woodFenceProps);

    // ========== 活板门 ==========
    // 木活板门（所有木材类型）
    // 木活板门属性: 硬度3.0, 抗爆3.0, 可燃
    BlockProperties woodTrapdoorProps =
        BlockProperties(Material::WOOD).hardness(3.0f).resistance(3.0f).flammable().ignitedByLava();

    BuildingVariantBlocks::OAK_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:oak_trapdoor"), woodTrapdoorProps, false);
    BuildingVariantBlocks::SPRUCE_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:spruce_trapdoor"), woodTrapdoorProps, false);
    BuildingVariantBlocks::BIRCH_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:birch_trapdoor"), woodTrapdoorProps, false);
    BuildingVariantBlocks::JUNGLE_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:jungle_trapdoor"), woodTrapdoorProps, false);
    BuildingVariantBlocks::ACACIA_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:acacia_trapdoor"), woodTrapdoorProps, false);
    BuildingVariantBlocks::DARK_OAK_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:dark_oak_trapdoor"), woodTrapdoorProps, false);

    // 下界木材活板门（不可燃）
    BlockProperties netherWoodTrapdoorProps = BlockProperties(Material::NETHER_WOOD).hardness(3.0f).resistance(3.0f);
    BuildingVariantBlocks::CRIMSON_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:crimson_trapdoor"), netherWoodTrapdoorProps, false);
    BuildingVariantBlocks::WARPED_TRAPDOOR = &registry.registerBlock<blocks::TrapDoorBlock>(
        ResourceLocation("minecraft:warped_trapdoor"), netherWoodTrapdoorProps, false);

    // 铁活板门
    BuildingVariantBlocks::IRON_TRAPDOOR =
        &registry.registerBlock<blocks::TrapDoorBlock>(ResourceLocation("minecraft:iron_trapdoor"),
            BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).harvestTool(HarvestTool::Pickaxe),
            true);

    // ========== 门 ==========
    // 橡木门
    BuildingVariantBlocks::OAK_DOOR = &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:oak_door"),
        BlockProperties(Material::WOOD).hardness(3.0f).resistance(3.0f).notSolid().flammable().ignitedByLava(),
        false // 不是铁门
    );

    // 铁门
    BuildingVariantBlocks::IRON_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:iron_door"),
            BlockProperties(Material::IRON).hardness(5.0f).resistance(5.0f).notSolid(),
            true // 是铁门
        );

    // 其他木门（所有木材类型）
    BlockProperties woodDoorProps =
        BlockProperties(Material::WOOD).hardness(3.0f).resistance(3.0f).notSolid().flammable().ignitedByLava();

    BuildingVariantBlocks::SPRUCE_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:spruce_door"), woodDoorProps, false);
    BuildingVariantBlocks::BIRCH_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:birch_door"), woodDoorProps, false);
    BuildingVariantBlocks::JUNGLE_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:jungle_door"), woodDoorProps, false);
    BuildingVariantBlocks::ACACIA_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:acacia_door"), woodDoorProps, false);
    BuildingVariantBlocks::DARK_OAK_DOOR =
        &registry.registerBlock<blocks::DoorBlock>(ResourceLocation("minecraft:dark_oak_door"), woodDoorProps, false);

    // 下界木材门（不可燃）
    BlockProperties netherWoodDoorProps =
        BlockProperties(Material::NETHER_WOOD).hardness(3.0f).resistance(3.0f).notSolid();
    BuildingVariantBlocks::CRIMSON_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:crimson_door"), netherWoodDoorProps, false);
    BuildingVariantBlocks::WARPED_DOOR = &registry.registerBlock<blocks::DoorBlock>(
        ResourceLocation("minecraft:warped_door"), netherWoodDoorProps, false);

    // ========== 栅栏门（所有木材类型）==========
    // 橡木栅栏门
    BuildingVariantBlocks::OAK_FENCE_GATE =
        &registry.registerBlock<blocks::FenceGateBlock>(ResourceLocation("minecraft:oak_fence_gate"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).notSolid().flammable().ignitedByLava());

    BlockProperties woodFenceGateProps =
        BlockProperties(Material::WOOD).hardness(2.0f).resistance(2.0f).notSolid().flammable().ignitedByLava();

    BuildingVariantBlocks::SPRUCE_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:spruce_fence_gate"), woodFenceGateProps);
    BuildingVariantBlocks::BIRCH_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:birch_fence_gate"), woodFenceGateProps);
    BuildingVariantBlocks::JUNGLE_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:jungle_fence_gate"), woodFenceGateProps);
    BuildingVariantBlocks::ACACIA_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:acacia_fence_gate"), woodFenceGateProps);
    BuildingVariantBlocks::DARK_OAK_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:dark_oak_fence_gate"), woodFenceGateProps);

    // 下界木材栅栏门（不可燃）
    BlockProperties netherWoodFenceGateProps =
        BlockProperties(Material::NETHER_WOOD).hardness(2.0f).resistance(2.0f).notSolid();
    BuildingVariantBlocks::CRIMSON_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:crimson_fence_gate"), netherWoodFenceGateProps);
    BuildingVariantBlocks::WARPED_FENCE_GATE = &registry.registerBlock<blocks::FenceGateBlock>(
        ResourceLocation("minecraft:warped_fence_gate"), netherWoodFenceGateProps);

    // ========== 染色玻璃板（16色）==========
    BlockProperties stainedGlassPaneProps = BlockProperties(Material::GLASS).hardness(0.3f).notSolid();

    BuildingVariantBlocks::WHITE_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:white_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::ORANGE_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:orange_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::MAGENTA_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:magenta_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::LIGHT_BLUE_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:light_blue_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::YELLOW_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:yellow_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::LIME_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:lime_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::PINK_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:pink_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::GRAY_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:gray_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::LIGHT_GRAY_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:light_gray_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::CYAN_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:cyan_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::PURPLE_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:purple_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::BLUE_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:blue_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::BROWN_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:brown_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::GREEN_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:green_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::RED_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:red_stained_glass_pane"), stainedGlassPaneProps);
    BuildingVariantBlocks::BLACK_STAINED_GLASS_PANE = &registry.registerBlock<blocks::PaneBlock>(
        ResourceLocation("minecraft:black_stained_glass_pane"), stainedGlassPaneProps);

    // ========== 特殊方块 ==========
    // 刷怪笼 - 方块实体，生成生物
    BuildingVariantBlocks::SPAWNER =
        &registry.registerBlock<blocks::SpawnerBlock>(ResourceLocation("minecraft:spawner"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 结构方块 - 创造模式专用，用于保存/加载结构
    BuildingVariantBlocks::STRUCTURE_BLOCK =
        &registry.registerBlock<blocks::StructureBlock>(ResourceLocation("minecraft:structure_block"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 结构空位 - 结构生成时不会替换现有方块
    BuildingVariantBlocks::STRUCTURE_VOID =
        &registry.registerBlock<blocks::StructureVoidBlock>(ResourceLocation("minecraft:structure_void"),
            BlockProperties(Material::STRUCTURE_VOID).noCollision().noLootTable());

    // 拼图方块 - Jigsaw 结构生成系统核心
    BuildingVariantBlocks::JIGSAW = &registry.registerBlock<blocks::JigsawBlock>(ResourceLocation("minecraft:jigsaw"),
        BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 屏障 - 不可见的不可破坏方块
    BuildingVariantBlocks::BARRIER =
        &registry.registerBlock<blocks::BarrierBlock>(ResourceLocation("minecraft:barrier"),
            BlockProperties(Material::BARRIER).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 命令方块 - 红石触发型，需要管理员权限
    BuildingVariantBlocks::COMMAND_BLOCK =
        &registry.registerBlock<blocks::CommandBlock>(ResourceLocation("minecraft:command_block"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 重复命令方块 - 每tick自动执行，需要管理员权限
    BuildingVariantBlocks::REPEATING_COMMAND_BLOCK =
        &registry.registerBlock<blocks::RepeatingCommandBlock>(ResourceLocation("minecraft:repeating_command_block"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());

    // 连锁命令方块 - 链式触发，需要管理员权限
    BuildingVariantBlocks::CHAIN_COMMAND_BLOCK =
        &registry.registerBlock<blocks::ChainCommandBlock>(ResourceLocation("minecraft:chain_command_block"),
            BlockProperties(Material::ROCK).hardness(-1.0f).resistance(3600000.0f).noLootTable());
}

} // namespace block_registry
} // namespace mc
