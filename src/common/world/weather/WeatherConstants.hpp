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

namespace mc::weather {

/**
 * @brief 天气常量
 *
 * 天气周期使用与原版 ServerLevel 相同的四组区间，均为闭区间：
 * - 降雨等待/持续：RAIN_DELAY / RAIN_DURATION
 * - 雷暴等待/持续：THUNDER_DELAY / THUNDER_DURATION
 *
 * 所有时间单位为 ticks (20 ticks = 1秒)。天气强度每 tick 变化
 * ±STRENGTH_CHANGE_RATE，并夹取到 [0, 1]。
 */
namespace WeatherConstants {

// ========== 天气周期区间（闭区间） ==========

/// 不在下雨时，rainTime 的重置范围 (10-150 分钟)
constexpr i32 RAIN_DELAY_MIN = 12000;
constexpr i32 RAIN_DELAY_MAX = 180000;

/// 正在下雨时，rainTime 的重置范围 (10-20 分钟)
constexpr i32 RAIN_DURATION_MIN = 12000;
constexpr i32 RAIN_DURATION_MAX = 24000;

/// 不在雷暴时，thunderTime 的重置范围 (10-150 分钟)
constexpr i32 THUNDER_DELAY_MIN = 12000;
constexpr i32 THUNDER_DELAY_MAX = 180000;

/// 正在雷暴时，thunderTime 的重置范围 (3-13 分钟)
constexpr i32 THUNDER_DURATION_MIN = 3600;
constexpr i32 THUNDER_DURATION_MAX = 15600;

// ========== 强度 ==========

/// 天气强度渐变速率 (每 tick)，从 0.0 到 1.0 需要 100 ticks (5秒)
constexpr f32 STRENGTH_CHANGE_RATE = 0.01f;

/// 降雨强度阈值（超过此值认为正在下雨，对应原版 Level.isRaining 的 0.2）
constexpr f32 RAIN_THRESHOLD = 0.2f;

/// 雷暴强度阈值（超过此值认为正在雷暴，对应原版 Level.isThundering 的 0.9；
/// 判定使用"雷暴强度 × 降雨强度"，与 Level.getThunderLevel 一致）
constexpr f32 THUNDER_THRESHOLD = 0.9f;

// ========== 闪电 ==========

/// 闪电生成概率分母：每个实体 ticking 区块每 tick 判定 1/100000
constexpr i32 LIGHTNING_CHANCE_DENOMINATOR = 100000;

/// 骷髅马陷阱难度系数：区域有效难度 × 本系数即为陷阱生成概率
/// 区域难度随难度与居住时间在 0.75~1.5（简单）/1.5~4（普通）/2.25~6.75（困难）区间变化，
/// 因此实际概率约为 0.75%~1.5% / 1.5%~4% / 2.25%~6.75%。
constexpr f32 SKELETON_HORSE_TRAP_CHANCE = 0.01f;

} // namespace WeatherConstants

} // namespace mc::weather
