#include "common/sound/SoundEvent.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::sound {

SoundEvent::SoundEvent(ResourceLocation id)
    : m_id(std::move(id))
    , m_attenuationDistance(DEFAULT_ATTENUATION_DISTANCE)
{}

SoundEvent::SoundEvent(std::string_view idString)
    : m_id(ResourceLocation::parse(idString))
    , m_attenuationDistance(DEFAULT_ATTENUATION_DISTANCE)
{}

void SoundEvent::setAttenuationDistance(f32 distance) noexcept
{
    MC_ASSERT_MSG(distance > 0.0f, "Attenuation distance must be positive");
    m_attenuationDistance = distance;
}

} // namespace mc::sound
