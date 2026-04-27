#include "FlameParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

FlameParticle::FlameParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);
    setSize(DEFAULT_SIZE * (0.6f + rng.nextFloat() * 0.4f));
    m_initialSize = size();

    // 火焰颜色：橙黄色
    f64 colorVariation = rng.nextFloat() * 0.2f;
    setColor(glm::vec4(1.0f, 0.6f + colorVariation, 0.1f, 1.0f));

    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> FlameParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<FlameParticle>(pos, velocity);
}

void FlameParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 火焰向上飘动并随机摇摆
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.01f;

    // 火焰向上漂浮
    m_velocity.y += 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄缩小
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f - lifeRatio * 0.5f));

    // 淡出
    if (lifeRatio > 0.5f) {
        m_color.a = 1.0f - (lifeRatio - 0.5f) * 2.0f;
    }
}

f64 FlameParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    // 使用 size 属性直接控制大小
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
