#include "SplashParticle.hpp"
#include "../../../../../../common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

SplashParticle::SplashParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.5f + rng.nextFloat() * 0.5f));
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.7f));  // 淡蓝色半透明

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + rng.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> SplashParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SplashParticle>(pos, velocity);
}

void SplashParticle::tick(ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= m_gravity * 0.04f;

    // 限制速度
    if (m_velocity.y < -0.3f) {
        m_velocity.y = -0.3f;
    }

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5f) {
        m_color.a = 0.7f * (1.0f - (lifeRatio - 0.5f) / 0.5f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
