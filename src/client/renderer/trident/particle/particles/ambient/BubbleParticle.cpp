#include "BubbleParticle.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::client::renderer::trident::particle::particles {

BubbleParticle::BubbleParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
{
    mc::math::Random rng;

    setGravity(DEFAULT_GRAVITY);  // 负重力 = 向上浮
    setSize(DEFAULT_SIZE * (0.8f + rng.nextFloat() * 0.4f));

    // 气泡颜色：淡蓝色半透明
    setColor(glm::vec4(0.8f, 0.9f, 1.0f, 0.6f));

    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME * (0.8f + rng.nextFloat() * 0.4f));
}

std::unique_ptr<Particle> BubbleParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<BubbleParticle>(pos, velocity);
}

void BubbleParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 气泡向上升起（负重力）
    m_velocity.y -= m_gravity * 0.04f;

    // 随机水平漂移
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.005f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.005f;

    m_position += m_velocity;
    m_velocity.x *= m_friction;
    m_velocity.z *= m_friction;

    // TODO: 检查是否到达水面
    // 如果不在水中则消失

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.8f) {
        m_color.a = 0.6f * (1.0f - (lifeRatio - 0.8f) / 0.2f);
    }
}

} // namespace mc::client::renderer::trident::particle::particles
