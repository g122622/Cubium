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

#include "client/sound/SoundEngine.hpp"
#include "client/sound/handler/IAmbientSoundHandler.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include <memory>

namespace mc::client::sound {

/**
 * @brief 天气音效处理器
 *
 * 处理雨天和雷暴时的环境音效。
 *
 * 音效行为:
 * - 户外（canSeeSky=true）时播放 WEATHER_RAIN（音量 0.2，音调 1.0）
 * - 遮挡物下方（canSeeSky=false）时播放 WEATHER_RAIN_ABOVE（音量 0.1，音调 0.5）
 *   模拟从上方隔层传来的闷雨声效果
 * - 雷暴时有概率播放雷声 (WEATHER_THUNDER)
 * - 音量根据天气强度调整
 *
 * 雨声类型判断逻辑:
 * 当玩家位置上方有运动阻挡方块（MOTION_BLOCKING 高度图值 > 玩家 Y），
 * 且雨滴落点在玩家上方时，播放 WEATHER_RAIN_ABOVE。
 * 当前使用 canSeeSky（基于天空光照 >= 15）来近似 MOTION_BLOCKING 判断：
 * canSeeSky=false 等价于玩家上方有遮挡物（屋顶、洞穴等）。
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<WeatherSoundHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * // 当天气状态改变时:
 * handler->updateWeatherState(rainStrength, thunderStrength, canSeeSky);
 * @endcode
 */
class WeatherSoundHandler : public IAmbientSoundHandler {
public:
    WeatherSoundHandler();
    ~WeatherSoundHandler() override;

    /**
     * @brief 每帧更新
     *
     * 检查天气状态并播放/停止天气音效。
     *
     * @param engine 声音引擎
     */
    void tick(SoundEngine& engine) override;

    /**
     * @brief 更新天气状态
     *
     * 每帧由客户端应用调用，传递当前天气状态。
     *
     * @param rainStrength 降雨强度 (0.0 - 1.0)
     * @param thunderStrength 雷暴强度 (0.0 - 1.0)
     * @param canSeeSky 玩家眼睛位置是否能看到天空（基于天空光照判断）
     */
    void updateWeatherState(f32 rainStrength, f32 thunderStrength, bool canSeeSky);

    /**
     * @brief 检查是否正在下雨
     */
    [[nodiscard]] bool isRaining() const noexcept { return m_rainStrength > 0.01f; }

    /**
     * @brief 检查是否正在雷暴
     */
    [[nodiscard]] bool isThundering() const noexcept { return m_thunderStrength > 0.9f; }

private:
    /**
     * @brief 更新雨声音效
     *
     * 根据天气强度和天空可见性播放雨声。
     * canSeeSky=true 时播放 WEATHER_RAIN，canSeeSky=false 时播放 WEATHER_RAIN_ABOVE。
     */
    void _updateRainSound(SoundEngine& engine);

    /**
     * @brief 尝试播放雷声
     *
     * 雷暴时有概率播放雷声音效。
     * 雷声音量和音调随机变化。
     */
    void _tryPlayThunder(SoundEngine& engine);

    /// 降雨强度
    f32 m_rainStrength = 0.0f;

    /// 雷暴强度
    f32 m_thunderStrength = 0.0f;

    /// 玩家眼睛位置是否能看到天空
    bool m_canSeeSky = true;

    /// 雨声声音实例ID
    SoundInstanceId m_rainSoundId = 0;

    /// 是否正在播放雨声
    bool m_playingRainSound = false;

    /// 上次播放雨声的类型 (false = RAIN, true = RAIN_ABOVE)
    bool m_rainSoundAbove = false;

    /// 雷声计时器 (ticks)
    i32 m_thunderTimer = 0;

    /// 下次雷声延迟 (ticks)
    i32 m_nextThunderDelay = 0;

    /// 随机数生成器
    math::Random m_rng{0};
};

} // namespace mc::client::sound
