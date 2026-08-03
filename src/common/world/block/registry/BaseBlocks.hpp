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
 * @brief 基础方块、矿石、矿物方块、原木、石头变种、泥土变种、砂岩的静态引用
 */
struct BaseBlocks {
    // 基础方块
    static Block* AIR;
    static Block* CAVE_AIR;
    static Block* VOID_AIR;
    static Block* STONE;
    static Block* GRASS_BLOCK;
    static Block* DIRT;
    static Block* COBBLESTONE;
    static Block* OAK_PLANKS;
    static Block* WATER;
    static Block* LAVA;
    static Block* BEDROCK;
    static Block* SAND;
    static Block* GRAVEL;

    // 石头变种
    static Block* GRANITE;
    static Block* POLISHED_GRANITE;
    static Block* DIORITE;
    static Block* POLISHED_DIORITE;
    static Block* ANDESITE;
    static Block* POLISHED_ANDESITE;

    // 泥土变种
    static Block* COARSE_DIRT;
    static Block* PODZOL;

    // 砂岩系列
    static Block* SANDSTONE;
    static Block* CHISELED_SANDSTONE;
    static Block* CUT_SANDSTONE;
    static Block* SMOOTH_SANDSTONE;
    static Block* RED_SANDSTONE;
    static Block* CHISELED_RED_SANDSTONE;
    static Block* CUT_RED_SANDSTONE;
    static Block* SMOOTH_RED_SANDSTONE;

    // 矿石方块
    static Block* GOLD_ORE;
    static Block* IRON_ORE;
    static Block* COAL_ORE;
    static Block* DIAMOND_ORE;
    static Block* DIAMOND_BLOCK;
    static Block* EMERALD_ORE;
    static Block* LAPIS_ORE;
    static Block* REDSTONE_ORE;
    static Block* COPPER_ORE;

    // 下界矿石
    static Block* NETHER_QUARTZ_ORE;
    static Block* NETHER_GOLD_ORE;
    static Block* ANCIENT_DEBRIS;

    // 矿物方块
    static Block* COAL_BLOCK;
    static Block* GOLD_BLOCK;
    static Block* IRON_BLOCK;
    static Block* LAPIS_BLOCK;
    static Block* EMERALD_BLOCK;
    static Block* REDSTONE_BLOCK;
    static Block* NETHERITE_BLOCK;

    // 原木和树叶
    static Block* OAK_LOG;
    static Block* OAK_WOOD;
    static Block* OAK_LEAVES;
    static Block* SPRUCE_LOG;
    static Block* SPRUCE_WOOD;
    static Block* BIRCH_LOG;
    static Block* BIRCH_WOOD;
    static Block* JUNGLE_LOG;
    static Block* JUNGLE_WOOD;
    static Block* ACACIA_LOG;
    static Block* ACACIA_WOOD;
    static Block* DARK_OAK_LOG;
    static Block* DARK_OAK_WOOD;
    static Block* STRIPPED_OAK_LOG;
    static Block* STRIPPED_SPRUCE_LOG;
    static Block* STRIPPED_BIRCH_LOG;
    static Block* STRIPPED_JUNGLE_LOG;
    static Block* STRIPPED_ACACIA_LOG;
    static Block* STRIPPED_DARK_OAK_LOG;
    static Block* STRIPPED_OAK_WOOD;
    static Block* STRIPPED_SPRUCE_WOOD;
    static Block* STRIPPED_BIRCH_WOOD;
    static Block* STRIPPED_JUNGLE_WOOD;
    static Block* STRIPPED_ACACIA_WOOD;
    static Block* STRIPPED_DARK_OAK_WOOD;
    static Block* SPRUCE_LEAVES;
    static Block* BIRCH_LEAVES;
    static Block* JUNGLE_LEAVES;
    static Block* ACACIA_LEAVES;
    static Block* DARK_OAK_LEAVES;

    // 木板变种
    static Block* SPRUCE_PLANKS;
    static Block* BIRCH_PLANKS;
    static Block* JUNGLE_PLANKS;
    static Block* ACACIA_PLANKS;
    static Block* DARK_OAK_PLANKS;

    // 其他基础方块
    static Block* SNOW;
    static Block* SNOW_BLOCK;
    static Block* ICE;
    static Block* GLASS;
    static Block* NETHERRACK;
    static Block* GLOWSTONE;
    static Block* END_STONE;
    static Block* OBSIDIAN;
};

void registerBaseBlocks();

} // namespace block_registry
} // namespace mc
