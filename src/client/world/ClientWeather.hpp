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
#include "common/util/math/MathUtils.hpp"
#include "common/world/weather/WeatherConstants.hpp"

namespace mc {
namespace client {

/**
 * @brief 客户端天气状态
 *
 * 存储和维护客户端的天气状态，用于渲染。
 * 通过网络包接收服务端天气同步，平滑过渡天气效果。
 */
class ClientWeather {
public:
    ClientWeather() = default;
    ~ClientWeather() = default;

    // ========== 状态更新 ==========

    /**
     * @brief 重置天气状态
     *
     * 维度切换时调用，清除降雨、雷暴和闪电状态。
     */
    void reset() noexcept
    {
        m_rainStrength = 0.0f;
        m_prevRainStrength = 0.0f;
        m_thunderStrength = 0.0f;
        m_prevThunderStrength = 0.0f;
        m_lightningFlashTime = 0;
    }

    /**
     * @brief 更新降雨强度
     *
     * 由 ir::play::GameStateChange (RainStrengthChange) 调用
     * 同时设置 prev 和 current 为相同值
     *
     * @param strength 目标降雨强度 (0.0 - 1.0)
     */
    void setRainStrength(f32 strength) noexcept
    {
        m_prevRainStrength = strength;
        m_rainStrength = strength;
    }

    /**
     * @brief 更新雷暴强度
     *
     * 由 ir::play::GameStateChange (ThunderStrengthChange) 调用
     * 同时设置 prev 和 current 为相同值
     *
     * @param strength 目标雷暴强度 (0.0 - 1.0)
     */
    void setThunderStrength(f32 strength) noexcept
    {
        m_prevThunderStrength = strength;
        m_thunderStrength = strength;
    }

    /**
     * @brief 开始下雨
     *
     * 由 ir::play::GameStateChange (BeginRaining) 调用
     * 服务端会随后发送 RainStrengthChange 包来设置具体强度
     */
    void beginRain() noexcept
    {
        // 空实现：服务端会通过 RainStrengthChange 包同步具体强度
    }

    /**
     * @brief 雨停
     *
     * 由 ir::play::GameStateChange (EndRaining) 调用
     */
    void endRain() noexcept
    {
        // 雨停时，设置当前强度为 0，并保留 prev 用于插值
        m_prevRainStrength = m_rainStrength;
        m_rainStrength = 0.0f;
        m_prevThunderStrength = m_thunderStrength;
        m_thunderStrength = 0.0f;
    }

    /**
     * @brief 每 tick 更新（用于平滑过渡）
     *
     * 客户端本地调用，使 prev 值渐变到当前值
     * 实际上 MC 服务端每 tick 发送强度更新，客户端只需接收
     */
    void tick() noexcept
    {
        // 可选：如果需要客户端本地平滑过渡，可以在这里实现
        // 当前实现：prev 值在 setXXX 时设置，渲染时使用 partialTick 插值
    }

    // ========== 状态查询 ==========

    /**
     * @brief 是否正在下雨（强度检查）
     */
    [[nodiscard]] bool isRaining() const noexcept { return m_rainStrength > weather::WeatherConstants::RAIN_THRESHOLD; }

    /**
     * @brief 是否正在雷暴（强度检查）
     * 使用 thunderStrength() 方法（已乘以 rainStrength）
     */
    [[nodiscard]] bool isThundering() const noexcept
    {
        return thunderStrength(1.0f) > weather::WeatherConstants::THUNDER_THRESHOLD;
    }

    /**
     * @brief 获取插值后的降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 rainStrength(f32 partialTick) const noexcept
    {
        return math::lerp(m_prevRainStrength, m_rainStrength, partialTick);
    }

    /**
     * @brief 获取插值后的雷暴强度
     * 雷暴强度始终乘以降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 thunderStrength(f32 partialTick) const noexcept
    {
        return math::lerp(m_prevThunderStrength, m_thunderStrength, partialTick) * rainStrength(partialTick);
    }

    /**
     * @brief 获取当前降雨强度（无插值）
     */
    [[nodiscard]] f32 rainStrength() const noexcept { return m_rainStrength; }

    /**
     * @brief 获取当前雷暴强度（无插值）
     */
    [[nodiscard]] f32 thunderStrength() const noexcept { return m_thunderStrength; }

    /**
     * @brief 计算天空颜色混合因子
     * 暗化因子 = 1 - (1 - rain * 5/16) * (1 - thunder * 5/16)
     *
     * @param partialTick 部分 tick
     * @return 暗化因子 (0.0=正常亮度, 约0.527=最大暗化)
     */
    [[nodiscard]] f32 skyDarkenFactor(f32 partialTick) const noexcept
    {
        f32 rain = rainStrength(partialTick);
        f32 thunder = thunderStrength(partialTick);
        // 暗化因子计算：1 - (1 - rain * 5/16) * (1 - thunder * 5/16)
        f32 rainFactor = rain * (5.0f / 16.0f);
        f32 thunderFactor = thunder * (5.0f / 16.0f);
        return 1.0f - (1.0f - rainFactor) * (1.0f - thunderFactor);
    }

    /**
     * @brief 计算太阳/月亮可见度
     *
     * @param partialTick 部分 tick
     * @return 可见度 (0.0=不可见, 1.0=完全可见)
     */
    [[nodiscard]] f32 celestialVisibility(f32 partialTick) const noexcept { return 1.0f - rainStrength(partialTick); }

    /**
     * @brief 计算天空光照上限
     *
     * @return 天空光照上限 (0-15)，0表示无限制
     */
    [[nodiscard]] u8 skyLightLimit() const noexcept
    {
        if (isThundering()) {
            return weather::WeatherConstants::THUNDER_SKY_LIGHT_LIMIT;
        }
        if (isRaining()) {
            return weather::WeatherConstants::RAIN_SKY_LIGHT_LIMIT;
        }
        return 15;
    }

    // ========== 闪电闪烁效果 ==========

    /**
     * @brief 设置闪电闪烁时间
     * 当闪电击中时，客户端需要知道以便产生天空闪烁效果
     *
     * @param time 闪烁时间（ticks），通常为 2
     */
    void setTimeLightningFlash(i32 time) noexcept { m_lightningFlashTime = time; }

    /**
     * @brief 获取当前闪电闪烁时间
     *
     * @return 当前闪烁时间（ticks），0表示无闪烁
     */
    [[nodiscard]] i32 lightningFlashTime() const noexcept { return m_lightningFlashTime; }

    /**
     * @brief 检查是否有闪电闪烁效果
     *
     * @return 如果有闪烁效果返回 true
     */
    [[nodiscard]] bool hasLightningFlash() const noexcept { return m_lightningFlashTime > 0; }

    /**
     * @brief 每 tick 更新闪电闪烁时间
     * 当闪电闪烁时间 > 0 时递减
     */
    void tickLightningFlash() noexcept
    {
        if (m_lightningFlashTime > 0) {
            --m_lightningFlashTime;
        }
    }

    /**
     * @brief 计算闪电闪烁亮度因子
     * 当闪电闪烁时，返回一个 0-1 的因子用于增强天空亮度
     *
     * @return 亮度增强因子 (0.0 = 无效果, 1.0 = 最大闪烁)
     */
    [[nodiscard]] f32 lightningFlashBrightness() const noexcept
    {
        // 闪电闪烁时，天空会短暂变亮
        // 简单实现：有闪烁时返回 1.0
        return m_lightningFlashTime > 0 ? 1.0f : 0.0f;
    }

private:
    f32 m_rainStrength = 0.0f;
    f32 m_prevRainStrength = 0.0f;
    f32 m_thunderStrength = 0.0f;
    f32 m_prevThunderStrength = 0.0f;
    i32 m_lightningFlashTime = 0; ///< 闪电闪烁剩余时间（ticks）
};

} // namespace client
} // namespace mc
