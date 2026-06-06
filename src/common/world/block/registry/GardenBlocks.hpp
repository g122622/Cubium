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
 * @brief 1.21.2+ 花园觉醒植被方块的静态引用
 *
 * 包含眼花、野花、枯草、仙人掌花、萤火虫灌木等新植被。
 */
struct GardenBlocks {
    // 野花
    static Block* WILDFLOWERS;

    // 枯叶
    static Block* LEAF_LITTER;

    // 干草系列
    static Block* SHORT_DRY_GRASS;
    static Block* TALL_DRY_GRASS;

    // 仙人掌花
    static Block* CACTUS_FLOWER;

    // 萤火虫灌木（发光）
    static Block* FIREFLY_BUSH;

    // 灌木
    static Block* BUSH;
};

void registerGardenBlocks();

} // namespace block_registry
} // namespace mc
