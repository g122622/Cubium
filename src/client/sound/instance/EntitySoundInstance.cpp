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

#include "client/sound/instance/EntitySoundInstance.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

EntitySoundInstance::EntitySoundInstance(
    const ResourceLocation& soundEventId, SoundCategory category, Entity& entity, f32 volume, f32 pitch)
    : TickableSound(soundEventId,
          category,
          glm::vec3(entity.x(), entity.y(), entity.z()),
          volume,
          pitch,
          false, // 默认不循环
          AttenuationType::Linear,
          DEFAULT_ATTENUATION_DISTANCE)
    , m_entity(entity)
{
    // 初始化位置为实体当前位置
    setPosition(glm::vec3(entity.x(), entity.y(), entity.z()));
}

void EntitySoundInstance::tick()
{
    // 检查实体是否仍然有效
    if (!isEntityValid()) {
        markDone();
        return;
    }

    // 更新位置跟随实体
    setPosition(glm::vec3(m_entity.x(), m_entity.y(), m_entity.z()));
}

bool EntitySoundInstance::isEntityValid() const
{
    // 实体被移除
    if (m_entity.isRemoved()) {
        return false;
    }

    // 实体静音
    if (m_entity.isSilent()) {
        return false;
    }

    return true;
}

} // namespace mc::client::sound
