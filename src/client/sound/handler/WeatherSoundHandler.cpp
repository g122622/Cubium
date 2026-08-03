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
#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/sound/SoundTypes.hpp"
#include <chrono>
#include <memory>
#include <utility>
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

// 从 mc::sound 引入类型
using ::mc::sound::DEFAULT_ATTENUATION_DISTANCE;

// ============================================================================
// 常量
// ============================================================================

namespace {
/// 雷声最小间隔（ticks）
constexpr i32 THUNDER_MIN_DELAY = 100; // 5秒

/// 雷声最大间隔（ticks）
constexpr i32 THUNDER_MAX_DELAY = 600; // 30秒

/// WEATHER_RAIN 音量
constexpr f32 RAIN_VOLUME = 0.2f;

/// WEATHER_RAIN 音调
constexpr f32 RAIN_PITCH = 1.0f;

/// WEATHER_RAIN_ABOVE 音量（闷雨声音量为正常雨声的一半）
constexpr f32 RAIN_ABOVE_VOLUME = 0.1f;

/// WEATHER_RAIN_ABOVE 音调（闷雨声音调为正常雨声的一半）
constexpr f32 RAIN_ABOVE_PITCH = 0.5f;
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
    _updateRainSound(engine);
    _tryPlayThunder(engine);
}

void WeatherSoundHandler::updateWeatherState(f32 rainStrength, f32 thunderStrength, bool canSeeSky)
{
    m_rainStrength = rainStrength;
    m_thunderStrength = thunderStrength;
    m_canSeeSky = canSeeSky;
}

void WeatherSoundHandler::_updateRainSound(SoundEngine& engine)
{
    // 检查是否应该播放雨声
    bool shouldPlayRain = isRaining();

    // 判断是否使用高空/遮挡雨声
    // MC 原版逻辑: 当玩家位置上方有 MOTION_BLOCKING 方块遮挡时播放 WEATHER_RAIN_ABOVE
    // 使用 canSeeSky 近似判断: canSeeSky=false 表示天空光照被遮挡，即玩家在遮挡物下方
    bool useAbove = !m_canSeeSky;

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
            const f32 volume = useAbove ? RAIN_ABOVE_VOLUME : RAIN_VOLUME;
            const f32 pitch = useAbove ? RAIN_ABOVE_PITCH : RAIN_PITCH;

            auto sound = std::make_unique<SoundInstance>(rainSound,
                sound::SoundCategory::Weather,
                glm::vec3(0.0f),         // 位置不重要，跟随玩家
                m_rainStrength * volume, // 音量根据强度和雨声类型调整
                pitch,                   // 音调根据雨声类型调整
                true,                    // 循环
                AttenuationType::None,   // 无衰减
                DEFAULT_ATTENUATION_DISTANCE);

            m_rainSoundId = engine.play(std::move(sound));
            m_playingRainSound = true;
            m_rainSoundAbove = useAbove;
        } else {
            // 更新音量（保持类型对应的音量系数）
            const f32 volume = useAbove ? RAIN_ABOVE_VOLUME : RAIN_VOLUME;
            ISoundInstance* sound = engine.getSoundInstance(m_rainSoundId);
            if (sound) {
                sound->setVolume(m_rainStrength * volume);
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

void WeatherSoundHandler::_tryPlayThunder(SoundEngine& engine)
{
    // 只在雷暴且能看到天空时播放雷声
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
