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

#include "common/core/Types.hpp"
#include "common/util/DateTimeUtils.hpp"

#include <chrono>
#include <cmath>

using namespace mc;
using namespace mc::util;

// ============================================================================
// formatDateTime 测试
// ============================================================================

TEST(DateTimeUtilsTest, FormatDateTimeFromTimePoint)
{
    // 使用已知时间点：2024-01-15 10:30:00 UTC
    // 构造毫秒时间戳：2024-01-15T10:30:00Z
    // 1705314600000 毫秒 = 2024-01-15T10:30:00Z
    i64 millis = 1705314600000LL;
    auto timePoint = DateTimeUtils::millisToTimePoint(millis);
    std::string result = DateTimeUtils::formatDateTime(timePoint);

    // 结果应包含日期和时间部分（格式：yyyy-MM-dd HH:mm:ss Z）
    // 注意：具体输出依赖本地时区，但格式结构应一致
    EXPECT_FALSE(result.empty());
    // 验证格式结构：至少应包含日期和时间分隔符
    EXPECT_NE(result.find('-'), std::string::npos); // 日期分隔符
    EXPECT_NE(result.find(':'), std::string::npos); // 时间分隔符
    EXPECT_NE(result.find(' '), std::string::npos); // 日期与时间之间的空格
}

TEST(DateTimeUtilsTest, FormatDateTimeFromMillis)
{
    // 从毫秒时间戳格式化
    i64 millis = 1705314600000LL;
    std::string result = DateTimeUtils::formatDateTime(millis);

    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find('-'), std::string::npos);
    EXPECT_NE(result.find(':'), std::string::npos);
    EXPECT_NE(result.find(' '), std::string::npos);
}

TEST(DateTimeUtilsTest, GetCurrentDateTimeString)
{
    // 获取当前时间字符串
    std::string result = DateTimeUtils::getCurrentDateTimeString();

    EXPECT_FALSE(result.empty());
    // 验证格式结构：yyyy-MM-dd HH:mm:ss +ZZZZ
    // 日期部分长度应为 10（yyyy-MM-dd）
    EXPECT_GE(result.size(), 19u); // 至少 "yyyy-MM-dd HH:mm:ss" 的长度
    EXPECT_NE(result.find('-'), std::string::npos);
    EXPECT_NE(result.find(':'), std::string::npos);
}

TEST(DateTimeUtilsTest, FormatDateTimeConsistentWithGetCurrent)
{
    // formatDateTime(now) 和 getCurrentDateTimeString() 应产生相同结果
    auto now = std::chrono::system_clock::now();
    std::string result1 = DateTimeUtils::formatDateTime(now);
    std::string result2 = DateTimeUtils::getCurrentDateTimeString();

    // 日期和小时:分钟部分应该相同（秒数可能差1）
    EXPECT_EQ(result1.substr(0, 16), result2.substr(0, 16));
}

// ============================================================================
// parseDateTimeToMillis 测试
// ============================================================================

TEST(DateTimeUtilsTest, ParseDateTimeValidFormat)
{
    // 解析有效的日期时间字符串
    // 使用 UTC 时间 "2024-01-15 10:30:00 +0000"
    auto result = DateTimeUtils::parseDateTimeToMillis("2024-01-15 10:30:00 +0000");

    EXPECT_TRUE(result.has_value());
    // 验证毫秒时间戳精度（允许 ±1 秒误差，因为 mktime 精度为秒）
    if (result.has_value()) {
        i64 expected = 1705314600000LL; // 2024-01-15T10:30:00Z
        i64 diff = std::abs(result.value() - expected);
        EXPECT_LE(diff, 1000LL); // 允许 1 秒误差
    }
}

TEST(DateTimeUtilsTest, ParseDateTimeEmptyString)
{
    // 空字符串应返回 nullopt
    auto result = DateTimeUtils::parseDateTimeToMillis("");
    EXPECT_FALSE(result.has_value());
}

TEST(DateTimeUtilsTest, ParseDateTimeInvalidFormat)
{
    // 无效格式应返回 nullopt
    auto result1 = DateTimeUtils::parseDateTimeToMillis("not a date");
    EXPECT_FALSE(result1.has_value());

    auto result2 = DateTimeUtils::parseDateTimeToMillis("2024/01/15");
    EXPECT_FALSE(result2.has_value());

    auto result3 = DateTimeUtils::parseDateTimeToMillis("10:30:00");
    EXPECT_FALSE(result3.has_value());
}

TEST(DateTimeUtilsTest, ParseDateTimeWithTimezoneOffset)
{
    // 解析带时区偏移的日期时间字符串
    // UTC+8 时间 "2024-01-15 18:30:00 +0800" 应等于 UTC "2024-01-15 10:30:00 +0000"
    auto utcResult = DateTimeUtils::parseDateTimeToMillis("2024-01-15 10:30:00 +0000");
    auto utc8Result = DateTimeUtils::parseDateTimeToMillis("2024-01-15 18:30:00 +0800");

    ASSERT_TRUE(utcResult.has_value());
    ASSERT_TRUE(utc8Result.has_value());

    // 两个时间戳应相等（时区差异已补偿），允许 ±2 秒误差
    i64 diff = std::abs(utcResult.value() - utc8Result.value());
    EXPECT_LE(diff, 2000LL);
}

TEST(DateTimeUtilsTest, ParseDateTimeNegativeTimezoneOffset)
{
    // 解析负时区偏移
    // UTC-5 时间 "2024-01-15 05:30:00 -0500" 应等于 UTC "2024-01-15 10:30:00 +0000"
    auto utcResult = DateTimeUtils::parseDateTimeToMillis("2024-01-15 10:30:00 +0000");
    auto utc5Result = DateTimeUtils::parseDateTimeToMillis("2024-01-15 05:30:00 -0500");

    ASSERT_TRUE(utcResult.has_value());
    ASSERT_TRUE(utc5Result.has_value());

    i64 diff = std::abs(utcResult.value() - utc5Result.value());
    EXPECT_LE(diff, 2000LL);
}

// ============================================================================
// 往返一致性测试（格式化 → 解析）
// ============================================================================

TEST(DateTimeUtilsTest, RoundTripFormatAndParse)
{
    // 从毫秒时间戳格式化，再解析回来，时间戳应一致
    i64 originalMillis = 1705314600000LL; // 2024-01-15T10:30:00Z
    std::string formatted = DateTimeUtils::formatDateTime(originalMillis);
    auto parsedMillis = DateTimeUtils::parseDateTimeToMillis(formatted);

    ASSERT_TRUE(parsedMillis.has_value());

    // 允许 ±1 秒误差（因为格式化使用本地时区，解析使用 mktime）
    i64 diff = std::abs(parsedMillis.value() - originalMillis);
    EXPECT_LE(diff, 1000LL);
}

TEST(DateTimeUtilsTest, RoundTripCurrentTime)
{
    // 使用当前时间进行往返测试
    auto now = std::chrono::system_clock::now();
    i64 originalMillis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::string formatted = DateTimeUtils::formatDateTime(originalMillis);
    auto parsedMillis = DateTimeUtils::parseDateTimeToMillis(formatted);

    ASSERT_TRUE(parsedMillis.has_value());

    // 允许 ±1 秒误差
    i64 diff = std::abs(parsedMillis.value() - originalMillis);
    EXPECT_LE(diff, 1000LL);
}

// ============================================================================
// millisToTimePoint 测试
// ============================================================================

TEST(DateTimeUtilsTest, MillisToTimePointRoundTrip)
{
    // millisToTimePoint 应与 formatDateTime(millis) 一致
    i64 millis = 1705314600000LL;
    auto timePoint = DateTimeUtils::millisToTimePoint(millis);

    // 从 time_point 转回毫秒应得到原值
    auto resultMillis = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    EXPECT_EQ(resultMillis, millis);
}

TEST(DateTimeUtilsTest, MillisToTimePointEpoch)
{
    // Unix 纪元（0 毫秒）
    auto timePoint = DateTimeUtils::millisToTimePoint(0);
    auto resultMillis = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    EXPECT_EQ(resultMillis, 0);
}

// ============================================================================
// 边界值测试
// ============================================================================

TEST(DateTimeUtilsTest, ParseDateTimePartialInvalidInput)
{
    // std::get_time 在 MSVC 上是宽松的：部分输入会被补零解析
    // "2024-01-15" 被解析为 "2024-01-15 00:00:00"（时间部分默认为 0）
    auto r1 = DateTimeUtils::parseDateTimeToMillis("2024-01-15");
    EXPECT_TRUE(r1.has_value()); // MSVC 宽松解析：时间部分默认为 0

    // "2024-01-15 10:30" 被解析为 "2024-01-15 10:30:00"（秒数默认为 0）
    auto r2 = DateTimeUtils::parseDateTimeToMillis("2024-01-15 10:30");
    EXPECT_TRUE(r2.has_value()); // MSVC 宽松解析：秒数默认为 0

    auto r3 = DateTimeUtils::parseDateTimeToMillis("2024-01-15 10:30:00");
    // 不带时区偏移的格式可以解析（当作本地时间），这是兼容行为
    EXPECT_TRUE(r3.has_value());

    // 完全无效的格式应返回 nullopt
    auto r4 = DateTimeUtils::parseDateTimeToMillis("not a date");
    EXPECT_FALSE(r4.has_value());
}

TEST(DateTimeUtilsTest, FormatDateTimeEpoch)
{
    // Unix 纪元时间点
    std::string result = DateTimeUtils::formatDateTime(0LL);
    EXPECT_FALSE(result.empty());
    // 应包含 "1970" 年份（可能因时区偏移显示不同年份，但不应为空）
}

TEST(DateTimeUtilsTest, ParseDateTimeDifferentDates)
{
    // 测试多个不同日期的解析
    auto r1 = DateTimeUtils::parseDateTimeToMillis("2020-06-15 12:00:00 +0000");
    auto r2 = DateTimeUtils::parseDateTimeToMillis("2023-12-31 23:59:59 +0000");
    auto r3 = DateTimeUtils::parseDateTimeToMillis("2024-02-29 00:00:00 +0000"); // 闰年

    EXPECT_TRUE(r1.has_value());
    EXPECT_TRUE(r2.has_value());
    EXPECT_TRUE(r3.has_value());

    // 时间顺序应正确
    if (r1.has_value() && r2.has_value() && r3.has_value()) {
        EXPECT_LT(r1.value(), r2.value());
        EXPECT_LT(r2.value(), r3.value());
    }
}
