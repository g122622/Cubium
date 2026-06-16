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
#include "world/block/BlockRegistry.hpp"
#include "world/block/HarvestTool.hpp"
#include "world/block/blocks/DoorBlock.hpp"
#include "world/block/blocks/FenceGateBlock.hpp"
#include "world/block/blocks/SimpleBlock.hpp"
#include "world/block/blocks/building/FenceBlock.hpp"
#include "world/block/blocks/building/SlabBlock.hpp"
#include "world/block/blocks/building/StairsBlock.hpp"
#include "world/block/blocks/building/TrapDoorBlock.hpp"
#include "world/block/blocks/building/WallBlock.hpp"
#include "world/block/blocks/decorative/PaneBlock.hpp"
#include "world/block/blocks/mob/SpawnerBlock.hpp"
#include "world/block/blocks/special/SpecialBlocks.hpp"
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

// 墙
Block* BuildingVariantBlocks::COBBLESTONE_WALL = nullptr;
Block* BuildingVariantBlocks::STONE_BRICK_WALL = nullptr;

// 栅栏
Block* BuildingVariantBlocks::OAK_FENCE = nullptr;

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

    // ========== 栅栏 ==========
    // 橡木栅栏
    BuildingVariantBlocks::OAK_FENCE =
        &registry.registerBlock<blocks::FenceBlock>(ResourceLocation("minecraft:oak_fence"),
            BlockProperties(Material::WOOD).hardness(2.0f).resistance(3.0f).flammable().ignitedByLava());

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
}

} // namespace block_registry
} // namespace mc
