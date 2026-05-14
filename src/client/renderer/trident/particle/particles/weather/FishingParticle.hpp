#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 钓鱼粒子（水面涟漪效果）
 *
 * 参考 MC 1.16.5 FishingParticle
 *
 * 特性：
 * - 水面涟漪效果，向下移动
 * - 生命周期较短
 * - 用于钓鱼浮标的水面效果
 */
class FishingParticle : public Particle {
public:
    FishingParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/fishing");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    static constexpr f64 DEFAULT_SIZE = 0.04;
};

} // namespace mc::client::renderer::trident::particle::particles
