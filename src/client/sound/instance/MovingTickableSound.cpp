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

#include "client/sound/instance/MovingTickableSound.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/handler/EntitySoundHandler.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

MovingTickableSound::MovingTickableSound(const ResourceLocation& soundEventId,
    SoundCategory category,
    const EntitySoundHandler* handler,
    EntityInstanceId entityId,
    f32 volume,
    f32 pitch)
    : TickableSound(soundEventId,
          category,
          glm::vec3(0.0f), // 初始位置，将在tick中更新
          volume,
          pitch,
          true, // 循环
          AttenuationType::Linear,
          DEFAULT_ATTENUATION_DISTANCE)
    , m_handler(handler)
    , m_entityId(entityId)
{}

void MovingTickableSound::tick()
{
    // 检查处理器是否有效
    if (!m_handler) {
        markDone();
        return;
    }

    // 获取实体状态
    const EntitySoundState* state = m_handler->getEntityState(m_entityId);
    if (!state || state->isRemoved) {
        // 实体不存在或已移除，停止声音
        markDone();
        return;
    }

    // 更新声音位置
    setPosition(state->position);
}

} // namespace mc::client::sound
