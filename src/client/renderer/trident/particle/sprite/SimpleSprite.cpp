#include "SimpleSprite.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle {

SimpleSprite::SimpleSprite(const glm::vec2& uvMin, const glm::vec2& uvMax)
    : m_uvMin(uvMin)
    , m_uvMax(uvMax)
{
}

glm::vec4 SimpleSprite::getFrameUV(f64 age, f64 maxAge) const {
    MC_UNUSED(age);
    MC_UNUSED(maxAge);
    return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
}

glm::vec4 SimpleSprite::getRandomFrameUV(u32 seed) const {
    MC_UNUSED(seed);
    return glm::vec4(m_uvMin.x, m_uvMin.y, m_uvMax.x, m_uvMax.y);
}

} // namespace mc::client::renderer::trident::particle
