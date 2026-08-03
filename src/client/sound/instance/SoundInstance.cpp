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

#include "client/sound/instance/SoundInstance.hpp"
#include "client/sound/instance/ISoundInstance.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <glm/ext/vector_float3.hpp>

namespace mc::client::sound {

SoundInstance::SoundInstance(const ResourceLocation& soundEventId,
    SoundCategory category,
    const glm::vec3& position,
    f32 volume,
    f32 pitch,
    bool looping,
    AttenuationType attenuation,
    f32 attenuationDistance)
    : m_soundEventId(soundEventId)
    , m_category(category)
    , m_position(position)
    , m_volume(volume)
    , m_pitch(pitch)
    , m_looping(looping)
    , m_attenuationType(attenuation)
    , m_attenuationDistance(attenuationDistance)
{}

SoundInstance SoundInstance::createGlobal(
    const ResourceLocation& soundEventId, SoundCategory category, f32 volume, f32 pitch)
{
    return SoundInstance(soundEventId,
        category,
        glm::vec3(0.0f),
        volume,
        pitch,
        false,                 // 不循环
        AttenuationType::None, // 无衰减
        0.0f                   // 衰减距离为0
    );
}

SoundInstance SoundInstance::createLocated(
    const ResourceLocation& soundEventId, SoundCategory category, f32 x, f32 y, f32 z, f32 volume, f32 pitch)
{
    return SoundInstance(soundEventId,
        category,
        glm::vec3(x, y, z),
        volume,
        pitch,
        false,                   // 不循环
        AttenuationType::Linear, // 线性衰减
        DEFAULT_ATTENUATION_DISTANCE);
}

SoundInstance SoundInstance::createMusic(const ResourceLocation& soundEventId, f32 volume, f32 pitch)
{
    return SoundInstance(soundEventId,
        SoundCategory::Music,
        glm::vec3(0.0f),
        volume,
        pitch,
        false,                 // 不循环（音乐自己会循环或结束）
        AttenuationType::None, // 全局
        0.0f);
}

SoundInstance SoundInstance::createRecord(const ResourceLocation& soundEventId, f32 x, f32 y, f32 z)
{
    return SoundInstance(soundEventId,
        SoundCategory::Records,
        glm::vec3(x, y, z),
        4.0f,                  // 唱片机音量较大
        1.0f,                  // 正常音调
        true,                  // 循环播放
        AttenuationType::None, // 无衰减（但会受距离影响）
        64.0f                  // 大衰减距离
    );
}

} // namespace mc::client::sound
