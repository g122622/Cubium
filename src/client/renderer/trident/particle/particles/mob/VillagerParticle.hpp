#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 愤怒村民粒子
 *
 * 参考 MC 1.16.5 AngryVillagerParticle
 *
 * 特性：
 * - 灰色/深灰色烟雾
 * - 向上漂浮
 * - 快速淡出
 */
class AngryVillagerParticle : public Particle {
public:
    AngryVillagerParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/angry_villager");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 开心村民粒子
 *
 * 参考 MC 1.16.5 HappyVillagerParticle
 *
 * 特性：
 * - 绿色星星/心形
 * - 向上漂浮
 * - 快速淡出
 */
class HappyVillagerParticle : public Particle {
public:
    HappyVillagerParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/happy_villager");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 10.0;
    f64 m_initialSize;
};

/**
 * @brief 喷嚏粒子（熊猫）
 *
 * 参考 MC 1.16.5 SneezeParticle
 *
 * 特性：
 * - 淡绿色水滴
 * - 向前喷射
 * - 重力影响
 */
class SneezeParticle : public Particle {
public:
    SneezeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/sneeze");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.02f;
    static constexpr f64 DEFAULT_LIFETIME = 10.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
