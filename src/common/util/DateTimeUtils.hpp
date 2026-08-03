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

#pragma once

#include "common/core/Types.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

namespace mc::util {

/**
 * @brief 日期时间格式化和解析工具
 *
 * 提供与 Minecraft Java 版兼容的日期时间格式化和解析功能。
 * MC Java 版使用的日期时间格式为 "yyyy-MM-dd HH:mm:ss Z"，
 * 对应 C/C++ 格式字符串 "%Y-%m-%d %H:%M:%S %z"。
 *
 * 示例输出："2024-06-15 14:30:00 +0800"
 *
 * 时区策略（与 MC Java 版对齐）：
 * - formatDateTime：使用本地时区（localtime_s/localtime_r），对应 MC 的 ZoneId.systemDefault()
 * - parseDateTimeToMillis：将 tm 视为 UTC 解释（自实现的 portableTimegm），
 *   再手动减去字符串携带的时区偏移，得到真实 UTC 时间戳。不依赖本地时区/DST。
 */
namespace DateTimeUtils {

namespace {

/// @brief 单位"天"对应的秒数
inline constexpr i64 SECONDS_PER_DAY = 86400;

/**
 * @brief 将公历日期转换为自 Unix 纪元（1970-01-01）以来的天数
 *
 * 使用 Howard Hinnant 的 days_from_civil 算法（公历转换的标准实现），
 * 完全可移植、无平台依赖、线程安全、无 DST 干扰。
 * 算法正确性：该算法基于 era 的连续 400 年周期，对 proleptic Gregorian calendar
 * （外推公历）精确成立，与 POSIX time_t 的天数学定义一致。
 *
 * @param y 年份（如 2024），允许负值（公元前）
 * @param m 月份，1..12
 * @param d 日，1..31
 * @return 自 1970-01-01 起的天数（可为负）
 */
[[nodiscard]] inline i64 daysFromCivil(i32 y, u32 m, u32 d) noexcept
{
    // 1 月、2 月视为上一年的 13、14 月，使闰日始终落在年末
    y -= m <= 2 ? 1 : 0;
    // era = floor(y / 400)，对负年份也正确
    const i64 era = (y >= 0 ? static_cast<i64>(y) : static_cast<i64>(y) - 399) / 400;
    // yoe = year of era，[0, 399]
    const u32 yoe = static_cast<u32>(static_cast<i64>(y) - era * 400);
    // doy = day of year，[0, 365]；3 月起点使闰日为最后一天
    const u32 doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    // doe = day of era，[0, 146096]
    const u32 doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    // 719468 = 1970-01-01 在 era 0 起点的天数偏移
    return era * 146097 + static_cast<i64>(doe) - 719468;
}

/**
 * @brief 可移植的 timegm 实现
 *
 * 将 struct tm 视为 UTC 时间转换为 time_t（自 Unix 纪元起的秒数）。
 * 完全自包含实现，不依赖 _mkgmtime（Windows）或 timegm（Linux/macOS/BSD）等
 * 平台扩展，也不修改 TZ 环境变量等全局状态，线程安全。
 *
 * 行为对齐 POSIX timegm：
 * - tm_wday/tm_yday 字段被忽略（由年月日重新计算）
 * - tm_isdst 字段被忽略（UTC 无 DST）
 * - 返回 -1 表示溢出或非法输入
 *
 * @param tm 拆分为字段的 UTC 时间（调用后 tm_wday/tm_yday 会被填充）
 * @return 自 Unix 纪元起的秒数，溢出或非法输入返回 -1
 */
[[nodiscard]] inline time_t portableTimegm(std::tm& tm) noexcept
{
    // 规范化输入字段：年月为基础，时分秒叠加
    // tm_year 是相对 1900 的年份，tm_mon 是 0..11
    const i32 year = 1900 + tm.tm_year;
    const u32 month = static_cast<u32>(tm.tm_mon + 1); // 1..12
    const u32 day = static_cast<u32>(tm.tm_mday);      // 1..31

    // 基础输入校验（防止 daysFromCivil 对越界输入产生静默错误结果）
    // 允许 tm_hour/tm_min/tm_sec 超出常规范围以兼容 std::get_time 的宽松解析
    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return -1;
    }

    // 计算自 Unix 纪元起的天数
    const i64 days = daysFromCivil(year, month, day);

    // 叠加时分秒（允许 tm_sec/tm_min/tm_hour 携带进位，对齐 mktime/timegm 的归一化语义）
    const i64 seconds = days * SECONDS_PER_DAY + static_cast<i64>(tm.tm_hour) * 3600 +
        static_cast<i64>(tm.tm_min) * 60 + static_cast<i64>(tm.tm_sec);

    // 填充 tm_wday（0=周日）和 tm_yday（0..365）以对齐 timegm 行为
    // 1970-01-01 是周四 → wday = (days % 7 + 4 + 7) % 7
    const i64 wdayRaw = (days % 7 + 4 + 7) % 7;
    tm.tm_wday = static_cast<i32>(wdayRaw);

    // doy = day - 1 + 前几个月的天数（不考虑闰年的简化版，仅用于信息填充）
    // 为简化实现，使用 days_from_civil 反推：doy = days(y,m,d) - days(y,m,1)
    const i64 doy = days - daysFromCivil(year, month, 1);
    tm.tm_yday = static_cast<i32>(doy);

    // UTC 时间无夏令时
    tm.tm_isdst = 0;

    return static_cast<time_t>(seconds);
}

} // namespace

/// MC Java 版日期时间格式（对应 Java 的 "yyyy-MM-dd HH:mm:ss Z"）
inline constexpr char MC_DATE_FORMAT[] = "%Y-%m-%d %H:%M:%S %z";

/**
 * @brief 将时间点格式化为 MC Java 版日期时间字符串
 *
 * 使用本地时区将 system_clock 时间点格式化为 "yyyy-MM-dd HH:mm:ss Z" 格式，
 * 与 MC Java 版的 DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss Z") 兼容。
 *
 * @param timePoint 系统时钟时间点
 * @return 格式化后的日期时间字符串，如 "2024-06-15 14:30:00 +0800"
 */
[[nodiscard]] inline std::string formatDateTime(std::chrono::system_clock::time_point timePoint)
{
    auto timeT = std::chrono::system_clock::to_time_t(timePoint);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &timeT);
#else
    localtime_r(&timeT, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, MC_DATE_FORMAT);
    return ss.str();
}

/**
 * @brief 将毫秒时间戳格式化为 MC Java 版日期时间字符串
 *
 * @param millisSinceEpoch 自 Unix 纪元以来的毫秒数
 * @return 格式化后的日期时间字符串
 */
[[nodiscard]] inline std::string formatDateTime(i64 millisSinceEpoch)
{
    auto timePoint = std::chrono::system_clock::time_point{
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(millisSinceEpoch))};
    return formatDateTime(timePoint);
}

/**
 * @brief 获取当前时间的 MC Java 版日期时间字符串
 *
 * @return 当前时间的格式化字符串
 */
[[nodiscard]] inline std::string getCurrentDateTimeString()
{
    return formatDateTime(std::chrono::system_clock::now());
}

/**
 * @brief 将 MC Java 版日期时间字符串解析为毫秒时间戳
 *
 * 解析 "yyyy-MM-dd HH:mm:ss Z" 格式的字符串，返回自 Unix 纪元以来的毫秒数。
 * 与 MC Java 版 AdvancementProgress 中 OBTAINED_TIME_CODEC 的解析逻辑兼容。
 *
 * 注意：Windows 的 std::get_time 不支持 %z 时区偏移解析，因此此函数手动解析时区部分。
 * UTC 转换使用自实现的 portableTimegm（基于 Howard Hinnant 算法），
 * 完全可移植、线程安全，避免 mktime 的本地时区和夏令时(DST)干扰，
 * 也不依赖非 POSIX 标准的 timegm 平台扩展。
 *
 * @param dateTimeStr 日期时间字符串，如 "2024-06-15 14:30:00 +0800"
 * @return 解析成功返回毫秒时间戳，解析失败返回 std::nullopt
 */
[[nodiscard]] inline std::optional<i64> parseDateTimeToMillis(const std::string& dateTimeStr)
{
    if (dateTimeStr.empty()) {
        return std::nullopt;
    }

    // MC Java 格式："2024-06-15 14:30:00 +0800"
    // 预期最小长度：19 字符（不含时区），完整格式至少 24 字符
    // 格式：yyyy-MM-dd HH:mm:ss +HHMM 或 -HHMM
    // 先尝试不带时区部分的解析
    std::tm tm{};
    std::istringstream ss(dateTimeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

    if (ss.fail()) {
        return std::nullopt;
    }

    // 手动解析时区偏移部分（"+HHMM" 或 "-HHMM"）
    i64 tzOffsetSeconds = 0;
    std::string remaining;
    if (ss >> remaining) {
        // remaining 应为 "+HHMM" 或 "-HHMM"
        if (remaining.size() >= 5 && (remaining[0] == '+' || remaining[0] == '-')) {
            try {
                int tzHours = std::stoi(remaining.substr(1, 2));
                int tzMinutes = std::stoi(remaining.substr(3, 2));
                tzOffsetSeconds = static_cast<i64>(tzHours) * 3600 + static_cast<i64>(tzMinutes) * 60;
                if (remaining[0] == '-') {
                    tzOffsetSeconds = -tzOffsetSeconds;
                }
            }
            catch (...) {
                // 时区解析失败，忽略时区偏移（使用本地时间）
            }
        }
        // 如果没有时区部分或格式不正确，tzOffsetSeconds 保持为 0
    }

    // 将 tm 视为 UTC 时间转换为 time_t，再减去时区偏移得到真实 UTC 时间戳
    // 字符串时间在 tzOffsetSeconds 时区，因此：
    //   UTC = stringTime - tzOffsetSeconds
    //   utcTimeT = portableTimegm(tm) - tzOffsetSeconds
    // 使用自实现的 portableTimegm 避免 mktime 的本地时区和夏令时(DST)干扰，
    // 同时不依赖非 POSIX 标准的 timegm 平台扩展
    auto utcTimeT = portableTimegm(tm);
    if (utcTimeT == -1) {
        return std::nullopt;
    }

    // 减去字符串中的时区偏移（正偏移表示东时区，需减去以得到 UTC）
    utcTimeT -= tzOffsetSeconds;

    // 转换为毫秒时间戳
    auto timePoint = std::chrono::system_clock::from_time_t(utcTimeT);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    return millis;
}

/**
 * @brief 将毫秒时间戳转换为 system_clock::time_point
 *
 * @param millisSinceEpoch 自 Unix 纪元以来的毫秒数
 * @return 对应的时间点
 */
[[nodiscard]] inline std::chrono::system_clock::time_point millisToTimePoint(i64 millisSinceEpoch)
{
    return std::chrono::system_clock::time_point{
        std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::milliseconds(millisSinceEpoch))};
}

} // namespace DateTimeUtils
} // namespace mc::util
