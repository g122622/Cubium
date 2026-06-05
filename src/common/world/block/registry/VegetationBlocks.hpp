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
 * @brief 植被方块（草、花、蘑菇、树苗、南瓜西瓜等）的静态引用
 */
struct VegetationBlocks {
    // 植被方块
    static Block* SHORT_GRASS;
    static Block* TALL_GRASS;
    static Block* FERN;
    static Block* DANDELION;
    static Block* POPPY;
    static Block* BLUE_ORCHID;
    static Block* ALLIUM;
    static Block* AZURE_BLUET;
    static Block* RED_TULIP;
    static Block* ORANGE_TULIP;
    static Block* WHITE_TULIP;
    static Block* PINK_TULIP;
    static Block* OXEYE_DAISY;
    static Block* LILY_OF_THE_VALLEY;
    static Block* SUNFLOWER;
    static Block* LILAC;
    static Block* ROSE_BUSH;
    static Block* PEONY;
    static Block* LARGE_FERN;
    static Block* CORNFLOWER;
    static Block* WITHER_ROSE;
    static Block* BROWN_MUSHROOM;
    static Block* RED_MUSHROOM;
    static Block* BROWN_MUSHROOM_BLOCK;
    static Block* RED_MUSHROOM_BLOCK;
    static Block* MUSHROOM_STEM;

    // 树苗
    static Block* OAK_SAPLING;
    static Block* SPRUCE_SAPLING;
    static Block* BIRCH_SAPLING;
    static Block* JUNGLE_SAPLING;
    static Block* ACACIA_SAPLING;
    static Block* DARK_OAK_SAPLING;

    // 南瓜和西瓜系列
    static Block* MELON;
    static Block* PUMPKIN;
    static Block* CARVED_PUMPKIN;
    static Block* MELON_STEM;
    static Block* PUMPKIN_STEM;
    static Block* ATTACHED_MELON_STEM;
    static Block* ATTACHED_PUMPKIN_STEM;

    // 竹子
    static Block* BAMBOO;
    static Block* BAMBOO_SAPLING;
};

void registerVegetationBlocks();

} // namespace block_registry
} // namespace mc
