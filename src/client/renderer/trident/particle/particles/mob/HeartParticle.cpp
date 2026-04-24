#include "HeartParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

HeartParticle::HeartParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));

    // 爱心颜色：红色
    setColor(glm::vec4(1.0f, 0.2f, 0.2f, 1.0f));

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));

    // 爱心向上飘动
    m_velocity.y = 0.02f + rng.nextFloat() * 0.01f;
}

std::unique_ptr<Particle> HeartParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<HeartParticle>(pos, velocity);
}

void HeartParticle::tick(ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 爱心向上飘动
    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.5f) {
        m_color.a = 1.0f - (lifeRatio - 0.5f) * 2.0f;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
