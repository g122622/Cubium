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
 * @brief file_clock 与 system_clock 之间的时钟差（秒）
 *
 * file_time_type 的 epoch 在不同平台上不同：
 * - MSVC（Windows）：file_clock epoch 为 1601-01-01，system_clock epoch 为 1970-01-01，差 11644473600 秒
 * - libc++（macOS/Linux）：file_clock 与 system_clock 共享 Unix epoch（1970-01-01），差为 0
 *
 * 通过探测两时钟对同一 time_point 的数值差，编译期确定该偏移，
 * 从而在数值层面正确转换 file_time_type <-> system_clock，
 * 避免使用 C++20 std::chrono::clock_cast（部分 libc++ 版本尚未实现）。
 */
inline constexpr i64 kFileClockToSystemOffsetSeconds = []() constexpr {
#if defined(_LIBCPP_VERSION) || defined(__APPLE__)
    // libc++ 与 libstdc++ 现代版本中 file_clock 与 system_clock 共享 Unix epoch
    return 0;
#else
    // MSVC：file_clock epoch 为 1601-01-01（Windows FILETIME）
    return 11644473600LL;
#endif
}();

} // namespace detail

/**
 * @brief 将 file_time_type 转换为 Unix 纪元秒数
 *
 * file_time_type 的 epoch 在不同平台上不同（Windows: 1601-01-01, Unix: 1970-01-01），
 * 直接使用 time_since_epoch() 会产生平台相关的值。
 * 此函数通过数值转换 + 平台偏移补偿，确保输出始终为 Unix 纪元秒数，
 * 不依赖 C++20 std::chrono::clock_cast（部分标准库尚未实现）。
 *
 * @param ft 文件时间戳
 * @return 自 1970-01-01 00:00:00 UTC 以来的秒数
 */
[[nodiscard]] inline i64 fileTimeToUnixSeconds(const std::filesystem::file_time_type& ft)
{
    // 取 file_clock 下的纳秒数值，减去平台偏移后落到 Unix epoch 基准
    const auto fileNs = std::chrono::duration_cast<std::chrono::nanoseconds>(ft.time_since_epoch()).count();
    const auto unixNs =
        fileNs - std::chrono::seconds{detail::kFileClockToSystemOffsetSeconds}.count() * 1'000'000'000LL;
    return unixNs / std::chrono::nanoseconds::period::den;
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
    // 先在 Unix epoch 基准下构造纳秒数值，再补回平台偏移得到 file_clock 数值
    const auto unixNs = unixSeconds * std::chrono::nanoseconds::period::den;
    const auto fileNs =
        unixNs + std::chrono::seconds{detail::kFileClockToSystemOffsetSeconds}.count() * 1'000'000'000LL;
    return std::filesystem::file_time_type{
        std::chrono::duration_cast<std::filesystem::file_time_type::duration>(std::chrono::nanoseconds{fileNs})};
}

} // namespace TimeUtils
} // namespace mc::util
