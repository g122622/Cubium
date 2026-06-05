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
 * @brief 洞穴方块（1.17 洞穴与山崖 Part 1）的静态引用
 */
struct CaveBlocks {
    // 紫水晶系列
    static Block* AMETHYST_BLOCK;
    static Block* BUDDING_AMETHYST;
    static Block* SMALL_AMETHYST_BUD;
    static Block* MEDIUM_AMETHYST_BUD;
    static Block* LARGE_AMETHYST_BUD;
    static Block* AMETHYST_CLUSTER;

    // 滴水石系列
    static Block* DRIPSTONE_BLOCK;
    static Block* POINTED_DRIPSTONE;

    // 方解石
    static Block* CALCITE;

    // 遮光玻璃
    static Block* TINTED_GLASS;

    // 苔藓系列
    static Block* MOSS_BLOCK;
    static Block* MOSS_CARPET;

    // 杜鹃花系列
    static Block* AZALEA;
    static Block* FLOWERING_AZALEA;
    static Block* AZALEA_LEAVES;
    static Block* FLOWERING_AZALEA_LEAVES;

    // 大垂滴叶系列
    static Block* BIG_DRIPLEAF;
    static Block* BIG_DRIPLEAF_STEM;
    static Block* SMALL_DRIPLEAF;

    // 其他洞穴方块
    static Block* HANGING_ROOTS;
    static Block* ROOTED_DIRT;
    static Block* SPORE_BLOSSOM;
    static Block* GLOW_LICHEN;
    static Block* CAVE_VINES;
    static Block* CAVE_VINES_PLANT;
    static Block* POWDER_SNOW;
};

void registerCaveBlocks();

} // namespace block_registry
} // namespace mc
