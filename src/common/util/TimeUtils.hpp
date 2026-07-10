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
#include <filesystem>

namespace mc::util {

/**
 * @brief 时间工具函数
 */
namespace TimeUtils {

/**
 * @brief 获取当前时间戳（毫秒）
 * @return 自 epoch 以来的毫秒数
 */
[[nodiscard]] inline u64 getCurrentTimeMs()
{
    return static_cast<u64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

/**
 * @brief 获取当前时间戳（微秒）
 * @return 自 epoch 以来的微秒数
 */
[[nodiscard]] inline u64 getCurrentTimeUs()
{
    return static_cast<u64>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

namespace detail {

/**
 * @brief 是否可用 C++20 std::chrono::clock_cast 做时钟转换
 *
 * MSVC 14.4+、libstdc++ 12+、libc++ 17+ 均已实现 clock_cast。
 * 老版 libc++（macOS 系统库）可能缺失，此时走秒级手工回退路径。
 */
inline constexpr bool kHasClockCast =
#if defined(_MSC_VER)
    true;
#elif defined(_LIBCPP_VERSION)
    (_LIBCPP_VERSION >= 17000);
#else
    true;
#endif

/**
 * @brief file_clock 与 system_clock 的时钟差（秒），仅在无 clock_cast 的回退路径使用
 *
 * file_time_type 的 epoch 在不同平台上不同：
 * - libc++（macOS）/libstdc++ 现代版本：file_clock 与 system_clock 共享 Unix epoch（1970-01-01），差为 0
 * - MSVC：file_clock epoch 为 1601-01-01（Windows FILETIME），差 11644473600 秒
 *
 * 注意：offset 必须在秒级参与运算，禁止 offset*1e9 放入 i64（会溢出）。
 */
inline constexpr i64 kFileClockToSystemOffsetSeconds = []() constexpr {
#if defined(_LIBCPP_VERSION) || defined(__APPLE__)
    return 0;
#else
    return 11644473600LL;
#endif
}();

} // namespace detail

/**
 * @brief 将 file_time_type 转换为 Unix 纪元秒数
 *
 * 优先用 C++20 std::chrono::clock_cast（标准、无溢出、正确处理闰秒），
 * 老旧 libc++ 无 clock_cast 时回退到秒级手工补偿（避免纳秒级 i64 溢出）。
 *
 * @param ft 文件时间戳
 * @return 自 1970-01-01 00:00:00 UTC 以来的秒数
 */
[[nodiscard]] inline i64 fileTimeToUnixSeconds(const std::filesystem::file_time_type& ft)
{
    if constexpr (detail::kHasClockCast) {
        // 标准路径：file_clock -> system_clock，取秒。库内部用更高精度类型，无溢出。
        const auto sysTp = std::chrono::clock_cast<std::chrono::system_clock>(ft);
        return static_cast<i64>(std::chrono::duration_cast<std::chrono::seconds>(sysTp.time_since_epoch()).count());
    } else {
        // 回退路径：在秒级做 offset 补偿。file_clock 秒值（2026 年约 1.34e10）在 i64 范围内安全。
        const auto fileSecs = std::chrono::duration_cast<std::chrono::seconds>(ft.time_since_epoch()).count();
        return fileSecs - detail::kFileClockToSystemOffsetSeconds;
    }
}

/**
 * @brief 将 Unix 纪元秒数转换为 file_time_type
 *
 * fileTimeToUnixSeconds 的逆操作，用于从 JSON 等序列化格式恢复时间戳。
 *
 * @param unixSeconds 自 1970-01-01 00:00:00 UTC 以来的秒数
 * @return 对应的文件时间戳
 */
[[nodiscard]] inline std::filesystem::file_time_type unixSecondsToFileTime(i64 unixSeconds)
{
    if constexpr (detail::kHasClockCast) {
        // 标准路径：system_clock -> file_clock
        const auto sysTp = std::chrono::system_clock::time_point{std::chrono::seconds{unixSeconds}};
        return std::chrono::clock_cast<std::filesystem::file_time_type::clock>(sysTp);
    } else {
        // 回退路径：秒级补偿后构造 file_time_type，避免纳秒级溢出
        const auto fileSecs = std::chrono::seconds{unixSeconds + detail::kFileClockToSystemOffsetSeconds};
        return std::filesystem::file_time_type{
            std::chrono::duration_cast<std::filesystem::file_time_type::duration>(fileSecs)};
    }
}

} // namespace TimeUtils
} // namespace mc::util
