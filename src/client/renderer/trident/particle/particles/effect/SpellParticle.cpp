#include "SpellParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// SpellParticle
// ============================================================================

SpellParticle::SpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.04f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 设置颜色，添加一点随机性
    f32 r = color.r * (0.8f + rng.nextFloat() * 0.2f);
    f32 g = color.g * (0.8f + rng.nextFloat() * 0.2f);
    f32 b = color.b * (0.8f + rng.nextFloat() * 0.2f);
    setColor(glm::vec4(r, g, b, 0.8f));

    // 轻微向上漂浮
    m_velocity.y += 0.01f;
}

std::unique_ptr<Particle> SpellParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 默认紫色的药水粒子
    return std::make_unique<SpellParticle>(pos, velocity, glm::vec4(0.5f, 0.0f, 1.0f, 1.0f));
}

void SpellParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 向上漂浮
    m_velocity.y += 0.002f;

    // 随机漂移
    mc::math::Random rng;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    // 淡出
    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

f64 SpellParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// InstantSpellParticle
// ============================================================================

InstantSpellParticle::InstantSpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 设置颜色，更亮
    setColor(glm::vec4(color.r * 1.2f, color.g * 1.2f, color.b * 1.2f, 1.0f));
    m_velocity.y += 0.02f;
}

std::unique_ptr<Particle> InstantSpellParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<InstantSpellParticle>(pos, velocity, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void InstantSpellParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    m_velocity.y += 0.005f;
    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(1.0f - lifeRatio);
}

f64 InstantSpellParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// EntityEffectParticle
// ============================================================================

EntityEffectParticle::EntityEffectParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.03f);
    m_initialSize = size();
    setFriction(0.95f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    setColor(color);
}

std::unique_ptr<Particle> EntityEffectParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<EntityEffectParticle>(pos, velocity, glm::vec4(0.5f, 0.0f, 0.5f, 0.5f));
}

void EntityEffectParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 漂浮并漂移
    mc::math::Random rng;
    m_velocity.y += 0.002f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.003f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.003f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.5f * (1.0f - lifeRatio));
}

f64 EntityEffectParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// AmbientEntityEffectParticle
// ============================================================================

AmbientEntityEffectParticle::AmbientEntityEffectParticle(
    const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.02f);
    m_initialSize = size();
    setFriction(0.98f); // 更高的摩擦，更慢
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 6.0);

    // 更透明的颜色
    setColor(glm::vec4(color.r, color.g, color.b, 0.3f));
}

std::unique_ptr<Particle> AmbientEntityEffectParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<AmbientEntityEffectParticle>(pos, velocity, glm::vec4(0.5f, 0.5f, 1.0f, 0.3f));
}

void AmbientEntityEffectParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 很慢的漂浮
    mc::math::Random rng;
    m_velocity.y += 0.001f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.001f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.001f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.3f * (1.0f - lifeRatio));
}

f64 AmbientEntityEffectParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// WitchParticle
// ============================================================================

WitchParticle::WitchParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(0.1f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(0.1f + rng.nextFloat() * 0.04f);
    m_initialSize = size();
    setFriction(0.96f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 紫色
    f32 purpleIntensity = 0.6f + rng.nextFloat() * 0.4f;
    setColor(glm::vec4(purpleIntensity * 0.8f, 0.0f, purpleIntensity, 0.8f));

    m_velocity.y += 0.01f;
}

std::unique_ptr<Particle> WitchParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<WitchParticle>(pos, velocity);
}

void WitchParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    mc::math::Random rng;
    m_velocity.y += 0.002f;
    m_velocity.x += (rng.nextFloat() - 0.5f) * 0.002f;
    m_velocity.z += (rng.nextFloat() - 0.5f) * 0.002f;

    m_position += m_velocity;
    m_velocity *= m_friction;

    f64 lifeRatio = m_age / m_maxAge;
    m_color.a = static_cast<f32>(0.8f * (1.0f - lifeRatio));
}

} // namespace mc::client::renderer::trident::particle::particles
