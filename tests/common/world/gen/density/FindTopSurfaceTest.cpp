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

/**
 * @file FindTopSurfaceTest.cpp
 * @brief FindTopSurface 密度函数单元测试
 *
 * 验证 FindTopSurface 的核心算法：
 * 1. 当 upperBound <= lowerBound 时直接返回 lowerBound
 * 2. 从 upperBound 向下逐 cellHeight 步查找第一个 density > 0.0 的 Y
 * 3. 若全部 ≤ 0.0 则返回 lowerBound
 * 4. minValue/maxValue 正确计算
 * 5. mapAll 正确递归
 *
 * 参考 MC 1.21.11: net.minecraft.world.level.levelgen.DensityFunctions.FindTopSurface
 */

#include "common/world/gen/density/DensityFunctions.hpp"
#include <gtest/gtest.h>

namespace mc::world::gen::density {

// ============================================================================
// 辅助：创建一个始终返回固定值的密度函数
// ============================================================================
static std::unique_ptr<DensityFunction> constantFunc(f64 value)
{
    return factory::constant(value);
}

// ============================================================================
// 基础行为测试
// ============================================================================

TEST(FindTopSurfaceTest, ReturnsLowerBoundWhenUpperBoundAtOrBelowLowerBound)
{
    // upperBound = 100.0, lowerBound = 100 → i = floor(100/8)*8 = 96 <= 100，返回 lowerBound
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(100.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), 100, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 100.0);
}

TEST(FindTopSurfaceTest, ReturnsLowerBoundWhenUpperBoundBelowLowerBound)
{
    // upperBound = 50.0, lowerBound = 100 → i = floor(50/8)*8 = 48 <= 100，返回 lowerBound
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(50.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), 100, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 100.0);
}

TEST(FindTopSurfaceTest, FindsFirstPositiveY)
{
    // density 始终 > 0，应返回 i = floor(upperBound/cellHeight)*cellHeight
    // upperBound = 200, cellHeight = 8 → i = 200/8 = 25*8 = 200
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 200.0);
}

TEST(FindTopSurfaceTest, FindsFirstPositiveYWithNonMultipleUpperBound)
{
    // upperBound = 205, cellHeight = 8 → i = floor(205/8)*8 = 25*8 = 200
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(205.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 200.0);
}

// ============================================================================
// 搜索行为测试
// ============================================================================

TEST(FindTopSurfaceTest, ReturnsLowerBoundWhenAllDensityNegative)
{
    // density 始终 < 0，应返回 lowerBound
    auto density = constantFunc(-1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), -64.0);
}

TEST(FindTopSurfaceTest, ReturnsLowerBoundWhenAllDensityZero)
{
    // density 始终 = 0（不 > 0），应返回 lowerBound
    auto density = constantFunc(0.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), -64.0);
}

// ============================================================================
// cellHeight 测试
// ============================================================================

TEST(FindTopSurfaceTest, CellHeightAffectsStepSize)
{
    // cellHeight = 16, upperBound = 200 → i = 200/16 = 12*16 = 192
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 16);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 192.0);
}

TEST(FindTopSurfaceTest, CellHeightOf4)
{
    // cellHeight = 4, upperBound = 200 → i = 200/4 = 50*4 = 200
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 4);

    EXPECT_DOUBLE_EQ(fts->compute(0, 0, 0), 200.0);
}

// ============================================================================
// minValue / maxValue 测试
// ============================================================================

TEST(FindTopSurfaceTest, MinValueIsLowerBound)
{
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->minValue(), -64.0);
}

TEST(FindTopSurfaceTest, MaxValueIsMaxOfLowerBoundAndUpperBoundMax)
{
    // upperBound.maxValue() = 200.0, lowerBound = -64 → maxValue = max(-64, 200) = 200
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_DOUBLE_EQ(fts->maxValue(), 200.0);
}

TEST(FindTopSurfaceTest, MaxValueIsLowerBoundWhenUpperBoundSmaller)
{
    // upperBound.maxValue() = 50.0, lowerBound = 100 → maxValue = max(100, 50) = 100
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(50.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), 100, 8);

    EXPECT_DOUBLE_EQ(fts->maxValue(), 100.0);
}

// ============================================================================
// 访问器测试
// ============================================================================

TEST(FindTopSurfaceTest, AccessorsReturnCorrectValues)
{
    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    FindTopSurface fts(std::move(density), std::move(upperBound), -64, 8);

    EXPECT_EQ(fts.lowerBound(), -64);
    EXPECT_EQ(fts.cellHeight(), 8);
}

// ============================================================================
// mapAll 测试
// ============================================================================

TEST(FindTopSurfaceTest, MapAllIsIdentityForNoOpVisitor)
{
    class IdentityVisitor : public DensityFunction::Visitor {
    public:
        std::unique_ptr<DensityFunction> apply(std::unique_ptr<DensityFunction> func) override { return func; }
    };

    auto density = constantFunc(1.0);
    auto upperBound = constantFunc(200.0);
    auto fts = factory::findTopSurface(std::move(density), std::move(upperBound), -64, 8);

    IdentityVisitor visitor;
    auto mapped = fts->mapAll(visitor);

    // 映射后应返回相同结果
    ASSERT_NE(mapped, nullptr);
    EXPECT_DOUBLE_EQ(mapped->compute(0, 0, 0), 200.0);
    EXPECT_DOUBLE_EQ(mapped->minValue(), -64.0);
    EXPECT_DOUBLE_EQ(mapped->maxValue(), 200.0);
}

} // namespace mc::world::gen::density
