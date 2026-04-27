#include "SmokeParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

SmokeParticle::SmokeParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));
    m_initialSize = size();

    // 灰色烟雾
    f64 gray = 0.3f + rng.nextFloat() * 0.2f;
    setColor(glm::vec4(gray, gray, gray, 0.8f));

    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + rng.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> SmokeParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SmokeParticle>(pos, velocity);
}

void SmokeParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 烟雾向上缓慢飘动
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.005f;
    m_velocity.y += 0.001f;  // 轻微向上

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄变大并淡出
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0f + lifeRatio * 2.0f;
    setSize(m_initialSize * scale);

    // 淡出
    m_color.a = 0.8f * (1.0f - lifeRatio);
}

f64 SmokeParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
