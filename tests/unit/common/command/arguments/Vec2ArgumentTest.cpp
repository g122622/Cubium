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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file Vec2ArgumentTest.cpp
 * @brief Vec2ArgumentType 单元测试
 *
 * 测试二维向量坐标参数解析，包括：
 * - 绝对坐标解析（两个空格分隔的双精度浮点数）
 * - 相对坐标解析（~ 前缀）
 * - 混合绝对/相对坐标
 * - 负数坐标
 * - 小数坐标
 * - getTypeName 和 getExamples
 * - 工厂方法
 * - 游标位置
 *
 * Vec2ArgumentType::parse() 返回 Coordinates::Ptr（实际为 WorldCoordinates），
 * 其中 x 分量对应水平 x，y 分量填充为 relative(0)，z 分量对应水平 z。
 * 测试通过 dynamic_pointer_cast<WorldCoordinates> 验证各分量。
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/GameModeArgument.hpp"
#include "common/command/coordinates/Coordinates.hpp"
#include "common/command/coordinates/WorldCoordinates.hpp"

using mc::command::Coordinates;
using mc::command::StringReader;
using mc::command::Vec2ArgumentType;
using mc::command::WorldCoordinates;

// ========== 辅助函数 ==========

/**
 * @brief 解析并转换为 WorldCoordinates 指针
 *
 * Vec2ArgumentType::parse() 返回 Coordinates::Ptr，这里转换为 WorldCoordinates*
 * 以便访问各分量。如果转换失败则触发 ASSERT 失败。
 */
static const WorldCoordinates* toWorldCoords(const Coordinates::Ptr& ptr)
{
    auto* wc = dynamic_cast<const WorldCoordinates*>(ptr.get());
    EXPECT_NE(wc, nullptr) << "parse() should return WorldCoordinates";
    return wc;
}

// ========== 基础解析测试 ==========

class Vec2ArgumentTypeTest : public ::testing::Test {
protected:
    Vec2ArgumentType parser;
};

TEST_F(Vec2ArgumentTypeTest, ParseAbsoluteCoordinates)
{
    // "100 -200" -> WorldCoordinates(abs(100), rel(0), abs(-200))
    StringReader reader("100 -200");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 100.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), -200.0);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseZeroCoordinates)
{
    // "0 0" -> WorldCoordinates(abs(0), rel(0), abs(0))
    StringReader reader("0 0");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 0.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), 0.0);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseDecimalCoordinates)
{
    // "0.1 -0.5" -> WorldCoordinates(abs(0.1), rel(0), abs(-0.5))
    StringReader reader("0.1 -0.5");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 0.1);
    EXPECT_DOUBLE_EQ(wc->z().value(), -0.5);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseNegativeCoordinates)
{
    // "-50.5 -100.7" -> WorldCoordinates(abs(-50.5), rel(0), abs(-100.7))
    StringReader reader("-50.5 -100.7");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), -50.5);
    EXPECT_DOUBLE_EQ(wc->z().value(), -100.7);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

// ========== 相对坐标解析测试 ==========

TEST_F(Vec2ArgumentTypeTest, ParseTildeTilde)
{
    // "~ ~" -> WorldCoordinates(rel(0), rel(0), rel(0))
    StringReader reader("~ ~");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 0.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), 0.0);
    EXPECT_TRUE(wc->isXRelative());
    EXPECT_TRUE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseTildeWithOffset)
{
    // "~1 ~-2" -> WorldCoordinates(rel(1), rel(0), rel(-2))
    StringReader reader("~1 ~-2");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 1.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), -2.0);
    EXPECT_TRUE(wc->isXRelative());
    EXPECT_TRUE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseTildeWithDecimalOffset)
{
    // "~1.5 ~-0.5" -> WorldCoordinates(rel(1.5), rel(0), rel(-0.5))
    StringReader reader("~1.5 ~-0.5");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 1.5);
    EXPECT_DOUBLE_EQ(wc->z().value(), -0.5);
    EXPECT_TRUE(wc->isXRelative());
    EXPECT_TRUE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseMixedAbsoluteAndRelative)
{
    // "100 ~5" -> WorldCoordinates(abs(100), rel(0), rel(5))
    StringReader reader("100 ~5");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 100.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), 5.0);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_TRUE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseTildeAloneThenNumber)
{
    // "~ 50" -> WorldCoordinates(rel(0), rel(0), abs(50))
    StringReader reader("~ 50");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 0.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), 50.0);
    EXPECT_TRUE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

// ========== 类型名称和示例 ==========

TEST_F(Vec2ArgumentTypeTest, GetTypeName)
{
    EXPECT_EQ(parser.getTypeName(), "vec2");
}

TEST_F(Vec2ArgumentTypeTest, GetExamples)
{
    auto examples = parser.getExamples();
    EXPECT_EQ(examples.size(), 4u);
    // MC 原版示例: "0 0", "~ ~", "0.1 -0.5", "~1 ~-2"
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0 0"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "~ ~"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0.1 -0.5"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "~1 ~-2"), examples.end());
}

// ========== 工厂方法测试 ==========

TEST_F(Vec2ArgumentTypeTest, FactoryMethod)
{
    auto arg = Vec2ArgumentType::vec2();
    ASSERT_NE(arg, nullptr);
    EXPECT_EQ(arg->getTypeName(), "vec2");
}

// ========== 游标位置测试 ==========

TEST_F(Vec2ArgumentTypeTest, CursorPositionAfterAbsolute)
{
    // "100 -200" 消费全部 8 个字符
    StringReader reader("100 -200");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 8);
}

TEST_F(Vec2ArgumentTypeTest, CursorPositionAfterTilde)
{
    // "~1 ~-2" 消费全部 6 个字符
    StringReader reader("~1 ~-2");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 6);
}

TEST_F(Vec2ArgumentTypeTest, CursorPositionAfterTildeAlone)
{
    // "~ ~" 消费 3 个字符
    StringReader reader("~ ~");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 3);
}

// ========== 边界值测试 ==========

TEST_F(Vec2ArgumentTypeTest, ParseLargeCoordinates)
{
    // 大坐标值
    StringReader reader("30000000 -30000000");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 30000000.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), -30000000.0);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

TEST_F(Vec2ArgumentTypeTest, ParseVerySmallDecimals)
{
    // 极小小数
    StringReader reader("0.001 -0.001");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 0.001);
    EXPECT_DOUBLE_EQ(wc->z().value(), -0.001);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

// ========== 多空格分隔测试 ==========

TEST_F(Vec2ArgumentTypeTest, TabSeparatedCoordinates)
{
    // Tab 分隔也可以被 skipWhitespace 处理
    StringReader reader("100\t200");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->x().value(), 100.0);
    EXPECT_DOUBLE_EQ(wc->z().value(), 200.0);
    EXPECT_FALSE(wc->isXRelative());
    EXPECT_FALSE(wc->isZRelative());
}

// ========== Y 分量测试 ==========

TEST_F(Vec2ArgumentTypeTest, YComponentIsAlwaysRelativeZero)
{
    // Vec2ArgumentType 将 y 分量填充为 relative(0)，无论输入如何
    StringReader reader("100 -200");
    auto result = parser.parse(reader);
    auto* wc = toWorldCoords(result);
    ASSERT_NE(wc, nullptr);
    EXPECT_DOUBLE_EQ(wc->y().value(), 0.0);
    EXPECT_TRUE(wc->isYRelative());
}
