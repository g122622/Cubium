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

/**
 * @brief 将 file_time_type 转换为 Unix 纪元秒数
 *
 * 使用 C++20 clock_cast 实现跨平台兼容的转换。
 * file_time_type 的 epoch 在不同平台上不同（Windows: 1601-01-01, Unix: 1970-01-01），
 * 直接使用 time_since_epoch() 会产生平台相关的值。
 * 此函数通过 clock_cast 转换到 system_clock，确保输出始终为 Unix 纪元秒数。
 *
 * @param ft 文件时间戳
 * @return 自 1970-01-01 00:00:00 UTC 以来的秒数
 */
[[nodiscard]] inline i64 fileTimeToUnixSeconds(const std::filesystem::file_time_type& ft)
{
    auto sysTime = std::chrono::clock_cast<std::chrono::system_clock>(ft);
    return std::chrono::duration_cast<std::chrono::seconds>(sysTime.time_since_epoch()).count();
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
    auto sysTime = std::chrono::system_clock::from_time_t(static_cast<time_t>(unixSeconds));
    return std::chrono::clock_cast<std::filesystem::file_time_type::clock>(sysTime);
}

} // namespace TimeUtils
} // namespace mc::util
