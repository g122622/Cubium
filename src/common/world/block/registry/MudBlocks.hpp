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
 * @brief 泥巴系列方块的静态引用（1.19 荒野更新）
 *
 * 泥巴生成于红树林沼泽，可以用泥砖建造建筑。
 */
struct MudBlocks {
    // 泥巴 - 慢速方块，锹有效
    static Block* MUD;

    // 泥坯 - 泥巴压实后的方块
    static Block* PACKED_MUD;

    // 泥砖 - 建筑材料
    static Block* MUD_BRICKS;

    // 泥砖楼梯
    static Block* MUD_BRICK_STAIRS;

    // 泥砖台阶
    static Block* MUD_BRICK_SLAB;

    // 泥砖墙
    static Block* MUD_BRICK_WALL;
};

void registerMudBlocks();

} // namespace block_registry
} // namespace mc
