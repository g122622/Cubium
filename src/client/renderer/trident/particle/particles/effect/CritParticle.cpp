#include "CritParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

CritParticle::CritParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));
    m_initialSize = size();

    // 暴击颜色：淡黄色
    setColor(glm::vec4(1.0f, 0.9f, 0.5f, 1.0f));

    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.7f + rng.nextFloat() * 0.6f));
}

std::unique_ptr<Particle> CritParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<CritParticle>(pos, velocity);
}

void CritParticle::tick(ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 暴击粒子保持速度，略有阻力
    m_position += m_velocity;
    m_velocity *= m_friction;

    // 旋转
    m_roll += 0.3f;

    // 随年龄变大
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f + lifeRatio * 0.5f));

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = 1.0f - (lifeRatio - 0.5f) * 2.0f;
    }
}

} // namespace mc::client::renderer::trident::particle::particles
