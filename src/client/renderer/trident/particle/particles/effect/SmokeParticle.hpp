#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 烟雾粒子
 *
 * 参考 MC 1.16.5 SmokeParticle
 *
 * 特性：
 * - 向上缓慢飘动
 * - 随机漂移
 * - 逐渐变大并淡出
 */
class SmokeParticle : public Particle {
public:
    SmokeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/smoke");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.0f;   // 烟雾不受重力
    static constexpr f64 DEFAULT_SIZE = 0.1f;
    static constexpr f64 DEFAULT_LIFETIME = 40.0f;  // 约 2 秒

    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
