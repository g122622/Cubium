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
 * @brief 樱花系列方块（1.20 足迹与故事）的静态引用
 */
struct CherryBlocks {
    // 樱花原木和木材
    static Block* CHERRY_LOG;
    static Block* CHERRY_WOOD;
    static Block* STRIPPED_CHERRY_LOG;
    static Block* STRIPPED_CHERRY_WOOD;

    // 樱花木板系列
    static Block* CHERRY_PLANKS;
    static Block* CHERRY_STAIRS;
    static Block* CHERRY_SLAB;
    static Block* CHERRY_FENCE;
    static Block* CHERRY_FENCE_GATE;
    static Block* CHERRY_DOOR;
    static Block* CHERRY_TRAPDOOR;

    // 樱花植物
    static Block* CHERRY_LEAVES;
    static Block* CHERRY_SAPLING;
};

void registerCherryBlocks();

} // namespace block_registry
} // namespace mc
