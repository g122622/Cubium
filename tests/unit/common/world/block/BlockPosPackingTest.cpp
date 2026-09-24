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

// ============================================================================
// BlockPos 位打包回归测试
//
// 原版布局（BlockPos.java:50-58、116-121）：
//   PACKED_HORIZONTAL_LENGTH = 26，PACKED_Y_LENGTH = 12
//   Y_OFFSET = 0，Z_OFFSET = 12，X_OFFSET = 38
//   asLong = (x & 0x3FFFFFF) << 38 | (z & 0x3FFFFFF) << 12 | (y & 0xFFF)
//
// 【为什么单独立测】**y 在最低 12 位、z 在 bit 12..37** 这一点极易写反成
// `y << 26 | z`。写反后打包与解包互不自洽，负坐标会被解成完全错误的值——
// 世界生成的含水层曾因此把负 Z 区块的水层中心解成 704511 这类大正数，
// 使整片海域无水，且没有任何报错。本测试用**字面量位布局**锚定该约定。
// ============================================================================

#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"

#include <tuple>
#include <vector>
#include <gtest/gtest.h>

using namespace mc;

namespace {

/// (x, y, z) 采样：覆盖正负、零、以及各字段的位宽边界
const std::vector<std::tuple<i32, i32, i32>>& packingSamples()
{
    static const std::vector<std::tuple<i32, i32, i32>> samples = {
        {0, 0, 0},
        {1, 2, 3},
        {-1, -2, -3},
        {40, 62, -24},          // 实测过出问题的含水层中心所在区块的坐标
        {-160, 44, -28},        // 负 X + 负 Z
        {33554431, 2047, 33554431},   // 各字段正向上界
        {-33554432, -2048, -33554432}, // 各字段负向下界
        {12345, -6, 67890},
    };
    return samples;
}

constexpr i64 kXZ_MASK = (1LL << 26) - 1;
constexpr i64 kY_MASK = (1LL << 12) - 1;
constexpr i32 kXOffset = 38;
constexpr i32 kZOffset = 12;

} // namespace

/**
 * 打包的**位布局**必须与原版逐位一致：高位是 X、中段是 Z、最低 12 位是 Y。
 *
 * 这是本文件的核心断言——不复用 getXFromLong 等解包函数（那样打包与解包同时写错
 * 也能往返成功，掩盖缺陷），而是直接把 bit 段与原始坐标比较。
 */
TEST(BlockPosPackingTest, BitLayoutMatchesVanilla)
{
    for (const auto& [x, y, z] : packingSamples()) {
        const i64 packed = BlockPos::asLong(x, y, z);
        EXPECT_EQ((packed >> kXOffset) & kXZ_MASK, static_cast<i64>(x) & kXZ_MASK)
            << "X 必须在 bit 38..63：(" << x << "," << y << "," << z << ")";
        EXPECT_EQ((packed >> kZOffset) & kXZ_MASK, static_cast<i64>(z) & kXZ_MASK)
            << "Z 必须在 bit 12..37：(" << x << "," << y << "," << z << ")";
        EXPECT_EQ(packed & kY_MASK, static_cast<i64>(y) & kY_MASK)
            << "Y 必须在最低 12 位：(" << x << "," << y << "," << z << ")";
    }
}

/**
 * 往返：解包必须还原原坐标（含负值——符号扩展正确才成立）。
 */
TEST(BlockPosPackingTest, RoundTripPreservesSignedCoordinates)
{
    for (const auto& [x, y, z] : packingSamples()) {
        const i64 packed = BlockPos::asLong(x, y, z);
        EXPECT_EQ(BlockPos::getXFromLong(packed), x);
        EXPECT_EQ(BlockPos::getYFromLong(packed), y);
        EXPECT_EQ(BlockPos::getZFromLong(packed), z);
    }
}

/**
 * 分工明确：Y 只占 12 位，不会"污染"Z 所在的 bit 12..37。
 *
 * 若把布局误写成 `x << 38 | y << 26 | z`，Y 的位会落到 Z 的区间里，
 * 本用例会直接失败。
 */
TEST(BlockPosPackingTest, YFieldDoesNotBleedIntoZField)
{
    // 只改 y，Z 段与 X 段必须完全不变
    const i64 a = BlockPos::asLong(7, 0, 9);
    const i64 b = BlockPos::asLong(7, 2047, 9);
    EXPECT_EQ(a >> kZOffset, b >> kZOffset) << "改变 Y 不应影响 Z/X 段";
    EXPECT_EQ(a & kY_MASK, 0);
    EXPECT_EQ(b & kY_MASK, 2047);
}
