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
 * @brief 楼梯、台阶、墙、栅栏、门、活板门、栅栏门、染色玻璃板的静态引用
 */
struct BuildingVariantBlocks {
    // 楼梯
    static Block* OAK_STAIRS;
    static Block* SPRUCE_STAIRS;
    static Block* BIRCH_STAIRS;
    static Block* JUNGLE_STAIRS;
    static Block* ACACIA_STAIRS;
    static Block* DARK_OAK_STAIRS;
    static Block* STONE_STAIRS;
    static Block* COBBLESTONE_STAIRS;
    static Block* SANDSTONE_STAIRS;
    static Block* SMOOTH_SANDSTONE_STAIRS;
    static Block* GRANITE_STAIRS;
    static Block* POLISHED_GRANITE_STAIRS;
    static Block* DIORITE_STAIRS;
    static Block* POLISHED_DIORITE_STAIRS;
    static Block* ANDESITE_STAIRS;
    static Block* POLISHED_ANDESITE_STAIRS;
    static Block* BRICK_STAIRS;
    static Block* MOSSY_COBBLESTONE_STAIRS;
    static Block* QUARTZ_STAIRS;
    static Block* SMOOTH_QUARTZ_STAIRS;
    static Block* PURPUR_STAIRS;
    static Block* RED_SANDSTONE_STAIRS;
    static Block* SMOOTH_RED_SANDSTONE_STAIRS;

    // 台阶
    static Block* OAK_SLAB;
    static Block* SPRUCE_SLAB;
    static Block* BIRCH_SLAB;
    static Block* JUNGLE_SLAB;
    static Block* ACACIA_SLAB;
    static Block* DARK_OAK_SLAB;
    static Block* STONE_SLAB;
    static Block* COBBLESTONE_SLAB;
    static Block* SANDSTONE_SLAB;
    static Block* SMOOTH_SANDSTONE_SLAB;
    static Block* GRANITE_SLAB;
    static Block* POLISHED_GRANITE_SLAB;
    static Block* DIORITE_SLAB;
    static Block* POLISHED_DIORITE_SLAB;
    static Block* ANDESITE_SLAB;
    static Block* POLISHED_ANDESITE_SLAB;
    static Block* BRICK_SLAB;
    static Block* MOSSY_COBBLESTONE_SLAB;
    static Block* QUARTZ_SLAB;
    static Block* SMOOTH_QUARTZ_SLAB;
    static Block* PURPUR_SLAB;
    static Block* RED_SANDSTONE_SLAB;
    static Block* SMOOTH_RED_SANDSTONE_SLAB;
    static Block* CUT_SANDSTONE_SLAB;
    static Block* CUT_RED_SANDSTONE_SLAB;
    static Block* SMOOTH_STONE_SLAB;
    static Block* PETRIFIED_OAK_SLAB;

    // 墙
    static Block* COBBLESTONE_WALL;
    static Block* STONE_BRICK_WALL;
    static Block* MOSSY_COBBLESTONE_WALL;
    static Block* BRICK_WALL;
    static Block* PRISMARINE_WALL;
    static Block* SANDSTONE_WALL;
    static Block* RED_SANDSTONE_WALL;
    static Block* GRANITE_WALL;
    static Block* DIORITE_WALL;
    static Block* ANDESITE_WALL;

    // 栅栏
    static Block* OAK_FENCE;
    static Block* SPRUCE_FENCE;
    static Block* BIRCH_FENCE;
    static Block* JUNGLE_FENCE;
    static Block* ACACIA_FENCE;
    static Block* DARK_OAK_FENCE;

    // 门和栅栏门
    static Block* OAK_DOOR;
    static Block* SPRUCE_DOOR;
    static Block* BIRCH_DOOR;
    static Block* JUNGLE_DOOR;
    static Block* ACACIA_DOOR;
    static Block* DARK_OAK_DOOR;
    static Block* CRIMSON_DOOR;
    static Block* WARPED_DOOR;
    static Block* IRON_DOOR;
    static Block* OAK_FENCE_GATE;
    static Block* SPRUCE_FENCE_GATE;
    static Block* BIRCH_FENCE_GATE;
    static Block* JUNGLE_FENCE_GATE;
    static Block* ACACIA_FENCE_GATE;
    static Block* DARK_OAK_FENCE_GATE;
    static Block* CRIMSON_FENCE_GATE;
    static Block* WARPED_FENCE_GATE;

    // 活板门
    static Block* OAK_TRAPDOOR;
    static Block* SPRUCE_TRAPDOOR;
    static Block* BIRCH_TRAPDOOR;
    static Block* JUNGLE_TRAPDOOR;
    static Block* ACACIA_TRAPDOOR;
    static Block* DARK_OAK_TRAPDOOR;
    static Block* CRIMSON_TRAPDOOR;
    static Block* WARPED_TRAPDOOR;
    static Block* IRON_TRAPDOOR;

    // 染色玻璃板 (16色)
    static Block* WHITE_STAINED_GLASS_PANE;
    static Block* ORANGE_STAINED_GLASS_PANE;
    static Block* MAGENTA_STAINED_GLASS_PANE;
    static Block* LIGHT_BLUE_STAINED_GLASS_PANE;
    static Block* YELLOW_STAINED_GLASS_PANE;
    static Block* LIME_STAINED_GLASS_PANE;
    static Block* PINK_STAINED_GLASS_PANE;
    static Block* GRAY_STAINED_GLASS_PANE;
    static Block* LIGHT_GRAY_STAINED_GLASS_PANE;
    static Block* CYAN_STAINED_GLASS_PANE;
    static Block* PURPLE_STAINED_GLASS_PANE;
    static Block* BLUE_STAINED_GLASS_PANE;
    static Block* BROWN_STAINED_GLASS_PANE;
    static Block* GREEN_STAINED_GLASS_PANE;
    static Block* RED_STAINED_GLASS_PANE;
    static Block* BLACK_STAINED_GLASS_PANE;

    // 特殊方块
    static Block* SPAWNER;
    static Block* STRUCTURE_BLOCK;
    static Block* STRUCTURE_VOID;
    static Block* JIGSAW;
    static Block* BARRIER;
    static Block* COMMAND_BLOCK;
    static Block* REPEATING_COMMAND_BLOCK;
    static Block* CHAIN_COMMAND_BLOCK;
};

void registerBuildingVariantBlocks();

} // namespace block_registry
} // namespace mc
