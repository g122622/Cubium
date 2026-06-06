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
 * @brief 苍白花园方块系列的静态引用
 *
 * 苍白花园是1.21.2加入的生物群系，包含苍白橡木、树脂等。
 */
struct PaleGardenBlocks {
    // 苍白橡木原木和木材
    static Block* PALE_OAK_LOG;
    static Block* PALE_OAK_WOOD;
    static Block* STRIPPED_PALE_OAK_LOG;
    static Block* STRIPPED_PALE_OAK_WOOD;

    // 苍白橡木木板和树叶
    static Block* PALE_OAK_PLANKS;
    static Block* PALE_OAK_LEAVES;
    static Block* PALE_OAK_SAPLING;

    // 苍白橡木建筑方块
    static Block* PALE_OAK_STAIRS;
    static Block* PALE_OAK_SLAB;
    static Block* PALE_OAK_FENCE;
    static Block* PALE_OAK_FENCE_GATE;
    static Block* PALE_OAK_DOOR;
    static Block* PALE_OAK_TRAPDOOR;

    // 苍白苔藓
    static Block* PALE_MOSS_BLOCK;
    static Block* PALE_MOSS_CARPET;
    static Block* PALE_HANGING_MOSS;

    // 眼眸花
    static Block* OPEN_EYEBLOSSOM;
    static Block* CLOSED_EYEBLOSSOM;

    // 嘎枝之心
    static Block* CREAKING_HEART;

    // 树脂系列
    static Block* RESIN_CLUMP;
    static Block* RESIN_BLOCK;
    static Block* RESIN_BRICKS;
    static Block* CHISELED_RESIN_BRICKS;
    static Block* RESIN_BRICK_STAIRS;
    static Block* RESIN_BRICK_SLAB;
    static Block* RESIN_BRICK_WALL;
};

void registerPaleGardenBlocks();

} // namespace block_registry
} // namespace mc
