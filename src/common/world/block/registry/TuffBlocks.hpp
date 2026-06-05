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
 * @brief 凝灰岩方块系列的静态引用
 *
 * 凝灰岩是1.17加入的岩石方块，在1.21中获得了更多变体。
 */
struct TuffBlocks {
    // 基础凝灰岩
    static Block* TUFF;
    static Block* POLISHED_TUFF;
    static Block* TUFF_BRICKS;
    static Block* CHISELED_TUFF;
    static Block* CHISELED_TUFF_BRICKS;

    // 凝灰岩楼梯、台阶、墙
    static Block* TUFF_STAIRS;
    static Block* TUFF_SLAB;
    static Block* TUFF_WALL;

    // 磨制凝灰岩楼梯、台阶、墙
    static Block* POLISHED_TUFF_STAIRS;
    static Block* POLISHED_TUFF_SLAB;
    static Block* POLISHED_TUFF_WALL;

    // 凝灰岩砖楼梯、台阶、墙
    static Block* TUFF_BRICK_STAIRS;
    static Block* TUFF_BRICK_SLAB;
    static Block* TUFF_BRICK_WALL;
};

void registerTuffBlocks();

} // namespace block_registry
} // namespace mc
