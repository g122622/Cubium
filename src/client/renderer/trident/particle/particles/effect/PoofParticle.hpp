#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 消散粒子
 *
 * 参考 MC 1.16.5 PoofParticle
 *
 * 特性：
 * - 轻微向上飘动
 * - 随机运动
 * - 随年龄缩小并淡出
 */
class PoofParticle : public Particle {
public:
    PoofParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/poof");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 18.0;

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
