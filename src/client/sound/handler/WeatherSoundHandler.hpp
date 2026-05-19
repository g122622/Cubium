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
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/random/Random.hpp"

#include <optional>

namespace mc::client::sound {

// 前向声明
class TickableSound;

/**
 * @brief 天气音效处理器
 *
 * 处理雨天和雷暴时的环境音效。
 *
 * 参考: net.minecraft.client.audio.BackgroundMusicSelector (播放逻辑)
 * 参考: net.minecraft.world.server.ServerWorld.playWeatherSounds()
 *
 * 音效行为:
 * - 雨天时播放循环雨声 (WEATHER_RAIN / WEATHER_RAIN_ABOVE)
 * - 雷暴时有概率播放雷声 (WEATHER_THUNDER)
 * - 音量根据天气强度调整
 *
 * 使用示例:
 * @code
 * auto handler = std::make_unique<WeatherSoundHandler>();
 * engine.addAmbientHandler(std::move(handler));
 * // 当天气状态改变时:
 * handler->updateWeatherState(rainStrength, thunderStrength, playerPos, canSeeSky);
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
     * @param playerY 玩家Y坐标（用于判断是否在雨层上方）
     * @param canSeeSky 是否能看到天空
     */
    void updateWeatherState(f32 rainStrength, f32 thunderStrength, f32 playerY, bool canSeeSky);

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
     * 根据天气强度和玩家位置播放雨声。
     * 玩家在高处或看不到天空时使用 WEATHER_RAIN_ABOVE。
     */
    void updateRainSound(SoundEngine& engine);

    /**
     * @brief 尝试播放雷声
     *
     * 雷暴时有概率播放雷声音效。
     * 雷声音量和音调随机变化。
     */
    void tryPlayThunder(SoundEngine& engine);

    /// 降雨强度
    f32 m_rainStrength = 0.0f;

    /// 雷暴强度
    f32 m_thunderStrength = 0.0f;

    /// 玩家Y坐标
    f32 m_playerY = 0.0f;

    /// 是否能看到天空
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
