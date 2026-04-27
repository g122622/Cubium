#include "LavaParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

LavaParticle::LavaParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));

    // 熔岩颜色：橙红色
    f64 colorVar = rng.nextFloat() * 0.2f;
    setColor(glm::vec4(1.0f, 0.3f + colorVar, 0.0f, 1.0f));

    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> LavaParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<LavaParticle>(pos, velocity);
}

void LavaParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 随机水平漂移
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.6f) {
        m_color.a = 1.0f - (lifeRatio - 0.6f) / 0.4f;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
