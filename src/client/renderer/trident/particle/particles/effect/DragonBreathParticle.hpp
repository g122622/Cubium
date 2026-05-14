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
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
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
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/end_rod");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 60.0;
    f64 m_initialSize;
    f32 m_brightness; ///< 初始亮度
};

/**
 * @brief 扫荡攻击粒子
 *
 * 参考 MC 1.16.5 SweepAttackParticle
 *
 * 特性：
 * - 固定生命周期 4 tick
 * - 发光粒子（最大亮度 15728880）
 * - 根据年龄选择纹理帧（4帧动画）
 * - 无运动
 * - 缩放随 xSpeed 参数变化
 */
class SweepAttackParticle : public Particle {
public:
    SweepAttackParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::PARTICLE_SHEET_LIT; }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        // MC 1.16.5: 固定高亮度 15728880 (blockLight=15, skyLight=15)
        return 15728880;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    f64 m_scaleMultiplier;
};

} // namespace mc::client::renderer::trident::particle::particles
