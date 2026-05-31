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

#include "BeeSound.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/math/MathUtils.hpp"
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

namespace mc::client::sound {

// ============================================================================
// BeeSound 实现
// ============================================================================

BeeSound::BeeSound(const ClientEntity& bee, const ResourceLocation& soundEventId)
    : TickableSound(soundEventId,
          SoundCategory::Neutral,
          glm::vec3(bee.x(), bee.y(), bee.z()),
          0.0f,
          0.0f,
          true,
          AttenuationType::Linear,
          16.0f)
    , m_bee(bee)
{
    setPosition(glm::vec3(bee.x(), bee.y(), bee.z()));
    setLooping(true);
    setVolume(0.0f);
}

void BeeSound::tick()
{
    // 检查是否需要切换声音
    bool shouldSwitch = shouldSwitchSound();

    if (shouldSwitch && !isDone()) {
        // TODO: 目前仅标记完成，需要通过 SoundEngine::playOnNextTick 播放下一个声音
        m_hasSwitchedSound = true;
        markDone();
    }

    // 检查蜜蜂是否仍然有效
    if (m_bee.isRemoved()) {
        markDone();
        return;
    }

    // 如果已经切换声音，停止更新
    if (m_hasSwitchedSound) {
        markDone();
        return;
    }

    // 更新位置跟随蜜蜂
    setPosition(glm::vec3(m_bee.x(), m_bee.y(), m_bee.z()));

    // 根据水平速度计算音量和音调
    auto vel = m_bee.velocity();
    f32 horizontalSpeed = std::sqrt(vel.x * vel.x + vel.z * vel.z);

    if (horizontalSpeed >= 0.01f) {
        // 根据速度插值音调
        f32 minPitch = _getMinPitch();
        f32 maxPitch = _getMaxPitch();
        f32 clampedSpeed = std::clamp(horizontalSpeed, minPitch, maxPitch);
        f32 pitch = math::lerp(minPitch, maxPitch, clampedSpeed);
        setPitch(pitch);

        // 根据速度插值音量
        f32 clampedVol = std::clamp(horizontalSpeed, 0.0f, 0.5f);
        f32 volume = math::lerp(0.0f, 1.2f, clampedVol);
        setVolume(volume);
    } else {
        // 速度太低，静音
        setPitch(0.0f);
        setVolume(0.0f);
    }
}

f32 BeeSound::_getMinPitch() const
{
    // 幼年蜜蜂音调更高
    return m_bee.isChild() ? 1.1f : 0.7f;
}

f32 BeeSound::_getMaxPitch() const
{
    // 幼年蜜蜂音调更高
    return m_bee.isChild() ? 1.5f : 1.1f;
}

// ============================================================================
// BeeFlightSound 实现
// ============================================================================

BeeFlightSound::BeeFlightSound(const ClientEntity& bee)
    : BeeSound(bee, SoundEvents::ENTITY_BEE_LOOP)
{}

std::unique_ptr<TickableSound> BeeFlightSound::getNextSound()
{
    // 切换到愤怒声音
    return std::make_unique<BeeAngrySound>(bee());
}

bool BeeFlightSound::shouldSwitchSound()
{
    // 当蜜蜂愤怒时切换到愤怒声音
    return bee().isAngry();
}

// ============================================================================
// BeeAngrySound 实现
// ============================================================================

BeeAngrySound::BeeAngrySound(const ClientEntity& bee)
    : BeeSound(bee, SoundEvents::ENTITY_BEE_LOOP_AGGRESSIVE)
{}

std::unique_ptr<TickableSound> BeeAngrySound::getNextSound()
{
    // 切换回飞行声音
    return std::make_unique<BeeFlightSound>(bee());
}

bool BeeAngrySound::shouldSwitchSound()
{
    // 当蜜蜂不再愤怒时切换回飞行声音
    return !bee().isAngry();
}

} // namespace mc::client::sound
