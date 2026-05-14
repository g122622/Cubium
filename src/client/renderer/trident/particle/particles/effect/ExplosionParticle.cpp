#include "ExplosionParticle.hpp"
#include "common/util/math/random/Random.hpp"
#include <cmath>

namespace mc::client::renderer::trident::particle::particles {

// ============================================================================
// ExplosionParticle
// ============================================================================

ExplosionParticle::ExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(1.0f)
{
    mc::math::Random rng;

    setGravity(0.0f);
    setSize(1.0f + rng.nextFloat() * 0.5f);
    m_initialSize = size();
    setFriction(1.0f);
    setHasPhysics(false);
    setMaxAge(DEFAULT_LIFETIME + rng.nextFloat() * 4.0);

    // 爆炸颜色：亮白色/黄色
    setColor(glm::vec4(1.0f, 0.9f + rng.nextFloat() * 0.1f, 0.7f + rng.nextFloat() * 0.3f, 1.0f));
}

std::unique_ptr<Particle> ExplosionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<ExplosionParticle>(pos, velocity);
}

void ExplosionParticle::tick(mc::client::ClientWorld* world)
{
    MC_UNUSED(world);

    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 爆炸粒子静止并扩大
    m_position += m_velocity;

    // 随年龄快速扩大
    f64 lifeRatio = m_age / m_maxAge;
    f64 expansion = 1.0f + lifeRatio * 3.0f;
    setSize(m_initialSize * expansion);

    // 快速淡出
    m_color.a = static_cast<f32>(1.0f - lifeRatio * lifeRatio);
}

f64 ExplosionParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0f;
}

// ============================================================================
// LargeExplosionParticle
// ============================================================================

LargeExplosionParticle::LargeExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(2.0f)
{
    mc::math::Random rng;

    // MC 1.16.5: maxAge = 6 + rand(4)
    setMaxAge(6.0 + rng.nextInt(4));

    // MC 1.16.5: 颜色为随机灰白色
    f32 gray = rng.nextFloat() * 0.6f + 0.4f;
    setColor(glm::vec4(gray, gray, gray, 1.0f));

    // MC 1.16.5: scale = 2.0 * (1.0 - xSpeed * 0.5)
    m_initialSize = 2.0 * (1.0 - velocity.x * 0.5);

    setGravity(0.0f);
    setSize(static_cast<f32>(m_initialSize));
    setFriction(1.0f);
    setHasPhysics(false);

    // MC 1.16.5: 无运动
    m_velocity = glm::vec3(0.0f);
}

std::unique_ptr<Particle> LargeExplosionParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    return std::make_unique<LargeExplosionParticle>(pos, velocity);
}

void LargeExplosionParticle::tick(mc::client::ClientWorld* world)
{
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

ResourceLocation LargeExplosionParticle::getTextureLocation() const
{
    // MC 1.16.5: 根据年龄选择纹理帧
    i32 frame = static_cast<i32>((m_age / m_maxAge) * 4.0);
    frame = std::min(frame, 3);
    return ResourceLocation("minecraft:particle/explosion_" + std::to_string(frame));
}

f64 LargeExplosionParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    // MC 1.16.5: scale = 2.0 * (1.0 - xSpeed * 0.5)
    return m_initialSize;
}

} // namespace mc::client::renderer::trident::particle::particles
