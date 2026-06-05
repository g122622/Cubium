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
 * @brief 1.19 荒野更新其他方块的静态引用
 *
 * 包含蛙明灯、蛙卵等不属于其他类别的1.19新增方块。
 */
struct WildBlocks {
    // 赭黄蛙明灯 - 发光等级15，有轴属性
    static Block* OCHRE_FROGLIGHT;

    // 青翠蛙明灯 - 发光等级15，有轴属性
    static Block* VERDANT_FROGLIGHT;

    // 珠光蛙明灯 - 发光等级15，有轴属性
    static Block* PEARLESCENT_FROGLIGHT;

    // 蛙卵 - 非固体，无碰撞，青蛙产卵后生成
    static Block* FROGSPAWN;
};

void registerWildBlocks();

} // namespace block_registry
} // namespace mc
