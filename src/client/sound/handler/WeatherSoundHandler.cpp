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

#include "WeatherSoundHandler.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Constants.hpp"
#include "common/sound/SoundTypes.hpp"
#include <cmath>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;

// ============================================================================
// 常量
// ============================================================================

namespace {
/// 雨声最小延迟（ticks），防止开始下雨时立即播放
constexpr i32 RAIN_SOUND_MIN_DELAY = 0;

/// 雷声最小间隔（ticks）
constexpr i32 THUNDER_MIN_DELAY = 100; // 5秒

/// 雷声最大间隔（ticks）
constexpr i32 THUNDER_MAX_DELAY = 600; // 30秒

/// 高度阈值：超过此高度使用雨声_above版本
constexpr f32 RAIN_ABOVE_HEIGHT = static_cast<f32>(world::SEA_LEVEL) + 63.0f;
} // namespace

// ============================================================================
// WeatherSoundHandler 实现
// ============================================================================

WeatherSoundHandler::WeatherSoundHandler()
    : m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{}

WeatherSoundHandler::~WeatherSoundHandler()
{
    // 雨声实例由 SoundEngine 管理，不需要手动清理
}

void WeatherSoundHandler::tick(SoundEngine& engine)
{
    updateRainSound(engine);
    tryPlayThunder(engine);
}

void WeatherSoundHandler::updateWeatherState(f32 rainStrength, f32 thunderStrength, f32 playerY, bool canSeeSky)
{
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;
    m_playerY = playerY;
    m_canSeeSky = canSeeSky;
}

void WeatherSoundHandler::updateRainSound(SoundEngine& engine)
{
    // 检查是否应该播放雨声
    bool shouldPlayRain = isRaining() && m_canSeeSky;

    // 判断是否使用高空雨声
    bool useAbove = m_playerY > RAIN_ABOVE_HEIGHT;

    if (shouldPlayRain) {
        // 检查是否需要切换雨声类型
        if (m_playingRainSound && m_rainSoundAbove != useAbove) {
            // 停止当前雨声
            engine.stop(m_rainSoundId);
            m_playingRainSound = false;
        }

        // 如果没有播放雨声，启动新的
        if (!m_playingRainSound) {
            const ResourceLocation& rainSound = useAbove ? SoundEvents::WEATHER_RAIN_ABOVE : SoundEvents::WEATHER_RAIN;

            auto sound = std::make_unique<SoundInstance>(rainSound,
                sound::SoundCategory::Weather,
                glm::vec3(0.0f),       // 位置不重要，跟随玩家
                m_rainStrength,        // 音量根据强度
                1.0f,                  // 音调
                true,                  // 循环
                AttenuationType::None, // 无衰减
                DEFAULT_ATTENUATION_DISTANCE);

            m_rainSoundId = engine.play(std::move(sound));
            m_playingRainSound = true;
            m_rainSoundAbove = useAbove;
        } else {
            // 更新音量
            ISoundInstance* sound = engine.getSoundInstance(m_rainSoundId);
            if (sound) {
                sound->setVolume(m_rainStrength);
            }
        }
    } else {
        // 停止雨声
        if (m_playingRainSound) {
            engine.stop(m_rainSoundId);
            m_playingRainSound = false;
        }
    }
}

void WeatherSoundHandler::tryPlayThunder(SoundEngine& engine)
{
    // 只在雷暴时播放雷声
    if (!isThundering() || !m_canSeeSky) {
        m_thunderTimer = 0;
        m_nextThunderDelay = 0;
        return;
    }

    // 递增计时器
    m_thunderTimer++;

    // 如果还没到下次雷声时间，返回
    if (m_thunderTimer < m_nextThunderDelay) {
        return;
    }

    // 播放雷声
    auto sound = std::make_unique<SoundInstance>(SoundEvents::WEATHER_THUNDER,
        sound::SoundCategory::Weather,
        glm::vec3(0.0f),                    // 位置不重要，跟随玩家
        m_thunderStrength * m_rainStrength, // 音量根据雷暴和雨强度
        0.8f + m_rng.nextFloat() * 0.4f,    // 音调随机 0.8-1.2
        false,                              // 不循环
        AttenuationType::None,              // 无衰减
        DEFAULT_ATTENUATION_DISTANCE);

    engine.play(std::move(sound));

    // 重置计时器并设置下次雷声延迟
    m_thunderTimer = 0;
    m_nextThunderDelay =
        THUNDER_MIN_DELAY + static_cast<i32>(m_rng.nextFloat() * (THUNDER_MAX_DELAY - THUNDER_MIN_DELAY));
}

} // namespace mc::client::sound
