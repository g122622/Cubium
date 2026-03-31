#pragma once

#include "../liquid/DripParticle.hpp"

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

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/drip_hang");
    }

    [[nodiscard]] u32 getLightColor(ClientWorld* world) const override;

protected:
    void onLand(ClientWorld* world) override;
};

} // namespace mc::client::renderer::trident::particle::particles
