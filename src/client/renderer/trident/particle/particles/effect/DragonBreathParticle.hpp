#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 末影龙息粒子
 *
 * 参考 MC 1.16.5 DragonBreathParticle
 *
 * 特性：
 * - 紫色粒子
 * - 无物理碰撞
 * - 着地后继续漂浮
 */
class DragonBreathParticle : public Particle {
public:
    DragonBreathParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/dragon_breath");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    f64 m_initialSize;
    bool m_hasLanded;
};

/**
 * @brief 末地烛粒子
 *
 * 参考 MC 1.16.5 EndRodParticle
 *
 * 特性：
 * - 白色发光粒子
 * - 向上漂浮
 * - 颜色渐变淡出
 */
class EndRodParticle : public Particle {
public:
    EndRodParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/end_rod");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 60.0;
    f64 m_initialSize;
    f32 m_brightness;  ///< 初始亮度
};

/**
 * @brief 扫荡攻击粒子
 *
 * 参考 MC 1.16.5 SweepAttackParticle
 *
 * 特性：
 * - 扇形攻击效果
 * - 快速扩大
 * - 快速淡出
 */
class SweepAttackParticle : public Particle {
public:
    SweepAttackParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/sweep");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 4.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
