#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 爆炸粒子
 *
 * 参考 MC 1.16.5 ExplosionParticle
 *
 * 特性：
 * - 发光粒子（最大亮度）
 * - 快速扩大
 * - 快速淡出
 *
 * 用法：
 * @code
 * auto explosion = std::make_unique<ExplosionParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f)
 * );
 * particleManager.addParticle(std::move(explosion));
 * @endcode
 */
class ExplosionParticle : public Particle {
public:
    /**
     * @brief 构造爆炸粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    ExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_LIT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/explosion");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 6.0;  // 约 0.3 秒
    static constexpr f64 EXPLOSION_LIGHT = 15728880.0;  // MC 爆炸亮度

    f64 m_initialSize;
};

/**
 * @brief 大型爆炸粒子
 *
 * 参考 MC 1.16.5 LargeExplosionParticle
 * 更大的爆炸效果，持续时间稍长。
 */
class LargeExplosionParticle : public Particle {
public:
    LargeExplosionParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_LIT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/explosion_emitter");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 10.0;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
