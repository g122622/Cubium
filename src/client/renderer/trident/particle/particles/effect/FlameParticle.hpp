#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 火焰粒子
 *
 * 参考 MC 1.16.5 FlameParticle
 *
 * 特性：
 * - 发光粒子（不受世界光照影响）
 * - 向上飘动
 * - 随年龄变小
 * - 淡出消失
 *
 * 用法：
 * @code
 * auto flame = std::make_unique<FlameParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f, 0.05f, 0.0f)
 * );
 * particleManager.addParticle(std::move(flame));
 * @endcode
 */
class FlameParticle : public Particle {
public:
    /**
     * @brief 构造火焰粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    FlameParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_LIT;  // 发光粒子
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/flame");
    }

    [[nodiscard]] u32 getLightColor(ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;  // 始终最大亮度
    }

    [[nodiscard]] f32 getScale(f32 partialTick) const override;

private:
    static constexpr f32 DEFAULT_GRAVITY = 0.0f;  // 火焰不受重力
    static constexpr f32 DEFAULT_SIZE = 0.04f;
    static constexpr f32 DEFAULT_LIFETIME = 30.0f;  // 约 1.5 秒

    f32 m_initialSize;  ///< 初始大小（用于缩放动画）
};

} // namespace mc::client::renderer::trident::particle::particles
