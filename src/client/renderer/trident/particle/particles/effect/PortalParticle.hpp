#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 传送门粒子
 *
 * 参考 MC 1.16.5 PortalParticle
 *
 * 特性：
 * - 紫色半透明
 * - 向下缓慢飘落
 * - 随机旋转
 */
class PortalParticle : public Particle {
public:
    PortalParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/portal");
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.0f;
    static constexpr f32 DEFAULT_SIZE = 0.04f;
    static constexpr f32 DEFAULT_LIFETIME = 50.0f;  // 约 2.5 秒

    f32 m_startX;  ///< 初始 X 位置（用于水平摆动）
    f32 m_startZ;  ///< 初始 Z 位置
};

} // namespace mc::client::renderer::trident::particle::particles
