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

    // MC 1.16.5: 速度缩放 0.009999999776482582 倍
    // 实际上是从 velocity 乘以 0.1 后加上随机偏移
    setGravity(DEFAULT_GRAVITY);  // 火焰不受重力
    setSize(DEFAULT_SIZE);
    m_initialSize = size();

    // 火焰颜色：橙黄色
    f64 colorVariation = rng.nextFloat() * 0.2f;
    setColor(glm::vec4(1.0f, 0.6f + static_cast<f32>(colorVariation), 0.1f, 1.0f));

    // MC 1.16.5: 速度摩擦 0.96
    setFriction(0.96f);
    setHasPhysics(false);  // 火焰粒子不做碰撞检测

    // MC 1.16.5: 生命周期 = (8.0 / (rand * 0.8 + 0.2)) + 4
    // 但这里我们使用更标准的 30 tick (约 1.5 秒)
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

    // 保存上一帧位置（用于插值）
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    // 生命周期递增
    m_age += 1.0f;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // MC 1.16.5 DeceleratingParticle.tick():
    // 速度衰减 0.96
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.y *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // MC 1.16.5 FlameParticle.move(): 直接移动，不做碰撞检测
    // 火焰粒子穿过方块
    m_position += m_velocity;

    // MC 1.16.5 FlameParticle.getScale():
    // float f = ((float)this.age + scaleFactor) / (float)this.maxAge;
    // return this.particleScale * (1.0F - f * f * 0.5F);
    // 这里在 tick 中更新 size
    f64 lifeRatio = m_age / m_maxAge;
    f64 scale = 1.0f - lifeRatio * lifeRatio * 0.5f;
    setSize(m_initialSize * scale);

    // MC 1.16.5: 在生命周期后半段淡出
    // SimpleAnimatedParticle.tick() 中:
    // if (age > maxAge / 2) {
    //     setAlphaF(1.0F - (float)(age - maxAge / 2) / (float)maxAge);
    // }
    if (lifeRatio > 0.5f) {
        m_color.a = static_cast<f32>(1.0f - (lifeRatio - 0.5f) * 2.0f);
    }
}

f64 FlameParticle::getScale(f64 partialTick) const {
    MC_UNUSED(partialTick);
    // 使用 size 属性直接控制大小，这里返回 1.0
    // 因为 size 已经在 tick() 中根据生命周期更新
    return 1.0f;
}

} // namespace mc::client::renderer::trident::particle::particles
