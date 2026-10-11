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
    // 用 floor 而非 duration_cast：duration_cast 向零截断，对负 duration（libstdc++ 的
    // file_clock epoch 在 2174 年，当今读数即为负）会让余数落到 (-1s, 0]，使下面
    // "余数差四舍五入"在边界 ±1s 处需要额外处理。floor 保证余数恒在 [0, 1s)。
    const auto wholeSeconds = std::chrono::floor<std::chrono::seconds>(sinceEpoch);
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
    //
    // 必须用 floor 而非 duration_cast：libstdc++ 的 file_clock epoch 在 2174 年，当今文件
    // 时间戳的 duration 为负值，而 duration_cast 对负值向零截断（-0.5s → 0s）。若用
    // duration_cast，位于 Unix 秒 N + 0.5 的文件时间戳会得到 N+1 而非 N，与
    // unixSecondsToFileTime 的往返结果不一致。floor 向负无穷取整，保证结果稳定。
    const auto fileSecs = std::chrono::floor<std::chrono::seconds>(ft.time_since_epoch()).count();
    return fileSecs - detail::fileClockToSystemOffsetSeconds();
#endif
}

/**
 * @brief 将 Unix 纪元秒数转换为 file_time_type
 *
 * fileTimeToUnixSeconds 的逆操作，用于从 JSON 等序列化格式恢复时间戳。
 *
 * 回退路径（无 clock_cast）在整数域内完成换算：秒→刻度的乘法会把中间量推到 1.16e19，
 * 超出 i64（上限约 9.22e18），故不能先加偏移再 duration_cast
 * （UBSan: signed integer overflow: 11644473600 * 1000000000 cannot be represented in type 'long'）。
 * 实现改为先把可表示的秒数区间换算出来做边界比较，确认不溢出后才相乘。
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
    // 回退路径：全程整数运算，且不产生任何越界的中间量。
    using FileDuration = std::filesystem::file_time_type::duration;
    using FileRep = FileDuration::rep;
    using FilePeriod = FileDuration::period;

    // 换算因子：1 秒 = FilePeriod::den / FilePeriod::num 个 file_clock 刻度
    // （libstdc++ 为纳秒 → 1e9；MSVC 为 100ns → 1e7）。
    constexpr i64 kTicksPerSecond = static_cast<i64>(FilePeriod::den) / static_cast<i64>(FilePeriod::num);

    // 为什么不能先算 totalSeconds 再判越界：unixSeconds 可达 i64 上限，
    // `unixSeconds + offsetSeconds` 本身就会溢出（UB）。
    // 为什么不能借用浮点承载中间量：秒→刻度要乘 1e9，中间量可达 1.16e19，超出 i64；
    // 且 MSVC 上 long double 就是 double，把 FileRep::max() 转成浮点会向上舍入到 2^63，
    // 使"钳制到 max"之后的 static_cast<FileRep> 依然是 UB。
    //
    // 正确做法是把边界换算到"Unix 秒"这个共同尺度上先做比较，只在确认不溢出之后才相乘。
    // FileRep 为 i64、kTicksPerSecond 为正，可表示的秒数区间为 [min/k, max/k]。
    const i64 offsetSeconds = detail::fileClockToSystemOffsetSeconds();
    constexpr i64 kMaxSeconds = std::numeric_limits<FileRep>::max() / kTicksPerSecond;
    constexpr i64 kMinSeconds = std::numeric_limits<FileRep>::min() / kTicksPerSecond;

    // 边界与 offsetSeconds 同为 1e9~1e10 量级，相减安全。
    const i64 maxUnixSeconds = kMaxSeconds - offsetSeconds;
    const i64 minUnixSeconds = kMinSeconds - offsetSeconds;
    if (unixSeconds >= maxUnixSeconds) {
        return std::filesystem::file_time_type{FileDuration::max()};
    }
    if (unixSeconds <= minUnixSeconds) {
        return std::filesystem::file_time_type{FileDuration::min()};
    }

    // 此处 unixSeconds + offsetSeconds 必落在 (kMinSeconds, kMaxSeconds) 内，
    // 故乘 kTicksPerSecond 后仍在 FileRep 可表示范围内，无溢出。
    const i64 totalSeconds = unixSeconds + offsetSeconds;
    return std::filesystem::file_time_type{FileDuration{totalSeconds * kTicksPerSecond}};
#endif
}

} // namespace TimeUtils
} // namespace mc::util
