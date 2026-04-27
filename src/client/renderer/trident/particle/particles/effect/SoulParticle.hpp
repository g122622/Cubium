#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 灵魂火焰粒子
 *
 * 参考 MC 1.16.5 SoulFireFlameParticle
 *
 * 特性：
 * - 蓝色发光粒子
 * - 向上飘动
 * - 随年龄变小淡出
 */
class SoulFireFlameParticle : public Particle {
public:
    SoulFireFlameParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_LIT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/soul_fire_flame");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 30.0;
    f64 m_initialSize;
};

/**
 * @brief 灵魂粒子
 *
 * 参考 MC 1.16.5 SoulParticle
 *
 * 特性：
 * - 蓝色半透明
 * - 向上飘动
 * - 随年龄淡出
 */
class SoulParticle : public Particle {
public:
    SoulParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/soul");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 12.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
