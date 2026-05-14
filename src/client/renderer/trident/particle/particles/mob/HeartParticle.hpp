#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 爱心粒子
 *
 * 参考 MC 1.16.5 HeartParticle
 *
 * 特性：
 * - 生物繁殖或驯服时显示
 * - 红色心形
 * - 向上飘动后消失
 */
class HeartParticle : public Particle {
public:
    HeartParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/heart");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0f;
    static constexpr f64 DEFAULT_SIZE = 0.1f;
    static constexpr f64 DEFAULT_LIFETIME = 20.0f; // 约 1 秒
};

} // namespace mc::client::renderer::trident::particle::particles
