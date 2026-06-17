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
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND OF EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/**
 * @file TimeArgumentTest.cpp
 * @brief TimeArgumentType 单元测试
 *
 * 测试时间参数解析，包括：
 * - 纯数字解析（默认 tick 单位）
 * - tick 后缀解析（t）
 * - 秒后缀解析（s）
 * - 天后缀解析（d）
 * - 浮点数输入（如 1.5d）
 * - 最小值校验
 * - 无效单位异常
 * - getTypeName 和 getExamples
 * - 工厂方法
 */

#include <gtest/gtest.h>

#include "common/command/StringReader.hpp"
#include "common/command/arguments/TimeArgument.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"

using mc::command::CommandErrorType;
using mc::command::CommandException;
using mc::command::StringReader;
using mc::command::TimeArgumentType;
using mc::i32;

// ========== 基础解析测试 ==========

class TimeArgumentTypeTest : public ::testing::Test {
protected:
    TimeArgumentType parser{0}; // 默认最小值 0
};

TEST_F(TimeArgumentTypeTest, ParsePureNumber)
{
    // "100" -> 100 tick（无后缀默认为 tick）
    StringReader reader("100");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 100);
    EXPECT_EQ(reader.getCursor(), 3); // 消费了 "100"
}

TEST_F(TimeArgumentTypeTest, ParseTickSuffix)
{
    // "100t" -> 100 tick
    StringReader reader("100t");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 100);
    EXPECT_EQ(reader.getCursor(), 4);
}

TEST_F(TimeArgumentTypeTest, ParseSecondSuffix)
{
    // "5s" -> 5 * 20 = 100 tick
    StringReader reader("5s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 100);
    EXPECT_EQ(reader.getCursor(), 2);
}

TEST_F(TimeArgumentTypeTest, ParseDaySuffix)
{
    // "1d" -> 1 * 24000 = 24000 tick
    StringReader reader("1d");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 24000);
    EXPECT_EQ(reader.getCursor(), 2);
}

TEST_F(TimeArgumentTypeTest, ParseZeroTicks)
{
    // "0" -> 0 tick
    StringReader reader("0");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 0);
}

TEST_F(TimeArgumentTypeTest, ParseZeroSeconds)
{
    // "0s" -> 0 tick
    StringReader reader("0s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 0);
}

TEST_F(TimeArgumentTypeTest, ParseZeroDays)
{
    // "0d" -> 0 tick
    StringReader reader("0d");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 0);
}

// ========== 浮点数输入测试 ==========

TEST_F(TimeArgumentTypeTest, ParseFloatDay)
{
    // "1.5d" -> round(1.5 * 24000) = 36000 tick
    StringReader reader("1.5d");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 36000);
}

TEST_F(TimeArgumentTypeTest, ParseHalfSecond)
{
    // "0.5s" -> round(0.5 * 20) = 10 tick
    StringReader reader("0.5s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 10);
}

TEST_F(TimeArgumentTypeTest, ParseFractionalDay)
{
    // "0.25d" -> round(0.25 * 24000) = 6000 tick
    StringReader reader("0.25d");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 6000);
}

TEST_F(TimeArgumentTypeTest, ParseFloatTick)
{
    // "1.5t" -> round(1.5 * 1) = 2 tick
    StringReader reader("1.5t");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 2);
}

// ========== 最小值校验测试 ==========

TEST_F(TimeArgumentTypeTest, MinimumZeroAllowsZero)
{
    // minimum=0，0 tick 合法
    TimeArgumentType minZero(0);
    StringReader reader("0");
    i32 result = minZero.parse(reader);
    EXPECT_EQ(result, 0);
}

TEST_F(TimeArgumentTypeTest, MinimumOneRejectsZero)
{
    // minimum=1，0 tick 应抛出异常
    TimeArgumentType minOne(1);
    StringReader reader("0");
    EXPECT_THROW({ minOne.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, MinimumOneAllowsOne)
{
    // minimum=1，1 tick 合法
    TimeArgumentType minOne(1);
    StringReader reader("1");
    i32 result = minOne.parse(reader);
    EXPECT_EQ(result, 1);
}

TEST_F(TimeArgumentTypeTest, MinimumOneRejectsZeroSeconds)
{
    // minimum=1，"0s" (0 tick) 应抛出异常
    TimeArgumentType minOne(1);
    StringReader reader("0s");
    EXPECT_THROW({ minOne.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, MinimumOneAllowsOneSecond)
{
    // minimum=1，"1s" (20 tick) 合法
    TimeArgumentType minOne(1);
    StringReader reader("1s");
    i32 result = minOne.parse(reader);
    EXPECT_EQ(result, 20);
}

TEST_F(TimeArgumentTypeTest, MinimumOneRejectsFractionalSecond)
{
    // minimum=1，"0.01s" (round(0.2) = 0 tick) 应抛出异常
    TimeArgumentType minOne(1);
    StringReader reader("0.01s");
    EXPECT_THROW({ minOne.parse(reader); }, CommandException);
}

// ========== 无效输入测试 ==========

TEST_F(TimeArgumentTypeTest, InvalidUnitThrows)
{
    // "5m" -> "m" 不是合法单位
    StringReader reader("5m");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, InvalidUnitXThrows)
{
    // "5x" -> "x" 不是合法单位
    StringReader reader("5x");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, CursorResetOnInvalidUnit)
{
    // 解析失败时应回退游标到起始位置
    StringReader reader("5m");
    i32 startCursor = reader.getCursor();
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
    EXPECT_EQ(reader.getCursor(), startCursor);
}

TEST_F(TimeArgumentTypeTest, EmptyInputThrows)
{
    // 空输入无法读取浮点数
    StringReader reader("");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, LettersOnlyThrows)
{
    // 纯字母无法读取浮点数
    StringReader reader("abc");
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
}

TEST_F(TimeArgumentTypeTest, CursorResetOnInvalidNumber)
{
    // 解析失败时应回退游标到起始位置
    StringReader reader("abc");
    i32 startCursor = reader.getCursor();
    EXPECT_THROW({ parser.parse(reader); }, CommandException);
    EXPECT_EQ(reader.getCursor(), startCursor);
}

// ========== 类型名称和示例 ==========

TEST_F(TimeArgumentTypeTest, GetTypeName)
{
    EXPECT_EQ(parser.getTypeName(), "time");
}

TEST_F(TimeArgumentTypeTest, GetExamples)
{
    auto examples = parser.getExamples();
    EXPECT_EQ(examples.size(), 4u);
    // MC 原版示例: "0d", "0s", "0t", "0"
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0d"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0s"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0t"), examples.end());
    EXPECT_NE(std::find(examples.begin(), examples.end(), "0"), examples.end());
}

// ========== 工厂方法测试 ==========

TEST_F(TimeArgumentTypeTest, FactoryMethodDefault)
{
    auto arg = TimeArgumentType::time();
    ASSERT_NE(arg, nullptr);
    EXPECT_EQ(arg->getTypeName(), "time");
}

TEST_F(TimeArgumentTypeTest, FactoryMethodWithMinimum)
{
    auto arg = TimeArgumentType::time(1);
    ASSERT_NE(arg, nullptr);
    // minimum=1 应拒绝 0 tick
    StringReader reader("0");
    EXPECT_THROW({ arg->parse(reader); }, CommandException);
}

// ========== 序列化元数据测试 ==========

TEST_F(TimeArgumentTypeTest, SerializeMetadataDefault)
{
    TimeArgumentType defaultParser(0);
    auto meta = defaultParser.serializeMetadata();
    ASSERT_TRUE(meta.contains("min"));
    EXPECT_EQ(meta["min"], 0);
}

TEST_F(TimeArgumentTypeTest, SerializeMetadataWithMinimum)
{
    TimeArgumentType minOne(1);
    auto meta = minOne.serializeMetadata();
    ASSERT_TRUE(meta.contains("min"));
    EXPECT_EQ(meta["min"], 1);
}

// ========== 边界值测试 ==========

TEST_F(TimeArgumentTypeTest, LargeDayValue)
{
    // "100d" -> 100 * 24000 = 2400000 tick
    StringReader reader("100d");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 2400000);
}

TEST_F(TimeArgumentTypeTest, LargeSecondValue)
{
    // "1000s" -> 1000 * 20 = 20000 tick
    StringReader reader("1000s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 20000);
}

TEST_F(TimeArgumentTypeTest, RoundingBehavior)
{
    // "0.06s" -> round(0.06 * 20) = round(1.2) = 1 tick
    StringReader reader("0.06s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 1);
}

TEST_F(TimeArgumentTypeTest, RoundingDownBehavior)
{
    // "0.04s" -> round(0.04 * 20) = round(0.8) = 1 tick
    StringReader reader("0.04s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 1);
}

TEST_F(TimeArgumentTypeTest, RoundingZeroBehavior)
{
    // "0.02s" -> round(0.02 * 20) = round(0.4) = 0 tick
    StringReader reader("0.02s");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 0);
}

// ========== 游标位置测试 ==========

TEST_F(TimeArgumentTypeTest, CursorPositionAfterPureNumber)
{
    StringReader reader("100");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 3);
}

TEST_F(TimeArgumentTypeTest, CursorPositionAfterTickSuffix)
{
    StringReader reader("100t");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 4);
}

TEST_F(TimeArgumentTypeTest, CursorPositionAfterSecondSuffix)
{
    StringReader reader("5s");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 2);
}

TEST_F(TimeArgumentTypeTest, CursorPositionAfterDaySuffix)
{
    StringReader reader("1d");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 2);
}

TEST_F(TimeArgumentTypeTest, CursorPositionAfterFloatDay)
{
    StringReader reader("1.5d");
    parser.parse(reader);
    EXPECT_EQ(reader.getCursor(), 4);
}

// ========== 输入带空格的情况 ==========

TEST_F(TimeArgumentTypeTest, StopsAtSpace)
{
    // 输入 "100 extra" 应该只消费 "100"，留下 " extra"
    StringReader reader("100 extra");
    i32 result = parser.parse(reader);
    EXPECT_EQ(result, 100);
    EXPECT_EQ(reader.getCursor(), 3);
    EXPECT_EQ(reader.getRemaining(), " extra");
}
