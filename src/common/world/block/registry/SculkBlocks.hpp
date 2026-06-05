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
 * @brief 幽匿系列方块的静态引用（1.19 荒野更新）
 *
 * 幽匿方块生成于深暗之域生物群系，与监守者机制相关。
 * 幽匿系列方块使用锄作为有效工具。
 */
struct SculkBlocks {
    // 幽匿块 - 生成于深暗之域，会被幽匿催化体蔓延
    static Block* SCULK;

    // 幽匿脉络 - 可放置在方块表面，类似藤蔓
    static Block* SCULK_VEIN;

    // 幽匿催化体 - 生物死亡时生成幽匿，发光等级6
    static Block* SCULK_CATALYST;

    // 幽匿感测体 - 检测振动，可激活幽匿尖啸体
    static Block* SCULK_SENSOR;

    // 校准幽匿感测体 - 可通过红石信号过滤振动频率
    static Block* CALIBRATED_SCULK_SENSOR;

    // 幽匿尖啸体 - 激活后召唤监守者
    static Block* SCULK_SHRIEKER;
};

void registerSculkBlocks();

} // namespace block_registry
} // namespace mc
