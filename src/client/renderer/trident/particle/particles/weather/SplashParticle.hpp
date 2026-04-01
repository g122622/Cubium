#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 溅射粒子
 *
 * 参考 MC 1.16.5 SplashParticle
 *
 * 特性：
 * - 雨滴落地或实体落水时产生
 * - 向上喷射后下落
 * - 半透明白色
 */
class SplashParticle : public Particle {
public:
    SplashParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/splash");
    }

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.04f;
    static constexpr f64 DEFAULT_SIZE = 0.05f;
    static constexpr f64 DEFAULT_LIFETIME = 15.0f;
};

} // namespace mc::client::renderer::trident::particle::particles
