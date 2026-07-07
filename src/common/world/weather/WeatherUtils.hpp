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

#include "WeatherConstants.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;

namespace math {
class IRandom;
}

} // namespace mc

// 使用嵌套命名空间定义，避免嵌套包含问题
namespace mc::weather {

/**
 * @brief 天气工具函数
 *
 * 提供天气相关的计算和判断功能
 */
namespace WeatherUtils {

/**
 * @brief 根据生物群系温度判断降水类型
 *
 * 温度 < 0.15 时降雪，>= 0.15 时降雨。
 * 无降水生物群系需要由调用者结合 Biome::hasPrecipitation() 额外过滤，
 * 或直接使用 canRainAt()/canSnowAt() 方法。
 *
 * @param temperature 生物群系温度
 * @return 1=降雨, 2=降雪
 */
[[nodiscard]] inline i32 getPrecipitationType(f32 temperature) noexcept
{
    if (temperature < 0.15f) {
        return 2; // Snow
    }
    return 1; // Rain
}

/**
 * @brief 判断位置是否可以看到天空
 *
 * 用于判断是否受天气影响（雨滴、闪电等）
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 是否可以看到天空
 */
[[nodiscard]] bool canSeeSky(const mc::IWorld& world, const mc::BlockPos& pos);

/**
 * @brief 判断位置是否可以降雨
 *
 * 需要满足：可以看到天空 + 上方无运动阻挡方块（MOTION_BLOCKING 高度图）
 * + 生物群系允许降水 + 高度调整温度 >= 0.15
 * 使用 Biome::getHeightAdjustedTemperature() 替代基础温度，
 * 以正确处理高海拔降温效果（MC 1.21.11 对齐）。
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 是否可以降雨
 */
[[nodiscard]] bool canRainAt(const mc::IWorld& world, const mc::BlockPos& pos);

/**
 * @brief 判断位置是否可以降雪
 *
 * 需要满足：可以看到天空 + 上方无运动阻挡方块（MOTION_BLOCKING 高度图）
 * + 生物群系允许降水 + 高度调整温度 < 0.15
 * 使用 Biome::getHeightAdjustedTemperature() 替代基础温度，
 * 以正确处理高海拔降温效果（MC 1.21.11 对齐）。
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 是否可以降雪
 */
[[nodiscard]] bool canSnowAt(const mc::IWorld& world, const mc::BlockPos& pos);

/**
 * @brief 获取随机天气持续时间
 *
 * @param rng 随机数生成器
 * @param minTime 最小时间 (ticks)
 * @param maxTime 最大时间 (ticks)
 * @return 随机持续时间
 */
[[nodiscard]] i32 getRandomWeatherDuration(mc::math::IRandom& rng, i32 minTime, i32 maxTime);

/**
 * @brief 生成随机晴天持续时间
 *
 * 范围: 12000 - 168000 ticks (10-140分钟)
 *
 * @param rng 随机数生成器
 * @return 随机持续时间
 */
[[nodiscard]] i32 getRandomClearDuration(mc::math::IRandom& rng);

/**
 * @brief 生成随机降雨持续时间
 *
 * 范围: 12000 - 24000 ticks (10-20分钟)
 *
 * @param rng 随机数生成器
 * @return 随机持续时间
 */
[[nodiscard]] i32 getRandomRainDuration(math::IRandom& rng);

/**
 * @brief 生成随机雷暴持续时间
 *
 * 范围: 3600 - 15600 ticks (3-13分钟)
 *
 * @param rng 随机数生成器
 * @return 随机持续时间
 */
[[nodiscard]] i32 getRandomThunderDuration(math::IRandom& rng);

/**
 * @brief 计算天空颜色混合因子
 *
 * 根据天气强度计算天空颜色的暗化程度
 * f1 = (1 - rain * 5/16) * (1 - thunder * 5/16)
 * 暗化因子 = 1 - f1
 *
 * @param rainStrength 降雨强度 (0.0-1.0)
 * @param thunderStrength 雷暴强度 (0.0-1.0)，thunderStrength已经乘以rainStrength
 * @return 混合因子 (0.0=正常, 接近0.527=最暗)
 */
[[nodiscard]] inline f32 calculateSkyDarkenFactor(f32 rainStrength, f32 thunderStrength) noexcept
{
    // MC原版使用乘法组合: (1 - rain * 5/16) * (1 - thunder * 5/16)
    f32 rainFactor = rainStrength * (5.0f / 16.0f);
    f32 thunderFactor = thunderStrength * (5.0f / 16.0f);
    return 1.0f - (1.0f - rainFactor) * (1.0f - thunderFactor);
}

/**
 * @brief 计算太阳/月亮可见度
 *
 * @param rainStrength 降雨强度 (0.0-1.0)
 * @return 可见度 (0.0=不可见, 1.0=完全可见)
 */
[[nodiscard]] inline f32 calculateCelestialVisibility(f32 rainStrength) noexcept
{
    // 降雨时太阳/月亮不可见
    return 1.0f - rainStrength;
}

/**
 * @brief 计算星星亮度
 *
 * @param rainStrength 降雨强度 (0.0-1.0)
 * @param dayTime 当前时间 (0-23999)
 * @return 星星亮度 (0.0-1.0)
 */
[[nodiscard]] f32 calculateStarBrightness(f32 rainStrength, i64 dayTime);

} // namespace WeatherUtils

} // namespace mc::weather
