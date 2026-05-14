#include "client/sound/instance/SoundInstance.hpp"

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
