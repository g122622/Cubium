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
    , m_scaleMultiplier(1.0)
{
    mc::math::Random rng;

    // MC 1.16.5: 固定生命周期 4 tick
    setMaxAge(4.0);

    // MC 1.16.5: 颜色为随机灰白色
    f32 gray = rng.nextFloat() * 0.6f + 0.4f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    // MC 1.16.5: scale = 1.0 - xSpeed * 0.5
    m_scaleMultiplier = 1.0 - velocity.x * 0.5;

    // 无重力、无摩擦、无碰撞
    setGravity(0.0f);
    setFriction(1.0f);
    setHasPhysics(false);

    // MC 1.16.5: 无运动
    m_velocity = glm::vec3(0.0f);
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

    // MC 1.16.5: 无运动，仅更新纹理帧
}

ResourceLocation SweepAttackParticle::getTextureLocation() const {
    // MC 1.16.5: 根据年龄选择纹理帧（4帧）
    i32 frame = static_cast<i32>(m_age);
    frame = std::min(frame, 3);
    return ResourceLocation("minecraft:particle/sweep_" + std::to_string(frame));
}

f64 SweepAttackParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    // MC 1.16.5: scale = 1.0 - xSpeed * 0.5
    return m_scaleMultiplier;
}

} // namespace mc::client::renderer::trident::particle::particles
