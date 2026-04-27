#pragma once

#include "DripParticle.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 水滴粒子
 *
 * 参考 MC 1.16.5 DripWaterParticle
 *
 * 特性：
 * - 从含水方块下方滴落
 * - 悬挂积累后下落
 * - 落地后可能产生水花
 */
class DripWaterParticle : public DripParticle {
public:
    DripWaterParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建水滴粒子（悬挂状态）
     */
    static std::unique_ptr<Particle> createDripping(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    /**
     * @brief 工厂方法：创建下落水滴粒子
     */
    static std::unique_ptr<Particle> createFalling(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/drip_hang");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override;

protected:
    void onLand(mc::client::ClientWorld* world) override;
};

} // namespace mc::client::renderer::trident::particle::particles
