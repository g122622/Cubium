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
#include <limits>
#include <utility>

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
 * @brief 读取某时钟"整秒部分 + 秒内余数（纳秒）"，两个分量都保证不溢出
 *
 * 不能把 time_since_epoch 直接 duration_cast 到纳秒：MSVC 的 file_clock epoch 为
 * 1601-01-01，2026 年距该点约 1.34e10 秒，换算成纳秒为 1.34e19，已超出 i64
 * （上限约 9.22e18）。这里先取整秒（值约 1.34e10，安全），余数必然落在 [0, 1s)，
 * 其纳秒值必然小于 1e9，同样安全。
 */
template <typename Clock>
[[nodiscard]] std::pair<i64, i64> secondsAndFractionOfNow()
{
    const auto sinceEpoch = Clock::now().time_since_epoch();
    const auto wholeSeconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
    const auto remainder = sinceEpoch - wholeSeconds;
    const auto remainderNanos = std::chrono::duration_cast<std::chrono::nanoseconds>(remainder).count();
    return {static_cast<i64>(wholeSeconds.count()), static_cast<i64>(remainderNanos)};
}

/**
 * @brief file_clock 与 system_clock 的 epoch 差（秒），即 fileEpoch - unixEpoch
 *
 * 不使用平台宏猜测，而是在运行时实测：同一时刻读两个时钟，其读数之差就是 epoch 差。
 * 平台宏猜测此前是错的——注释写明"libc++/libstdc++ 现代版本 epoch 与 Unix 相同（差为 0），
 * 仅 MSVC 为 1601"，但代码分支只对 `_LIBCPP_VERSION`/`__APPLE__` 返回 0，libstdc++
 * 落入了 `#else` 返回 11644473600。后果是 Linux 上 fileTimeToUnixSeconds 对真实文件
 * 时间戳一律少算 11644473600 秒，且 unixSecondsToFileTime 在秒→纳秒换算时溢出
 * （UBSan: signed integer overflow: 11644473600 * 1000000000）。
 *
 * 实测法对所有平台与标准库实现都成立：真实 epoch 差恒为整数秒，两次 now() 之间的
 * 微秒级调用间隔由"秒内余数之差四舍五入"消除。
 */
[[nodiscard]] inline i64 fileClockToSystemOffsetSeconds()
{
    static const i64 s_offset = []() -> i64 {
        const auto [fileSeconds, fileNanos] = secondsAndFractionOfNow<std::filesystem::file_time_type::clock>();
        const auto [sysSeconds, sysNanos] = secondsAndFractionOfNow<std::chrono::system_clock>();

        // 整秒差 + 秒内余数差的四舍五入（余数差落在 (-1s, 1s)，只会是 -1/0/+1）
        i64 offset = fileSeconds - sysSeconds;
        const i64 nanosDelta = fileNanos - sysNanos;
        if (nanosDelta >= 500000000LL) {
            ++offset;
        } else if (nanosDelta <= -500000000LL) {
            --offset;
        }
        return offset;
    }();
    return s_offset;
}

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
#if defined(__cpp_lib_clock_cast)
    // 标准路径：file_clock -> system_clock，取秒。库内部用更高精度类型，无溢出。
    const auto sysTp = std::chrono::clock_cast<std::chrono::system_clock>(ft);
    return static_cast<i64>(std::chrono::duration_cast<std::chrono::seconds>(sysTp.time_since_epoch()).count());
#else
    // 回退路径：在秒级做 offset 补偿。file_clock 秒值（2026 年约 1.34e10）在 i64 范围内安全。
    const auto fileSecs = std::chrono::duration_cast<std::chrono::seconds>(ft.time_since_epoch()).count();
    return fileSecs - detail::fileClockToSystemOffsetSeconds();
#endif
}

/**
 * @brief 将 Unix 纪元秒数转换为 file_time_type
 *
 * fileTimeToUnixSeconds 的逆操作，用于从 JSON 等序列化格式恢复时间戳。
 *
 * 回退路径（无 clock_cast）须在"目标时钟自己的 duration 刻度"上做补偿，不能在
 * 秒刻度上加上 11644473600 再 duration_cast：libstdc++ 的 file_clock duration 是
 * 纳秒，11644473600 秒 → 1.164e19 ns 已超出 i64（上限约 9.22e18），
 * duration_cast 内部先乘 1e9 即触发有符号溢出
 * （UBSan: signed integer overflow: 11644473600 * 1000000000 cannot be represented in type 'long'）。
 * 正确做法是把 epoch 差值表示成目标时钟的 duration 再相加，其刻度值本身就在 i64 内。
 *
 * @param unixSeconds 自 1970-01-01 00:00:00 UTC 以来的秒数
 * @return 对应的文件时间戳
 */
[[nodiscard]] inline std::filesystem::file_time_type unixSecondsToFileTime(i64 unixSeconds)
{
#if defined(__cpp_lib_clock_cast)
    // 标准路径：system_clock -> file_clock
    const auto sysTp = std::chrono::system_clock::time_point{std::chrono::seconds{unixSeconds}};
    return std::chrono::clock_cast<std::filesystem::file_time_type::clock>(sysTp);
#else
    // 回退路径：在"file_clock 的 duration 刻度"上做补偿，避免秒→纳秒的中间量溢出。
    using FileDuration = std::filesystem::file_time_type::duration;
    using FileRep = FileDuration::rep;
    using FilePeriod = FileDuration::period;

    // 换算因子：1 秒 = FilePeriod::den / FilePeriod::num 个 file_clock 刻度
    // （libstdc++ 为纳秒 → 1e9；MSVC 为 100ns → 1e7）。
    constexpr long double kTicksPerSecond =
        static_cast<long double>(FilePeriod::den) / static_cast<long double>(FilePeriod::num);

    // epoch 差与秒数各自先算刻度值，再加总。用 long double 承载中间量：
    // 1.164e10 秒 × 1e9 = 1.164e19 已超出 i64（约 9.22e18），整数域里算必然溢出。
    const long double offsetTicks =
        static_cast<long double>(detail::fileClockToSystemOffsetSeconds()) * kTicksPerSecond;
    const long double secondsTicks = static_cast<long double>(unixSeconds) * kTicksPerSecond;

    const long double totalTicks = secondsTicks + offsetTicks;
    // 越界时钳制到该 duration 的可表示范围，避免窄化转换本身成为 UB。
    constexpr long double kMinTicks = static_cast<long double>(std::numeric_limits<FileRep>::min());
    constexpr long double kMaxTicks = static_cast<long double>(std::numeric_limits<FileRep>::max());
    const long double clampedTicks =
        totalTicks < kMinTicks ? kMinTicks : (totalTicks > kMaxTicks ? kMaxTicks : totalTicks);

    return std::filesystem::file_time_type{FileDuration{static_cast<FileRep>(clampedTicks)}};
#endif
}

} // namespace TimeUtils
} // namespace mc::util
