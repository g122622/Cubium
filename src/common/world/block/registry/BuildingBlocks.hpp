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

#pragma once

#include "world/block/Block.hpp"

namespace mc {
namespace block_registry {

/**
 * @brief 建筑、功能、含水方块的静态引用
 */
struct BuildingBlocks {
    // 建筑方块
    static Block* BRICKS;
    static Block* MOSSY_COBBLESTONE;
    static Block* BOOKSHELF;
    static Block* TNT;
    static Block* SPONGE;
    static Block* WET_SPONGE;

    // 功能方块
    static Block* CRAFTING_TABLE;
    static Block* FURNACE;
    static Block* BLAST_FURNACE;
    static Block* SMOKER;
    static Block* CAULDRON;
    static Block* WATER_CAULDRON;
    static Block* LAVA_CAULDRON;
    static Block* POWDER_SNOW_CAULDRON;
    static Block* ENCHANTING_TABLE;
    static Block* CHEST;
    static Block* TRAPPED_CHEST;
    static Block* SHULKER_BOX;
    static Block* LOOM;
    static Block* BARREL;
    static Block* GRINDSTONE;
    static Block* STONECUTTER;
    static Block* CARTOGRAPHY_TABLE;
    static Block* FLETCHING_TABLE;
    static Block* SMITHING_TABLE;
    static Block* COMPOSTER;
    static Block* CAKE;
    static Block* LECTERN;
    static Block* JUKEBOX;

    // 含水方块
    static Block* LADDER;
    static Block* CHAIN;
    static Block* SCAFFOLDING;
    static Block* GLASS_PANE;
    static Block* IRON_BARS;

    // 石砖系列
    static Block* STONE_BRICKS;
    static Block* MOSSY_STONE_BRICKS;
    static Block* CRACKED_STONE_BRICKS;
    static Block* CHISELED_STONE_BRICKS;
    static Block* STONE_BRICK_STAIRS;
    static Block* STONE_BRICK_SLAB;
    static Block* MOSSY_STONE_BRICK_STAIRS;
    static Block* MOSSY_STONE_BRICK_SLAB;
    static Block* MOSSY_STONE_BRICK_WALL;

    // 虫蚀方块系列
    static Block* INFESTED_STONE;
    static Block* INFESTED_COBBLESTONE;
    static Block* INFESTED_STONE_BRICKS;
    static Block* INFESTED_MOSSY_STONE_BRICKS;
    static Block* INFESTED_CRACKED_STONE_BRICKS;
    static Block* INFESTED_CHISELED_STONE_BRICKS;

    // 石英系列
    static Block* QUARTZ_BLOCK;
    static Block* CHISELED_QUARTZ_BLOCK;
    static Block* QUARTZ_PILLAR;
    static Block* SMOOTH_QUARTZ;

    // 海晶系列
    static Block* PRISMARINE;
    static Block* PRISMARINE_BRICKS;
    static Block* DARK_PRISMARINE;
    static Block* PRISMARINE_STAIRS;
    static Block* PRISMARINE_BRICK_STAIRS;
    static Block* DARK_PRISMARINE_STAIRS;
    static Block* PRISMARINE_SLAB;
    static Block* PRISMARINE_BRICK_SLAB;
    static Block* DARK_PRISMARINE_SLAB;
    static Block* SEA_LANTERN;

    // 紫珀系列
    static Block* PURPUR_BLOCK;
    static Block* PURPUR_PILLAR;

    // 骨块与干草块
    static Block* BONE_BLOCK;
    static Block* HAY_BLOCK;

    // 铁砧系列
    static Block* ANVIL;
    static Block* CHIPPED_ANVIL;
    static Block* DAMAGED_ANVIL;
};

void registerBuildingBlocks();

} // namespace block_registry
} // namespace mc
