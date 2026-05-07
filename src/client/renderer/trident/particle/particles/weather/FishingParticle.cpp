#include "FishingParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

FishingParticle::FishingParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    // MC 1.16.5: 钓鱼粒子向下移动
    setGravity(0.0);  // 无重力
    setSize(DEFAULT_SIZE * (0.5f + rng.nextFloat() * 0.5f));
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.6f));  // 淡蓝色半透明

    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.5f + rng.nextFloat()));
}

std::unique_ptr<Particle> FishingParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FishingParticle>(pos, velocity);
}

void FishingParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向下移动
    m_position += m_velocity;

    // 应用摩擦
    m_velocity.x *= m_friction;
    m_velocity.y *= m_friction;
    m_velocity.z *= m_friction;

    // 淡出效果
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.3f) {
        m_color.a = 0.6f * (1.0f - (lifeRatio - 0.3f) / 0.7f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
