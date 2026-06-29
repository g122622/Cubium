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
 */
namespace DateTimeUtils {

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
 * 使用 _mkgmtime/timegm 避免 mktime 的本地时区和夏令时(DST)干扰。
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
    //   utcTimeT = timegm(tm) - tzOffsetSeconds
    // 使用 timegm/_mkgmtime 避免本地时区和夏令时(DST)干扰
#ifdef _WIN32
    auto utcTimeT = _mkgmtime(&tm);
#else
    // TODO: timegm 非 POSIX 标准，但在 Linux/macOS/BSD 上均可用。
    //      若需移植到不支持 timegm 的平台，需实现基于 TZ 环境变量的替代方案。
    auto utcTimeT = timegm(&tm);
#endif
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
