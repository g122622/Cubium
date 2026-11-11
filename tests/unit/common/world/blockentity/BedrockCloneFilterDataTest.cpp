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
 * @file BedrockCloneFilterDataTest.cpp
 * @brief 基岩 clone filtered 的 data 值 → Java 方块状态属性串
 *
 * 重点覆盖两件事：
 * 1. stairs 的 data 0..7 完整映射（方向 × 上下）；
 * 2. 所有无法翻译的输入都退化成空串——这是刻意的安全回退，不能变成"猜一个状态"。
 */

#include "common/world/blockentity/redstone/BedrockCloneFilterData.hpp"

#include <gtest/gtest.h>

#include <string>

using mc::blockentity::bedrockCloneFilterDataToJavaProps;

namespace {

constexpr const char* STAIRS = "minecraft:stone_stairs";

} // namespace

// data 0..3 是方向（east/west/south/north），下半砖
TEST(BedrockCloneFilterDataTest, StairsLowerHalfMapsDirection)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "0"), "[facing=east,half=bottom]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "1"), "[facing=west,half=bottom]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "2"), "[facing=south,half=bottom]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "3"), "[facing=north,half=bottom]");
}

// data 4..7 是同一组方向，但 bit2 置位 → 上半砖
TEST(BedrockCloneFilterDataTest, StairsUpperHalfSetsTopBit)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "4"), "[facing=east,half=top]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "5"), "[facing=west,half=top]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "6"), "[facing=south,half=top]");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "7"), "[facing=north,half=top]");
}

// 缺失 data：退化为按方块类型过滤
TEST(BedrockCloneFilterDataTest, EmptyDataYieldsEmptyProps)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, ""), "");
}

// 非 stairs 方块：当前没有权威位表，一律退化
TEST(BedrockCloneFilterDataTest, NonStairsBlockYieldsEmptyProps)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps("minecraft:oak_trapdoor", "3"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps("minecraft:stone", "0"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps("", "0"), "");
}

// 超出 0..7 的值没有定义，不能硬套
TEST(BedrockCloneFilterDataTest, OutOfRangeDataYieldsEmptyProps)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "8"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "99"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "-1"), "");
}

// 非法数字串：整串都必须是数字，否则退化。
// 这里特意包含 "3abc"——std::stoi 会把它解析成 3，from_chars + 全消费检查必须拒绝它。
TEST(BedrockCloneFilterDataTest, MalformedDataYieldsEmptyProps)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "3abc"), "")
        << "trailing garbage must be rejected, not silently truncated like std::stoi does";
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "abc"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "3.5"), "");
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, " 3"), "")
        << "leading whitespace is not part of a clean data token";
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps(STAIRS, "+3"), "");
}

// 方块名判定是子串匹配：任何含 stairs 的名字都走台阶映射
TEST(BedrockCloneFilterDataTest, AnyStairsNameIsTreatedAsStairs)
{
    EXPECT_EQ(bedrockCloneFilterDataToJavaProps("stone_stairs", "1"), "[facing=west,half=bottom]");
}
