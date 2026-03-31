#include "RedstoneParticleData.hpp"
#include "../ParticleRegistry.hpp"
#include <sstream>
#include <iomanip>

namespace mc::client::renderer::trident::particle::data {

RedstoneParticleData::RedstoneParticleData(const glm::vec3& color)
    : m_color(color)
{
    // 颜色分量应在 0-1 范围内
    m_color = glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
}

String RedstoneParticleData::getTypeName() const {
    return ParticleRegistry::instance().getTypeName(ParticleTypeId::Redstone);
}

String RedstoneParticleData::getParameters() const {
    // 红石粒子参数格式: r g b
    // 例如: 1.0 0.0 0.0
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << m_color.r << " " << m_color.g << " " << m_color.b;
    return ss.str();
}

std::unique_ptr<ParticleData> RedstoneParticleData::clone() const {
    return std::make_unique<RedstoneParticleData>(m_color);
}

} // namespace mc::client::renderer::trident::particle::data
