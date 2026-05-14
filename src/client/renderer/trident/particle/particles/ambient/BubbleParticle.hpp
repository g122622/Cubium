#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 气泡粒子
 *
 * 参考 MC 1.16.5 BubbleParticle
 *
 * 特性：
 * - 在水中生成，向上升起（浮力 0.005/tick）
 * - 到达水面后消失（应生成 BubblePop 粒子）
 * - 摩擦系数 0.85
 * - 生命周期约 8-40 tick
 */
class BubbleParticle : public Particle {
public:
    BubbleParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/bubble");
    }
};

} // namespace mc::client::renderer::trident::particle::particles
