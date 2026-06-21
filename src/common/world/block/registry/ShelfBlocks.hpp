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
 *
 */

#pragma once

#include "world/block/Block.hpp"

namespace mc {
namespace block_registry {

/**
 * @brief 书架方块系列（1.21.4+ 木质变体）的静态引用
 *
 * 1.21.4版本新增了12种木质书架变体。
 * 10种主世界木质书架可燃（ignite=30, burn=20），2种下界木质不可燃。
 * 所有书架都不能为附魔台提供附魔能量（与普通书架不同）。
 */
struct ShelfBlocks {
    // 传统木材书架
    static Block* OAK_SHELF;
    static Block* SPRUCE_SHELF;
    static Block* BIRCH_SHELF;
    static Block* JUNGLE_SHELF;
    static Block* ACACIA_SHELF;
    static Block* DARK_OAK_SHELF;

    // 红树木书架
    static Block* MANGROVE_SHELF;

    // 樱花木书架
    static Block* CHERRY_SHELF;

    // 苍白橡木书架
    static Block* PALE_OAK_SHELF;

    // 竹木书架
    static Block* BAMBOO_SHELF;

    // 下界木质书架（不可燃）
    static Block* CRIMSON_SHELF;
    static Block* WARPED_SHELF;
};

void registerShelfBlocks();

} // namespace block_registry
} // namespace mc
