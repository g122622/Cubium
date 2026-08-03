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

#include "client/sound/instance/MinecartSound.hpp"
#include "client/sound/handler/EntitySoundHandler.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include <cmath>

namespace mc::client::sound {

// ============================================================================
// MinecartSoundStateful
// ============================================================================

MinecartSoundStateful::MinecartSoundStateful(const EntitySoundState& state, EntitySoundHandler* handler)
    : TickableSound(SoundEvents::ENTITY_MINECART_RIDING,
          SoundCategory::Neutral,
          state.position,
          0.0f, // 初始音量为0
          1.0f, // 音调
          true, // 循环
          AttenuationType::Linear,
          16.0f // 衰减距离
          )
    , m_handler(handler)
    , m_entityId(state.entityId)
{}

void MinecartSoundStateful::tick()
{
    MC_ASSERT_RELEASE(m_handler);
    const EntitySoundState* state = m_handler->getEntityState(m_entityId);
    if (state) {
        // 检查实体是否已移除
        if (state->isRemoved) {
            markDone();
            return;
        }

        // 更新位置
        setPosition(state->position);

        // 计算水平速度平方
        f32 horizontalSpeedSq = state->velocity.x * state->velocity.x + state->velocity.z * state->velocity.z;
        f32 horizontalSpeed = std::sqrt(horizontalSpeedSq);

        // 音量计算
        if (horizontalSpeed >= 0.01f) {
            // distance 用于平滑音量变化
            m_distance = mc::math::clamp(m_distance + 0.0025f, 0.0f, 1.0f);
            // 音量基于水平速度线性插值，范围 [0.0, 0.7]
            f32 t = mc::math::clamp(horizontalSpeed, 0.0f, 0.5f) / 0.5f;
            f32 volume = mc::math::lerp(0.0f, 0.7f, t);
            setVolume(volume);
        } else {
            m_distance = 0.0f;
            setVolume(0.0f);
        }
    } else {
        // 状态不存在，停止声音
        markDone();
    }
}

// ============================================================================
// RidingMinecartSoundStateful
// ============================================================================

RidingMinecartSoundStateful::RidingMinecartSoundStateful(
    const EntitySoundState& playerState, const EntitySoundState& minecartState, EntitySoundHandler* handler)
    : TickableSound(SoundEvents::ENTITY_MINECART_INSIDE,
          SoundCategory::Neutral,
          playerState.position,
          0.0f,                  // 初始音量为0
          1.0f,                  // 音调
          true,                  // 循环
          AttenuationType::None, // 无衰减（玩家内部声音）
          16.0f                  // 衰减距离（无意义）
          )
    , m_handler(handler)
    , m_playerId(playerState.entityId)
    , m_minecartId(minecartState.entityId)
{}

void RidingMinecartSoundStateful::tick()
{
    MC_ASSERT_RELEASE(m_handler);
    const EntitySoundState* playerState = m_handler->getEntityState(m_playerId);
    const EntitySoundState* minecartState = m_handler->getEntityState(m_minecartId);

    // 检查矿车是否被移除
    if (!minecartState || minecartState->isRemoved) {
        markDone();
        return;
    }

    // 检查玩家是否仍骑乘该矿车（参考 MC RidingEntitySoundInstance.tick）
    if (!playerState || playerState->isRemoved || !playerState->isRiding || playerState->vehicleId != m_minecartId) {
        markDone();
        return;
    }

    // 更新位置（跟随玩家）
    setPosition(playerState->position);

    // 计算水平速度
    f32 horizontalSpeedSq =
        minecartState->velocity.x * minecartState->velocity.x + minecartState->velocity.z * minecartState->velocity.z;
    f32 horizontalSpeed = std::sqrt(horizontalSpeedSq);

    // 音量计算
    if (horizontalSpeed >= 0.01f) {
        f32 volume = mc::math::clamp(horizontalSpeed, 0.0f, 1.0f) * 0.75f;
        setVolume(volume);
    } else {
        setVolume(0.0f);
    }
}

} // namespace mc::client::sound
