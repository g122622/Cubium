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

#include <gtest/gtest.h>

#include "common/util/Direction.hpp"

#include <cmath>
#include <limits>

using namespace mc;

// yaw → 水平方向：对齐原版 Direction.fromYRot（0=南，90=东，180=北，270/-90=西）
TEST(DirectionUtilsTest, FromYRot_MapsCardinalAngles)
{
    EXPECT_EQ(Directions::fromYRot(0.0), Direction::South);
    EXPECT_EQ(Directions::fromYRot(90.0), Direction::East);
    EXPECT_EQ(Directions::fromYRot(180.0), Direction::North);
    EXPECT_EQ(Directions::fromYRot(270.0), Direction::West);
    EXPECT_EQ(Directions::fromYRot(-90.0), Direction::West);
    EXPECT_EQ(Directions::fromYRot(-180.0), Direction::North);
    EXPECT_EQ(Directions::fromYRot(360.0), Direction::South);
    EXPECT_EQ(Directions::fromYRot(450.0), Direction::East);
}

// 象限边界按 floor(yaw/90 + 0.5) 四舍五入：±45 度处向高档位取整（与原版 & 3 一致）
TEST(DirectionUtilsTest, FromYRot_RoundsAtQuadrantBoundaries)
{
    EXPECT_EQ(Directions::fromYRot(44.9), Direction::South);
    EXPECT_EQ(Directions::fromYRot(45.0), Direction::East);
    EXPECT_EQ(Directions::fromYRot(134.9), Direction::East);
    EXPECT_EQ(Directions::fromYRot(135.0), Direction::North);
    EXPECT_EQ(Directions::fromYRot(224.9), Direction::North);
    EXPECT_EQ(Directions::fromYRot(225.0), Direction::West);
    EXPECT_EQ(Directions::fromYRot(314.9), Direction::West);
    EXPECT_EQ(Directions::fromYRot(315.0), Direction::South);
    EXPECT_EQ(Directions::fromYRot(-45.0), Direction::South);
    EXPECT_EQ(Directions::fromYRot(-135.0), Direction::West);
}

// toYRot 与 fromYRot 互逆：方向 → 偏航角 → 方向
TEST(DirectionUtilsTest, ToYRot_IsInverseOfFromYRot)
{
    EXPECT_FLOAT_EQ(Directions::toYRot(Direction::South), 0.0f);
    EXPECT_FLOAT_EQ(Directions::toYRot(Direction::East), 90.0f);
    EXPECT_FLOAT_EQ(Directions::toYRot(Direction::North), 180.0f);
    EXPECT_FLOAT_EQ(Directions::toYRot(Direction::West), 270.0f);

    for (Direction dir : {Direction::South, Direction::East, Direction::North, Direction::West}) {
        EXPECT_EQ(Directions::fromYRot(static_cast<f64>(Directions::toYRot(dir))), dir);
    }
}

// isFacingAngle：视线方向与指定方向的点积为正。视线单位向量为 (−sin yaw, 0, cos yaw)，
// 即 yaw=0 朝南、90 朝西、180 朝北、−90 朝东。
TEST(DirectionUtilsTest, IsFacingAngle_MatchesLookDirection)
{
    // yaw=0 朝南：面向 South
    EXPECT_TRUE(Directions::isFacingAngle(Direction::South, 0.0f));
    EXPECT_FALSE(Directions::isFacingAngle(Direction::North, 0.0f));
    // yaw=90 朝西
    EXPECT_TRUE(Directions::isFacingAngle(Direction::West, 90.0f));
    EXPECT_FALSE(Directions::isFacingAngle(Direction::East, 90.0f));
    // yaw=-90（等价 270）朝东
    EXPECT_TRUE(Directions::isFacingAngle(Direction::East, -90.0f));
    EXPECT_FALSE(Directions::isFacingAngle(Direction::West, -90.0f));
    // 竖直方向永远不"面向"
    EXPECT_FALSE(Directions::isFacingAngle(Direction::Up, 0.0f));
    EXPECT_FALSE(Directions::isFacingAngle(Direction::Down, 0.0f));
}
