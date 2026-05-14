#include "PoofParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

PoofParticle::PoofParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 8.0);

    // 灰白色烟雾
    f32 gray = 0.7f + rng.nextFloat() * 0.3f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    // 初始速度添加随机分量
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.1f;
    m_velocity.y += rng.nextFloat() * 0.05f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.1f;
}

std::unique_ptr<Particle> PoofParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<PoofParticle>(pos, velocity);
}

void PoofParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 轻微随机运动
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.005f;
    m_velocity.y += 0.001f; // 轻微上升
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.005f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄缩小并淡出
    f64 lifeRatio = m_age / m_maxAge;
    setSize(m_initialSize * (1.0f - lifeRatio * 0.75f));

    // 淡出
    m_color.a = static_cast<f32>(1.0f - lifeRatio);
}

f64 PoofParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
