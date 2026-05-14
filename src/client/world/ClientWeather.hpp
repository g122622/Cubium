#pragma once

#include "common/core/Types.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/weather/WeatherConstants.hpp"
#include <cmath>

namespace mc {
namespace client {

/**
 * @brief 客户端天气状态
 *
 * 存储和维护客户端的天气状态，用于渲染。
 * 通过网络包接收服务端天气同步，平滑过渡天气效果。
 *
 * 参考 MC 1.16.5 ClientWorld
 */
class ClientWeather {
public:
    ClientWeather() = default;
    ~ClientWeather() = default;

    // ========== 状态更新 ==========

    /**
     * @brief 更新降雨强度
     *
     * 由 GameStateChangePacket (RainStrengthChange) 调用
     *
     * 参考 MC 1.16.5 World.setRainStrength():
     * prevRainingStrength = strength;
     * rainingStrength = strength;
     * 同时设置 prev 和 current 为相同值
     *
     * @param strength 目标降雨强度 (0.0 - 1.0)
     */
    void setRainStrength(f32 strength)
    {
        m_prevRainStrength = strength;
        m_rainStrength = strength;
    }

    /**
     * @brief 更新雷暴强度
     *
     * 由 GameStateChangePacket (ThunderStrengthChange) 调用
     *
     * 参考 MC 1.16.5 World.setThunderStrength():
     * prevThunderingStrength = strength;
     * thunderingStrength = strength;
     * 同时设置 prev 和 current 为相同值
     *
     * @param strength 目标雷暴强度 (0.0 - 1.0)
     */
    void setThunderStrength(f32 strength)
    {
        m_prevThunderStrength = strength;
        m_thunderStrength = strength;
    }

    /**
     * @brief 开始下雨
     *
     * 由 GameStateChangePacket (BeginRaining) 调用
     *
     * 注意：MC 中 BeginRaining 只是通知下雨开始，实际强度由后续 RainStrengthChange 包同步
     */
    void beginRain()
    {
        // 开始下雨时，服务端会随后发送 RainStrengthChange 包来设置具体强度
        // 这里可以预先设置一个初始强度，用于渲染平滑过渡
        // 如果之前没有下雨，可以开始渐变
    }

    /**
     * @brief 雨停
     *
     * 由 GameStateChangePacket (EndRaining) 调用
     */
    void endRain()
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
    void tick()
    {
        // 可选：如果需要客户端本地平滑过渡，可以在这里实现
        // 当前实现：prev 值在 setXXX 时设置，渲染时使用 partialTick 插值
    }

    // ========== 状态查询 ==========

    /**
     * @brief 是否正在下雨（强度检查）
     */
    [[nodiscard]] bool isRaining() const { return m_rainStrength > weather::WeatherConstants::RAIN_THRESHOLD; }

    /**
     * @brief 是否正在雷暴（强度检查）
     *
     * 参考 MC 1.16.5 World.isThundering():
     * return (double)this.getThunderStrength(1.0F) > 0.9D;
     * 使用 thunderStrength() 方法（已乘以 rainStrength）
     */
    [[nodiscard]] bool isThundering() const
    {
        return thunderStrength(1.0f) > weather::WeatherConstants::THUNDER_THRESHOLD;
    }

    /**
     * @brief 获取插值后的降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 rainStrength(f32 partialTick) const
    {
        return math::lerp(m_prevRainStrength, m_rainStrength, partialTick);
    }

    /**
     * @brief 获取插值后的雷暴强度
     *
     * 参考 MC 1.16.5 World.getThunderStrength(float delta):
     * return MathHelper.lerp(delta, prevThunderingStrength, thunderingStrength) * getRainStrength(delta);
     * 雷暴强度始终乘以降雨强度
     *
     * @param partialTick 部分 tick (0.0 - 1.0)
     * @return 插值后的强度值
     */
    [[nodiscard]] f32 thunderStrength(f32 partialTick) const
    {
        return math::lerp(m_prevThunderStrength, m_thunderStrength, partialTick) * rainStrength(partialTick);
    }

    /**
     * @brief 获取当前降雨强度（无插值）
     */
    [[nodiscard]] f32 rainStrength() const { return m_rainStrength; }

    /**
     * @brief 获取当前雷暴强度（无插值）
     */
    [[nodiscard]] f32 thunderStrength() const { return m_thunderStrength; }

    /**
     * @brief 计算天空颜色混合因子
     *
     * 参考 MC 1.16.5 ClientWorld.getSunBrightness():
     * f1 = (1 - rain * 5/16) * (1 - thunder * 5/16)
     * 暗化因子 = 1 - f1
     *
     * @param partialTick 部分 tick
     * @return 暗化因子 (0.0=正常亮度, 约0.527=最大暗化)
     */
    [[nodiscard]] f32 skyDarkenFactor(f32 partialTick) const
    {
        f32 rain = rainStrength(partialTick);
        f32 thunder = thunderStrength(partialTick);
        // MC原版使用乘法组合: (1 - rain * 5/16) * (1 - thunder * 5/16)
        // 返回暗化因子: 1.0 - result
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
    [[nodiscard]] f32 celestialVisibility(f32 partialTick) const { return 1.0f - rainStrength(partialTick); }

    /**
     * @brief 计算天空光照上限
     *
     * @return 天空光照上限 (0-15)，0表示无限制
     */
    [[nodiscard]] u8 skyLightLimit() const
    {
        if (isThundering()) {
            return weather::WeatherConstants::THUNDER_SKY_LIGHT_LIMIT;
        }
        if (isRaining()) {
            return weather::WeatherConstants::RAIN_SKY_LIGHT_LIMIT;
        }
        return 15;
    }

private:
    f32 m_rainStrength = 0.0f;
    f32 m_prevRainStrength = 0.0f;
    f32 m_thunderStrength = 0.0f;
    f32 m_prevThunderStrength = 0.0f;
};

} // namespace client
} // namespace mc
