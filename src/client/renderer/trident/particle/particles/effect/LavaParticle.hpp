#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 熔岩滴粒子
 *
 * 参考 MC 1.16.5 LavaParticle
 *
 * 特性：
 * - 发光粒子（不受世界光照影响）
 * - 从熔岩表面滴落
 * - 橙红色
 */
class LavaParticle : public Particle {
public:
    LavaParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_LIT; // 发光
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/lava");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override
    {
        MC_UNUSED(world);
        return 0xF0; // 始终最大亮度
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.025f;
    static constexpr f64 DEFAULT_SIZE = 0.05f;
    static constexpr f64 DEFAULT_LIFETIME = 30.0f;
};

} // namespace mc::client::renderer::trident::particle::particles
