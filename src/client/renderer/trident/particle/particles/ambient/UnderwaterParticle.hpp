#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 水下悬浮粒子
 *
 * 参考 MC 1.16.5 UnderwaterParticle
 *
 * 特性：
 * - 水下环境效果
 * - 缓慢漂浮
 * - 半透明淡蓝色
 */
class UnderwaterParticle : public Particle {
public:
    UnderwaterParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/underwater");
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.0f;
    static constexpr f32 DEFAULT_SIZE = 0.03f;
    static constexpr f32 DEFAULT_LIFETIME = 60.0f;

    f32 m_initialAlpha;
};

} // namespace mc::client::renderer::trident::particle::particles
