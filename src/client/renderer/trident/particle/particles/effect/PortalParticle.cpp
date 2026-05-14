#include "PortalParticle.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

PortalParticle::PortalParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_startX(pos.x)
    , m_startZ(pos.z)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));

    // 传送门颜色：紫色
    f64 purple = 0.6f + rng.nextFloat() * 0.4f;
    setColor(glm::vec4(0.4f, 0.1f, purple, 0.8f));

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + rng.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> PortalParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<PortalParticle>(pos, velocity);
}

void PortalParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 传送门粒子向下飘落，带有水平摆动
    f64 ageRatio = m_age / m_maxAge;

    // 水平摆动
    f64 swing = std::sin(ageRatio * mc::math::PI * 4.0f) * 0.05f;
    m_position.x = m_startX + swing;
    m_position.z = m_startZ + std::cos(ageRatio * mc::math::PI * 4.0f) * 0.05f;

    // 向下移动
    m_position.y += m_velocity.y;

    // 旋转
    m_roll += 0.1f;

    // 淡出
    if (ageRatio > 0.5f) {
        m_color.a = 0.8f * (1.0f - (ageRatio - 0.5f) * 2.0f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
