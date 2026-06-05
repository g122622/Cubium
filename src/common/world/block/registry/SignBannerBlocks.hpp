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
 * @brief 告示牌和旗帜的静态引用
 */
struct SignBannerBlocks {
    // 告示牌（含水）
    static Block* OAK_SIGN;
    static Block* OAK_WALL_SIGN;
    static Block* SPRUCE_SIGN;
    static Block* SPRUCE_WALL_SIGN;
    static Block* BIRCH_SIGN;
    static Block* BIRCH_WALL_SIGN;
    static Block* JUNGLE_SIGN;
    static Block* JUNGLE_WALL_SIGN;
    static Block* ACACIA_SIGN;
    static Block* ACACIA_WALL_SIGN;
    static Block* DARK_OAK_SIGN;
    static Block* DARK_OAK_WALL_SIGN;
    static Block* CRIMSON_SIGN;
    static Block* CRIMSON_WALL_SIGN;
    static Block* WARPED_SIGN;
    static Block* WARPED_WALL_SIGN;

    // 旗帜 (16色 × 2形态 = 32方块)
    static Block* WHITE_BANNER;
    static Block* WHITE_WALL_BANNER;
    static Block* ORANGE_BANNER;
    static Block* ORANGE_WALL_BANNER;
    static Block* MAGENTA_BANNER;
    static Block* MAGENTA_WALL_BANNER;
    static Block* LIGHT_BLUE_BANNER;
    static Block* LIGHT_BLUE_WALL_BANNER;
    static Block* YELLOW_BANNER;
    static Block* YELLOW_WALL_BANNER;
    static Block* LIME_BANNER;
    static Block* LIME_WALL_BANNER;
    static Block* PINK_BANNER;
    static Block* PINK_WALL_BANNER;
    static Block* GRAY_BANNER;
    static Block* GRAY_WALL_BANNER;
    static Block* LIGHT_GRAY_BANNER;
    static Block* LIGHT_GRAY_WALL_BANNER;
    static Block* CYAN_BANNER;
    static Block* CYAN_WALL_BANNER;
    static Block* PURPLE_BANNER;
    static Block* PURPLE_WALL_BANNER;
    static Block* BLUE_BANNER;
    static Block* BLUE_WALL_BANNER;
    static Block* BROWN_BANNER;
    static Block* BROWN_WALL_BANNER;
    static Block* GREEN_BANNER;
    static Block* GREEN_WALL_BANNER;
    static Block* RED_BANNER;
    static Block* RED_WALL_BANNER;
    static Block* BLACK_BANNER;
    static Block* BLACK_WALL_BANNER;
};

void registerSignBannerBlocks();

} // namespace block_registry
} // namespace mc
