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
 * @brief 足迹与故事版本其他方块（1.20 考古和装饰）的静态引用
 */
struct TrailsBlocks {
    // 可疑方块（考古）
    static Block* SUSPICIOUS_SAND;
    static Block* SUSPICIOUS_GRAVEL;

    // 雕书架
    static Block* CHISELED_BOOKSHELF;

    // 饰纹陶罐
    static Block* DECORATED_POT;

    // 监守者蛋
    static Block* SNIFFER_EGG;

    // 粉红色花瓣
    static Block* PINK_PETALS;

    // 火把花
    static Block* TORCHFLOWER;

    // 瓶草
    static Block* PITCHER_PLANT;

    // 作物
    static Block* TORCHFLOWER_CROP;
    static Block* PITCHER_CROP;
};

void registerTrailsBlocks();

} // namespace block_registry
} // namespace mc
