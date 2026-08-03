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
 * @brief 深板岩系列方块的静态引用
 */
struct DeepslateBlocks {
    // 深板岩基础方块
    static Block* DEEPSLATE;
    static Block* COBBLED_DEEPSLATE;
    static Block* POLISHED_DEEPSLATE;
    static Block* DEEPSLATE_BRICKS;
    static Block* DEEPSLATE_TILES;
    static Block* CHISELED_DEEPSLATE;
    static Block* CRACKED_DEEPSLATE_BRICKS;
    static Block* CRACKED_DEEPSLATE_TILES;
    static Block* REINFORCED_DEEPSLATE;

    // 深板岩矿石变种
    static Block* DEEPSLATE_COAL_ORE;
    static Block* DEEPSLATE_IRON_ORE;
    static Block* DEEPSLATE_COPPER_ORE;
    static Block* DEEPSLATE_GOLD_ORE;
    static Block* DEEPSLATE_DIAMOND_ORE;
    static Block* DEEPSLATE_LAPIS_ORE;
    static Block* DEEPSLATE_EMERALD_ORE;
    static Block* DEEPSLATE_REDSTONE_ORE;

    // 深板岩圆石建筑方块
    static Block* COBBLED_DEEPSLATE_STAIRS;
    static Block* COBBLED_DEEPSLATE_SLAB;
    static Block* COBBLED_DEEPSLATE_WALL;

    // 磨制深板岩建筑方块
    static Block* POLISHED_DEEPSLATE_STAIRS;
    static Block* POLISHED_DEEPSLATE_SLAB;
    static Block* POLISHED_DEEPSLATE_WALL;

    // 深板岩砖建筑方块
    static Block* DEEPSLATE_BRICK_STAIRS;
    static Block* DEEPSLATE_BRICK_SLAB;
    static Block* DEEPSLATE_BRICK_WALL;

    // 深板岩瓦建筑方块
    static Block* DEEPSLATE_TILE_STAIRS;
    static Block* DEEPSLATE_TILE_SLAB;
    static Block* DEEPSLATE_TILE_WALL;

    // 其他
    static Block* SMOOTH_BASALT;
    static Block* INFESTED_DEEPSLATE;
};

void registerDeepslateBlocks();

} // namespace block_registry
} // namespace mc
