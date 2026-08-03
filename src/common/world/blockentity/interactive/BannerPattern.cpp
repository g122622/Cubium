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

#include "world/blockentity/interactive/BannerPattern.hpp"
#include "util/assert/AssertAll.hpp"
#include <cstddef>
#include <string>

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 图案哈希名表
 *
 * 索引与 BannerPatternType 枚举值对应
 * MC 1.16.5 使用 2-3 字符的短名作为哈希名
 */
const char* const HASH_NAMES[] = {
    "b",   // Base
    "bl",  // SquareBottomLeft
    "br",  // SquareBottomRight
    "tl",  // SquareTopLeft
    "tr",  // SquareTopRight
    "bs",  // StripeBottom
    "ts",  // StripeTop
    "ls",  // StripeLeft
    "rs",  // StripeRight
    "cs",  // StripeCenter
    "ms",  // StripeMiddle
    "drs", // StripeDownright
    "dls", // StripeDownleft
    "ss",  // StripeSmall
    "cr",  // Cross
    "sc",  // StraightCross
    "bt",  // TriangleBottom
    "tt",  // TriangleTop
    "bts", // TrianglesBottom
    "tts", // TrianglesTop
    "ld",  // DiagonalLeft
    "rd",  // DiagonalRight
    "lud", // DiagonalLeftMirror
    "rud", // DiagonalRightMirror
    "mc",  // CircleMiddle
    "mr",  // RhombusMiddle
    "vh",  // HalfVertical
    "hh",  // HalfHorizontal
    "vhr", // HalfVerticalMirror
    "hhb", // HalfHorizontalMirror
    "bo",  // Border
    "cbo", // CurlyBorder
    "gra", // Gradient
    "gru", // GradientUp
    "bri", // Bricks
    "glb", // Globe
    "cre", // Creeper
    "sku", // Skull
    "flo", // Flower
    "moj", // Mojang
    "pig", // Piglin
    "flw", // Flow
    "gus"  // Guster
};

/**
 * @brief 图案文件名表
 *
 * 索引与 BannerPatternType 枚举值对应
 */
const char* const FILE_NAMES[] = {
    "base",                   // Base
    "square_bottom_left",     // SquareBottomLeft
    "square_bottom_right",    // SquareBottomRight
    "square_top_left",        // SquareTopLeft
    "square_top_right",       // SquareTopRight
    "stripe_bottom",          // StripeBottom
    "stripe_top",             // StripeTop
    "stripe_left",            // StripeLeft
    "stripe_right",           // StripeRight
    "stripe_center",          // StripeCenter
    "stripe_middle",          // StripeMiddle
    "stripe_downright",       // StripeDownright
    "stripe_downleft",        // StripeDownleft
    "small_stripes",          // StripeSmall
    "cross",                  // Cross
    "straight_cross",         // StraightCross
    "triangle_bottom",        // TriangleBottom
    "triangle_top",           // TriangleTop
    "triangles_bottom",       // TrianglesBottom
    "triangles_top",          // TrianglesTop
    "diagonal_left",          // DiagonalLeft
    "diagonal_up_right",      // DiagonalRight
    "diagonal_up_left",       // DiagonalLeftMirror
    "diagonal_right",         // DiagonalRightMirror
    "circle",                 // CircleMiddle
    "rhombus",                // RhombusMiddle
    "half_vertical",          // HalfVertical
    "half_horizontal",        // HalfHorizontal
    "half_vertical_right",    // HalfVerticalMirror
    "half_horizontal_bottom", // HalfHorizontalMirror
    "border",                 // Border
    "curly_border",           // CurlyBorder
    "gradient",               // Gradient
    "gradient_up",            // GradientUp
    "bricks",                 // Bricks
    "globe",                  // Globe
    "creeper",                // Creeper
    "skull",                  // Skull
    "flower",                 // Flower
    "mojang",                 // Mojang
    "piglin",                 // Piglin
    "flow",                   // Flow
    "guster"                  // Guster
};

/**
 * @brief 图案是否需要特殊物品表
 */
constexpr bool HAS_PATTERN_ITEM[] = {
    false, // Base
    false, // SquareBottomLeft
    false, // SquareBottomRight
    false, // SquareTopLeft
    false, // SquareTopRight
    false, // StripeBottom
    false, // StripeTop
    false, // StripeLeft
    false, // StripeRight
    false, // StripeCenter
    false, // StripeMiddle
    false, // StripeDownright
    false, // StripeDownleft
    false, // StripeSmall
    false, // Cross
    false, // StraightCross
    false, // TriangleBottom
    false, // TriangleTop
    false, // TrianglesBottom
    false, // TrianglesTop
    false, // DiagonalLeft
    false, // DiagonalRight
    false, // DiagonalLeftMirror
    false, // DiagonalRightMirror
    false, // CircleMiddle
    false, // RhombusMiddle
    false, // HalfVertical
    false, // HalfHorizontal
    false, // HalfVerticalMirror
    false, // HalfHorizontalMirror
    false, // Border
    false, // CurlyBorder
    false, // Gradient
    false, // GradientUp
    false, // Bricks
    true,  // Globe
    true,  // Creeper
    true,  // Skull
    true,  // Flower
    true,  // Mojang
    true,  // Piglin
    true,  // Flow
    true   // Guster
};

constexpr size_t PATTERN_COUNT = static_cast<size_t>(BannerPatternType::Count);

} // namespace

BannerPatternType BannerPatterns::byHash(const std::string& hashName)
{
    for (size_t i = 0; i < PATTERN_COUNT; ++i) {
        if (hashName == HASH_NAMES[i]) {
            return static_cast<BannerPatternType>(i);
        }
    }
    // 未找到时返回 Base
    return BannerPatternType::Base;
}

std::string BannerPatterns::getHashName(BannerPatternType type)
{
    const size_t index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return HASH_NAMES[index];
}

std::string BannerPatterns::getFileName(BannerPatternType type)
{
    const size_t index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return FILE_NAMES[index];
}

bool BannerPatterns::hasPatternItem(BannerPatternType type)
{
    const size_t index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < PATTERN_COUNT);
    return HAS_PATTERN_ITEM[index];
}

} // namespace blockentity
} // namespace mc
