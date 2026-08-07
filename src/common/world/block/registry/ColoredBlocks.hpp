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
 * @brief 染色方块（羊毛、地毯、染色玻璃、混凝土、陶瓦、潜影盒）的静态引用
 */
struct ColoredBlocks {
    // 羊毛 (16色)
    static Block* WHITE_WOOL;
    static Block* ORANGE_WOOL;
    static Block* MAGENTA_WOOL;
    static Block* LIGHT_BLUE_WOOL;
    static Block* YELLOW_WOOL;
    static Block* LIME_WOOL;
    static Block* PINK_WOOL;
    static Block* GRAY_WOOL;
    static Block* LIGHT_GRAY_WOOL;
    static Block* CYAN_WOOL;
    static Block* PURPLE_WOOL;
    static Block* BLUE_WOOL;
    static Block* BROWN_WOOL;
    static Block* GREEN_WOOL;
    static Block* RED_WOOL;
    static Block* BLACK_WOOL;

    // 地毯 (16色)
    static Block* WHITE_CARPET;
    static Block* ORANGE_CARPET;
    static Block* MAGENTA_CARPET;
    static Block* LIGHT_BLUE_CARPET;
    static Block* YELLOW_CARPET;
    static Block* LIME_CARPET;
    static Block* PINK_CARPET;
    static Block* GRAY_CARPET;
    static Block* LIGHT_GRAY_CARPET;
    static Block* CYAN_CARPET;
    static Block* PURPLE_CARPET;
    static Block* BLUE_CARPET;
    static Block* BROWN_CARPET;
    static Block* GREEN_CARPET;
    static Block* RED_CARPET;
    static Block* BLACK_CARPET;

    // 染色玻璃 (16色)
    static Block* WHITE_STAINED_GLASS;
    static Block* ORANGE_STAINED_GLASS;
    static Block* MAGENTA_STAINED_GLASS;
    static Block* LIGHT_BLUE_STAINED_GLASS;
    static Block* YELLOW_STAINED_GLASS;
    static Block* LIME_STAINED_GLASS;
    static Block* PINK_STAINED_GLASS;
    static Block* GRAY_STAINED_GLASS;
    static Block* LIGHT_GRAY_STAINED_GLASS;
    static Block* CYAN_STAINED_GLASS;
    static Block* PURPLE_STAINED_GLASS;
    static Block* BLUE_STAINED_GLASS;
    static Block* BROWN_STAINED_GLASS;
    static Block* GREEN_STAINED_GLASS;
    static Block* RED_STAINED_GLASS;
    static Block* BLACK_STAINED_GLASS;

    // 混凝土 (16色)
    static Block* WHITE_CONCRETE;
    static Block* ORANGE_CONCRETE;
    static Block* MAGENTA_CONCRETE;
    static Block* LIGHT_BLUE_CONCRETE;
    static Block* YELLOW_CONCRETE;
    static Block* LIME_CONCRETE;
    static Block* PINK_CONCRETE;
    static Block* GRAY_CONCRETE;
    static Block* LIGHT_GRAY_CONCRETE;
    static Block* CYAN_CONCRETE;
    static Block* PURPLE_CONCRETE;
    static Block* BLUE_CONCRETE;
    static Block* BROWN_CONCRETE;
    static Block* GREEN_CONCRETE;
    static Block* RED_CONCRETE;
    static Block* BLACK_CONCRETE;

    // 混凝土粉末 (16色)
    static Block* WHITE_CONCRETE_POWDER;
    static Block* ORANGE_CONCRETE_POWDER;
    static Block* MAGENTA_CONCRETE_POWDER;
    static Block* LIGHT_BLUE_CONCRETE_POWDER;
    static Block* YELLOW_CONCRETE_POWDER;
    static Block* LIME_CONCRETE_POWDER;
    static Block* PINK_CONCRETE_POWDER;
    static Block* GRAY_CONCRETE_POWDER;
    static Block* LIGHT_GRAY_CONCRETE_POWDER;
    static Block* CYAN_CONCRETE_POWDER;
    static Block* PURPLE_CONCRETE_POWDER;
    static Block* BLUE_CONCRETE_POWDER;
    static Block* BROWN_CONCRETE_POWDER;
    static Block* GREEN_CONCRETE_POWDER;
    static Block* RED_CONCRETE_POWDER;
    static Block* BLACK_CONCRETE_POWDER;

    // 陶瓦 (16色 + 普通)
    static Block* WHITE_TERRACOTTA;
    static Block* ORANGE_TERRACOTTA;
    static Block* MAGENTA_TERRACOTTA;
    static Block* LIGHT_BLUE_TERRACOTTA;
    static Block* YELLOW_TERRACOTTA;
    static Block* LIME_TERRACOTTA;
    static Block* PINK_TERRACOTTA;
    static Block* GRAY_TERRACOTTA;
    static Block* LIGHT_GRAY_TERRACOTTA;
    static Block* CYAN_TERRACOTTA;
    static Block* PURPLE_TERRACOTTA;
    static Block* BLUE_TERRACOTTA;
    static Block* BROWN_TERRACOTTA;
    static Block* GREEN_TERRACOTTA;
    static Block* RED_TERRACOTTA;
    static Block* BLACK_TERRACOTTA;
    static Block* TERRACOTTA;

    // 釉面陶瓦 (16色，可旋转、不可被活塞拉动)
    // TODO: 16 色 GlazedTerracottaBlock 此前仅有类定义未注册，导致基岩 .mcstructure 结构
    // (如 minecraft-gametests 的 clone_command) 中的 purple_glazed_terracotta 解析退化为 air，
    // 进而使依附其上的按钮在形状更新阶段自毁。本期补齐注册以打通 GameTest。
    static Block* WHITE_GLAZED_TERRACOTTA;
    static Block* ORANGE_GLAZED_TERRACOTTA;
    static Block* MAGENTA_GLAZED_TERRACOTTA;
    static Block* LIGHT_BLUE_GLAZED_TERRACOTTA;
    static Block* YELLOW_GLAZED_TERRACOTTA;
    static Block* LIME_GLAZED_TERRACOTTA;
    static Block* PINK_GLAZED_TERRACOTTA;
    static Block* GRAY_GLAZED_TERRACOTTA;
    static Block* LIGHT_GRAY_GLAZED_TERRACOTTA;
    static Block* CYAN_GLAZED_TERRACOTTA;
    static Block* PURPLE_GLAZED_TERRACOTTA;
    static Block* BLUE_GLAZED_TERRACOTTA;
    static Block* BROWN_GLAZED_TERRACOTTA;
    static Block* GREEN_GLAZED_TERRACOTTA;
    static Block* RED_GLAZED_TERRACOTTA;
    static Block* BLACK_GLAZED_TERRACOTTA;

    // 床 (16色)
    static Block* WHITE_BED;
    static Block* ORANGE_BED;
    static Block* MAGENTA_BED;
    static Block* LIGHT_BLUE_BED;
    static Block* YELLOW_BED;
    static Block* LIME_BED;
    static Block* PINK_BED;
    static Block* GRAY_BED;
    static Block* LIGHT_GRAY_BED;
    static Block* CYAN_BED;
    static Block* PURPLE_BED;
    static Block* BLUE_BED;
    static Block* BROWN_BED;
    static Block* GREEN_BED;
    static Block* RED_BED;
    static Block* BLACK_BED;

    // 潜影盒 (16色)
    static Block* WHITE_SHULKER_BOX;
    static Block* ORANGE_SHULKER_BOX;
    static Block* MAGENTA_SHULKER_BOX;
    static Block* LIGHT_BLUE_SHULKER_BOX;
    static Block* YELLOW_SHULKER_BOX;
    static Block* LIME_SHULKER_BOX;
    static Block* PINK_SHULKER_BOX;
    static Block* GRAY_SHULKER_BOX;
    static Block* LIGHT_GRAY_SHULKER_BOX;
    static Block* CYAN_SHULKER_BOX;
    static Block* PURPLE_SHULKER_BOX;
    static Block* BLUE_SHULKER_BOX;
    static Block* BROWN_SHULKER_BOX;
    static Block* GREEN_SHULKER_BOX;
    static Block* RED_SHULKER_BOX;
    static Block* BLACK_SHULKER_BOX;
};

void registerColoredBlocks();

} // namespace block_registry
} // namespace mc
