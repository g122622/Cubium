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
 * @brief 红树林木材系列方块的静态引用（1.19 荒野更新）
 *
 * 红树林生成于红树林沼泽生物群系，是一种新型木材。
 * 所有红树木材使用斧作为有效工具，可燃。
 */
struct MangroveBlocks {
    // 红树原木 - 可剥皮，有轴属性
    static Block* MANGROVE_LOG;

    // 红树木 - 原木的六面 bark 变体，有轴属性
    static Block* MANGROVE_WOOD;

    // 剥皮红树原木 - 有轴属性
    static Block* STRIPPED_MANGROVE_LOG;

    // 剥皮红树木 - 有轴属性
    static Block* STRIPPED_MANGROVE_WOOD;

    // 红树木板 - 基础建筑材料
    static Block* MANGROVE_PLANKS;

    // 红树树叶 - 有距离属性，会腐烂
    static Block* MANGROVE_LEAVES;

    // 红树胎生苗 - 可种植，从红树树叶成熟后掉落
    static Block* MANGROVE_PROPAGULE;

    // 红树根 - 非固体，可透光，可燃
    static Block* MANGROVE_ROOTS;

    // 沾泥红树根 - 有轴属性，泥巴和红树根的混合物
    static Block* MUDDY_MANGROVE_ROOTS;

    // 红树木楼梯
    static Block* MANGROVE_STAIRS;

    // 红树木台阶
    static Block* MANGROVE_SLAB;

    // 红树木栅栏
    static Block* MANGROVE_FENCE;

    // 红树木栅栏门
    static Block* MANGROVE_FENCE_GATE;

    // 红树木门
    static Block* MANGROVE_DOOR;

    // 红树木活板门
    static Block* MANGROVE_TRAPDOOR;
};

void registerMangroveBlocks();

} // namespace block_registry
} // namespace mc
