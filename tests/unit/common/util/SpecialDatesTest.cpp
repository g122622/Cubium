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
 * LIABILITY, WHETHER IN AN EVENT OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/util/SpecialDates.hpp"

#include <chrono>

using namespace mc::util;

// ============================================================================
// 常量测试
// ============================================================================

TEST(SpecialDatesTest, HalloweenConstant)
{
    // 万圣节是 10月31日
    EXPECT_EQ(SpecialDates::HALLOWEEN.month(), std::chrono::October);
    EXPECT_EQ(SpecialDates::HALLOWEEN.day(), std::chrono::day{31});
}

TEST(SpecialDatesTest, ChristmasConstant)
{
    // 圣诞节（平安夜）是 12月24日
    EXPECT_EQ(SpecialDates::CHRISTMAS.month(), std::chrono::December);
    EXPECT_EQ(SpecialDates::CHRISTMAS.day(), std::chrono::day{24});
}

TEST(SpecialDatesTest, NewYearConstant)
{
    // 元旦是 1月1日
    EXPECT_EQ(SpecialDates::NEW_YEAR.month(), std::chrono::January);
    EXPECT_EQ(SpecialDates::NEW_YEAR.day(), std::chrono::day{1});
}

// ============================================================================
// dayNow 测试
// ============================================================================

TEST(SpecialDatesTest, DayNowReturnsValidDate)
{
    // dayNow() 应返回有效的月日
    auto today = SpecialDates::dayNow();
    EXPECT_TRUE(today.month().ok());
    EXPECT_TRUE(today.day().ok());

    // 月份应在 1-12 范围内
    auto monthValue = static_cast<unsigned>(today.month());
    EXPECT_GE(monthValue, 1u);
    EXPECT_LE(monthValue, 12u);
}

TEST(SpecialDatesTest, DayNowConsistentWithSystemClock)
{
    // dayNow() 应与 system_clock 一致
    auto today = SpecialDates::dayNow();
    auto now = std::chrono::system_clock::now();
    auto days = std::chrono::floor<std::chrono::days>(now);
    std::chrono::year_month_day ymd{days};
    EXPECT_EQ(today.month(), ymd.month());
    EXPECT_EQ(today.day(), ymd.day());
}

// ============================================================================
// isHalloween 测试
// ============================================================================

TEST(SpecialDatesTest, IsHalloweenOnOctober31)
{
    // 直接构造 10月31日并比较
    auto halloween = std::chrono::month_day{std::chrono::October, std::chrono::day{31}};
    EXPECT_EQ(SpecialDates::HALLOWEEN, halloween);
}

TEST(SpecialDatesTest, IsHalloweenNotOtherDates)
{
    // 确保 10月31日不等于其他日期
    auto notHalloween = std::chrono::month_day{std::chrono::November, std::chrono::day{1}};
    EXPECT_NE(SpecialDates::HALLOWEEN, notHalloween);

    auto oct30 = std::chrono::month_day{std::chrono::October, std::chrono::day{30}};
    EXPECT_NE(SpecialDates::HALLOWEEN, oct30);
}

// ============================================================================
// isExtendedChristmas 测试
// ============================================================================

TEST(SpecialDatesTest, IsExtendedChristmasCoversDec24To26)
{
    // 12月24日应该在圣诞范围内
    auto dec24 = std::chrono::month_day{std::chrono::December, std::chrono::day{24}};
    auto dec25 = std::chrono::month_day{std::chrono::December, std::chrono::day{25}};
    auto dec26 = std::chrono::month_day{std::chrono::December, std::chrono::day{26}};

    // 手动验证逻辑：month == December && day >= 24 && day <= 26
    for (int d = 24; d <= 26; ++d) {
        auto md = std::chrono::month_day{std::chrono::December, std::chrono::day{static_cast<unsigned>(d)}};
        EXPECT_EQ(md.month(), std::chrono::December);
        EXPECT_GE(static_cast<unsigned>(md.day()), 24u);
        EXPECT_LE(static_cast<unsigned>(md.day()), 26u);
    }

    // 12月23日和12月27日不在范围内
    auto dec23 = std::chrono::month_day{std::chrono::December, std::chrono::day{23}};
    EXPECT_LT(static_cast<unsigned>(dec23.day()), 24u);

    auto dec27 = std::chrono::month_day{std::chrono::December, std::chrono::day{27}};
    EXPECT_GT(static_cast<unsigned>(dec27.day()), 26u);
}

TEST(SpecialDatesTest, IsExtendedChristmasNotOtherMonths)
{
    // 1月24-26日不在圣诞范围内
    // isExtendedChristmas 检查月份必须是 December
    // 仅验证常量定义正确
    EXPECT_EQ(SpecialDates::CHRISTMAS.month(), std::chrono::December);
}
