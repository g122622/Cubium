#pragma once

#include "../../core/Types.hpp"
#include "../../util/math/Vector3.hpp"
#include "WeatherConstants.hpp"

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
 * 无降水生物群系需要由调用者结合 BiomeClimate::Precipitation::None 额外过滤。
 *
 * 参考 MC 1.16.5 Biome.doesSnowGenerate(): temperature >= 0.15F 时返回 false
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
 * 需要满足：可以看到天空 + 生物群系允许降水
 *
 * @param world 世界引用
 * @param pos 方块位置
 * @return 是否可以降雨
 */
[[nodiscard]] bool canRainAt(const mc::IWorld& world, const mc::BlockPos& pos);

/**
 * @brief 判断位置是否可以降雪
 *
 * 需要满足：可以看到天空 + 生物群系温度 <= 0.15
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
 *
 * 参考 MC 1.16.5 ClientWorld.getSunBrightness():
 * f1 = (1 - rain * 5/16) * (1 - thunder * 5/16)
 * 暗化因子 = 1 - f1
 *
 * @param rainStrength 降雨强度 (0.0-1.0)
 * @param thunderStrength 雷暴强度 (0.0-1.0)，注意MC中thunderStrength已经乘以rainStrength
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
