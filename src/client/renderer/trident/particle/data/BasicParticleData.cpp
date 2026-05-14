#include "BasicParticleData.hpp"
#include "../ParticleRegistry.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::data {

BasicParticleData::BasicParticleData(ParticleTypeId type)
    : m_type(type)
{
    MC_ASSERT(isValidParticleType(type));
}

BasicParticleData::BasicParticleData(const std::string& typeName)
{
    auto id = ParticleRegistry::instance().getTypeId(typeName);
    if (id.has_value()) {
        m_type = id.value();
    } else {
        m_type = ParticleTypeId::Invalid;
    }
}

std::string BasicParticleData::getTypeName() const
{
    return ParticleRegistry::instance().getTypeName(m_type);
}

std::unique_ptr<ParticleData> BasicParticleData::clone() const
{
    return std::make_unique<BasicParticleData>(m_type);
}

} // namespace mc::client::renderer::trident::particle::data
