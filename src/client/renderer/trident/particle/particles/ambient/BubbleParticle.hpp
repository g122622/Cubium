#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 气泡粒子
 *
 * 参考 MC 1.16.5 BubbleParticle
 *
 * 特性：
 * - 在水中生成，向上升起
 * - 到达水面后消失（变成 BubblePop）
 * - 半透明蓝色
 */
class BubbleParticle : public Particle {
public:
    BubbleParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/bubble");
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = -0.008f;  // 负重力 = 向上
    static constexpr f32 DEFAULT_SIZE = 0.05f;
    static constexpr f32 DEFAULT_LIFETIME = 40.0f;
};

} // namespace mc::client::renderer::trident::particle::particles
