#include "DragonBreathParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// DragonBreathParticle
// ============================================================================

DragonBreathParticle::DragonBreathParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
    , m_hasLanded(false)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.05f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 10.0);

    // 紫色龙息颜色
    f32 purple = 0.8f + rng.nextFloat() * 0.2f;
    setColor(glm::vec4(purple * 0.6f, 0.0f, purple, 1.0f));

    // 添加随机初始速度
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.1f;
    m_velocity.y += rng.nextFloat() * 0.1f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.1f;
}

std::unique_ptr<Particle> DragonBreathParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<DragonBreathParticle>(pos, velocity);
}

void DragonBreathParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 随机漂浮运动
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.01f;
    m_velocity.y += (rng.nextFloat() - 0.5f) * 0.01f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.01f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 随年龄淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(1.0f - lifeRatio * lifeRatio);

    // 轻微缩小
    setSize(m_initialSize * (1.0f - lifeRatio * 0.3f));
}

f64 DragonBreathParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// EndRodParticle
// ============================================================================

EndRodParticle::EndRodParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.02f)
    , m_brightness(0.95f)
{
    mc::math::Random rng;

    setGravity(-5e-4f);  // 轻微向上浮动
    setSize(0.02f + rng.nextFloat() * 0.01f);
    m_initialSize = size();
    setFriction(0.98f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 12.0);

    // 白色/淡黄色
    m_brightness = 0.95f + rng.nextFloat() * 0.05f;
    setColor(glm::vec4(m_brightness, m_brightness, 1.0f, 1.0f));

    // 向上运动
    m_velocity.y -= 0.02f;  // 负重力让粒子向上
}

std::unique_ptr<Particle> EndRodParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EndRodParticle>(pos, velocity);
}

void EndRodParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用向上的"重力"
    m_velocity.y -= static_cast<f32>(m_gravity * 0.04);

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 颜色渐变淡出
    f64 lifeRatio = m_age / m_maxAge;
    f64 fade = 1.0f - lifeRatio;
    m_color.a = static_cast<f32>(fade);
    m_color.r = static_cast<f32>(m_brightness * fade);
    m_color.g = static_cast<f32>(m_brightness * fade);
}

f64 EndRodParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// SweepAttackParticle
// ============================================================================

SweepAttackParticle::SweepAttackParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.5f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.5f);
    m_initialSize = size();
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME);

    // 白色/淡蓝色
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 设置旋转
    setRoll(rng.nextFloat() * 3.14159 * 2.0);
}

std::unique_ptr<Particle> SweepAttackParticle::create(
    const glm::vec3& pos,
    const glm::vec3& velocity,
    mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<SweepAttackParticle>(pos, velocity);
}

void SweepAttackParticle::tick(mc::client::ClientWorld* world) {
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    m_position += m_velocity;

    // 快速扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0f + lifeRatio * 2.0f;
    setSize(m_initialSize * expansion);

    // 快速淡出
    m_color.a = static_cast<f32>(1.0f - lifeRatio * lifeRatio * lifeRatio);
}

f64 SweepAttackParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
