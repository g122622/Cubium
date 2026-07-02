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
 * @brief 蜡烛方块和蜡烛蛋糕方块的静态引用
 *
 * 包含17种蜡烛（无色+16色）和对应的17种蜡烛蛋糕方块。
 * 蜡烛可以堆叠1-4根并点燃/熄灭；蜡烛蛋糕是蜡烛插在蛋糕上的变体。
 */
struct CandleBlocks {
    // 蜡烛方块
    static Block* CANDLE;
    static Block* WHITE_CANDLE;
    static Block* ORANGE_CANDLE;
    static Block* MAGENTA_CANDLE;
    static Block* LIGHT_BLUE_CANDLE;
    static Block* YELLOW_CANDLE;
    static Block* LIME_CANDLE;
    static Block* PINK_CANDLE;
    static Block* GRAY_CANDLE;
    static Block* LIGHT_GRAY_CANDLE;
    static Block* CYAN_CANDLE;
    static Block* PURPLE_CANDLE;
    static Block* BLUE_CANDLE;
    static Block* BROWN_CANDLE;
    static Block* GREEN_CANDLE;
    static Block* RED_CANDLE;
    static Block* BLACK_CANDLE;

    // 蜡烛蛋糕方块
    static Block* CANDLE_CAKE;
    static Block* WHITE_CANDLE_CAKE;
    static Block* ORANGE_CANDLE_CAKE;
    static Block* MAGENTA_CANDLE_CAKE;
    static Block* LIGHT_BLUE_CANDLE_CAKE;
    static Block* YELLOW_CANDLE_CAKE;
    static Block* LIME_CANDLE_CAKE;
    static Block* PINK_CANDLE_CAKE;
    static Block* GRAY_CANDLE_CAKE;
    static Block* LIGHT_GRAY_CANDLE_CAKE;
    static Block* CYAN_CANDLE_CAKE;
    static Block* PURPLE_CANDLE_CAKE;
    static Block* BLUE_CANDLE_CAKE;
    static Block* BROWN_CANDLE_CAKE;
    static Block* GREEN_CANDLE_CAKE;
    static Block* RED_CANDLE_CAKE;
    static Block* BLACK_CANDLE_CAKE;
};

void registerCandleBlocks();

} // namespace block_registry
} // namespace mc
