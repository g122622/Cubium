#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 红石粉尘粒子
 *
 * 参考 MC 1.16.5 RedstoneParticle
 *
 * 特性：
 * - 可自定义颜色
 * - 发光粒子
 * - 无重力
 */
class RedstoneParticle : public Particle {
public:
    /**
     * @brief 构造红石粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param color 粒子颜色（ARGB）
     */
    RedstoneParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/redstone");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 附魔台符文粒子
 *
 * 参考 MC 1.16.5 EnchantmentTableParticle
 *
 * 特性：
 * - 发光粒子
 * - 向目标位置曲线运动
 * - 淡出
 */
class EnchantParticle : public Particle {
public:
    EnchantParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/enchant");
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
 * @brief 下落灰尘粒子
 *
 * 参考 MC 1.16.5 FallingDustParticle
 *
 * 特性：
 * - 从方块颜色获取颜色
 * - 受重力影响
 * - 旋转下落
 */
class FallingDustParticle : public Particle {
public:
    FallingDustParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/falling_dust");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.003f;
    static constexpr f64 DEFAULT_LIFETIME = 29.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
