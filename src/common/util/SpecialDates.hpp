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

#pragma once

#include <chrono>
#include <__msvc_chrono.hpp>

namespace mc::util {

/**
 * @brief 特殊日期判断工具
 *
 * 提供基于系统日期的特殊节日判断功能，用于游戏内季节性内容：
 * - 万圣节（10月31日）：僵尸/骷髅戴南瓜头
 * - 圣诞节（12月24-26日）：箱子纹理切换、物品模型切换
 *
 * 参考 MC 源码 net.minecraft.util.SpecialDates
 */
namespace SpecialDates {

/// 万圣节：10月31日
inline constexpr std::chrono::month_day HALLOWEEN = std::chrono::month_day{std::chrono::October, std::chrono::day{31}};

/// 圣诞节范围：12月24-26日
inline constexpr std::chrono::month_day CHRISTMAS = std::chrono::month_day{std::chrono::December, std::chrono::day{24}};

/// 元旦：1月1日
inline constexpr std::chrono::month_day NEW_YEAR = std::chrono::month_day{std::chrono::January, std::chrono::day{1}};

/**
 * @brief 获取当前本地日期（月-日）
 *
 * 使用系统本地时间获取当前月日，用于与常量比较。
 *
 * @return 当前日期的 month_day
 */
[[nodiscard]] inline std::chrono::month_day dayNow()
{
    const auto now = std::chrono::system_clock::now();
    const auto days = std::chrono::floor<std::chrono::days>(now);
    const std::chrono::year_month_day ymd{days};
    return std::chrono::month_day{ymd.month(), ymd.day()};
}

/**
 * @brief 判断今天是否为万圣节（10月31日）
 */
[[nodiscard]] inline bool isHalloween()
{
    return HALLOWEEN == dayNow();
}

/**
 * @brief 判断今天是否在圣诞范围内（12月24-26日）
 */
[[nodiscard]] inline bool isExtendedChristmas()
{
    const auto today = dayNow();
    const auto month = today.month();
    const auto day = today.day();
    return month == std::chrono::December && day >= std::chrono::day{24} && day <= std::chrono::day{26};
}

} // namespace SpecialDates
} // namespace mc::util
