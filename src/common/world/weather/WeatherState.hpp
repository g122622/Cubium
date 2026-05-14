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

#include "../../core/Types.hpp"
#include "../../util/math/MathUtils.hpp"
#include "WeatherConstants.hpp"

// 使用嵌套命名空间定义，避免嵌套包含问题
namespace mc::weather {

// 使用 mc::math::lerp 进行线性插值

/**
 * @brief 天气类型枚举
 *
 * 表示世界的基本天气状态
 */
enum class WeatherType : u8 {
    Clear = 0,  ///< 晴天 - 无降水
    Rain = 1,   ///< 降雨 - 正在下雨（非雷暴）
    Thunder = 2 ///< 雷暴 - 正在下雷暴（需要同时有降雨）
};

/**
 * @brief 天气状态数据
 *
 * 存储世界的天气状态，包括计时器和强度。
 * 参考 MC 1.16.5 ServerWorld 和 WorldInfo。
 *
 * 天气周期逻辑：
 * 1. clearWeatherTime > 0 时，强制晴天
 * 2. rainTime 倒计时控制降雨状态切换
 * 3. thunderTime 倒计时控制雷暴状态切换
 * 4. rainStrength/thunderStrength 用于平滑过渡渲染
 *
 * @note 线程安全：此类不保证线程安全，调用者需自行同步
 */
class WeatherState {
public:
    // ========== 计时器 (单位: ticks) ==========

    /**
     * @brief 晴天剩余时间
     *
     * 当 > 0 时，强制保持晴天状态。
     * 由 /weather clear 命令设置。
     * 正常天气周期下为 0。
     */
    i32 clearWeatherTime = 0;

    /**
     * @brief 降雨计时器
     *
     * 倒计时到 0 时切换降雨状态。
     * 正常周期: 12000-24000 ticks (10-20分钟)
     */
    i32 rainTime = 0;

    /**
     * @brief 雷暴计时器
     *
     * 倒计时到 0 时切换雷暴状态。
     * 正常周期: 3600-15600 ticks (3-13分钟)
     * 注意：雷暴只有在降雨时才有效果
     */
    i32 thunderTime = 0;

    // ========== 状态标志 ==========

    /**
     * @brief 是否正在降雨
     */
    bool raining = false;

    /**
     * @brief 是否正在雷暴
     *
     * 雷暴时必须同时 raining = true
     */
    bool thundering = false;

    // ========== 渐变强度 (0.0 - 1.0) ==========

    /**
     * @brief 当前降雨强度
     *
     * 用于渲染平滑过渡。
     * 当 raining = true 时渐变到 1.0，否则渐变到 0.0
     * 每tick变化 ±0.01
     */
    f32 rainStrength = 0.0f;

    /**
     * @brief 上一帧降雨强度（用于插值）
     */
    f32 prevRainStrength = 0.0f;

    /**
     * @brief 当前雷暴强度
     *
     * 用于渲染平滑过渡。
     * 当 thundering = true 时渐变到 1.0，否则渐变到 0.0
     * 每tick变化 ±0.01
     */
    f32 thunderStrength = 0.0f;

    /**
     * @brief 上一帧雷暴强度（用于插值）
     */
    f32 prevThunderStrength = 0.0f;

    // ========== 游戏规则 ==========

    /**
     * @brief 天气周期是否启用
     *
     * 对应游戏规则 doWeatherCycle
     * 为 false 时，天气不会自动变化
     */
    bool weatherCycleEnabled = true;

public:
    WeatherState() = default;
    ~WeatherState() = default;

    // ========== 状态查询 ==========

    /**
     * @brief 获取当前天气类型
     *
     * @return 天气类型枚举
     */
    [[nodiscard]] WeatherType weatherType() const noexcept
    {
        if (thundering && raining) {
            return WeatherType::Thunder;
        }
        if (raining) {
            return WeatherType::Rain;
        }
        return WeatherType::Clear;
    }

    /**
     * @brief 是否正在下雨（强度检查）
     *
     * 使用强度阈值判断，比状态标志更准确
     *
     * @return 如果降雨强度 > 0.2 返回 true
     */
    [[nodiscard]] bool isRaining() const noexcept { return rainStrength > WeatherConstants::RAIN_THRESHOLD; }

    /**
     * @brief 是否正在雷暴（强度检查）
     *
     * 使用强度阈值判断，比状态标志更准确
     *
     * @return 如果雷暴强度 > 0.9 返回 true
     */
    [[nodiscard]] bool isThundering() const noexcept { return thunderStrength > WeatherConstants::THUNDER_THRESHOLD; }

    /**
     * @brief 获取插值后的降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 getRainStrength(f32 partialTick) const noexcept
    {
        return mc::math::lerp(prevRainStrength, rainStrength, partialTick);
    }

    /**
     * @brief 获取插值后的雷暴强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 getThunderStrength(f32 partialTick) const noexcept
    {
        return mc::math::lerp(prevThunderStrength, thunderStrength, partialTick);
    }

    /**
     * @brief 是否可以睡觉
     *
     * 雷暴时任何时间都可以睡觉。
     * 降雨时在 12010-23991 ticks 内可以睡觉。
     * 晴天时在 12542-23459 ticks 内可以睡觉。
     *
     * @param dayTime 当前一天内的时间 (0-23999)
     * @return 是否可以睡觉
     */
    [[nodiscard]] bool canSleep(i64 dayTime) const noexcept
    {
        // 雷暴时任何时间都可以睡觉
        if (isThundering()) {
            return true;
        }

        // 降雨时的睡眠时间范围
        if (isRaining()) {
            return dayTime >= WeatherConstants::RAIN_BED_START_TIME || dayTime <= WeatherConstants::RAIN_BED_END_TIME;
        }

        // 晴天时的睡眠时间范围
        return dayTime >= WeatherConstants::CLEAR_BED_START_TIME && dayTime <= WeatherConstants::CLEAR_BED_END_TIME;
    }

    /**
     * @brief 获取当前天空光照上限
     *
     * 雷暴时上限为 10，降雨时上限为 12，晴天无上限
     *
     * @return 天空光照上限 (0-15)，0 表示无限制
     */
    [[nodiscard]] u8 skyLightLimit() const noexcept
    {
        if (isThundering()) {
            return WeatherConstants::THUNDER_SKY_LIGHT_LIMIT;
        }
        if (isRaining()) {
            return WeatherConstants::RAIN_SKY_LIGHT_LIMIT;
        }
        return 15; // 无限制
    }

    // ========== 状态重置 ==========

    /**
     * @brief 重置为晴天
     *
     * 立即清除所有天气效果，用于玩家睡觉后
     */
    void resetWeather() noexcept
    {
        clearWeatherTime = 0;
        rainTime = 0;
        thunderTime = 0;
        raining = false;
        thundering = false;
        // 强度会自然渐变到 0
    }
};

} // namespace mc::weather
