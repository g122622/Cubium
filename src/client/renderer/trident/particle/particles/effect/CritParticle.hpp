#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 暴击粒子
 *
 * 参考 MC 1.16.5 CritParticle
 *
 * 特性：
 * - 攻击实体时产生
 * - 向目标方向冲刺
 * - 淡黄色
 */
class CritParticle : public Particle {
public:
    CritParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/crit");
    }

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.0f;
    static constexpr f32 DEFAULT_SIZE = 0.04f;
    static constexpr f32 DEFAULT_LIFETIME = 20.0f;

    f32 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
