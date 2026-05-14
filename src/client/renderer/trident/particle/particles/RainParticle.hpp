#pragma once

#include "../Particle.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 雨滴粒子
 *
 * 参考 MC 1.16.5 RainParticle
 *
 * 特性：
 * - 快速下落
 * - 碰撞地面时消失或产生溅射效果
 * - 雨滴大小和速度随机变化
 *
 * 用法：
 * @code
 * auto rain = std::make_unique<RainParticle>(
 *     glm::vec3(x, y, z),
 *     glm::vec3(0.0f, -3.0f, 0.0f)
 * );
 * particleManager.addParticle(std::move(rain));
 * @endcode
 */
class RainParticle : public Particle {
public:
    /**
     * @brief 构造雨滴粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    RainParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 工厂方法：创建雨滴粒子
     *
     * @param pos 位置
     * @param velocity 速度
     * @param world 客户端世界（可选）
     * @return 雨滴粒子实例
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override
    {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override
    {
        return ResourceLocation("minecraft:particle/rain");
    }

private:
    static constexpr f64 DEFAULT_SIZE = 0.01f;      ///< 雨滴大小
    static constexpr f64 TERMINAL_VELOCITY = -3.0f; ///< 终端速度
};

} // namespace mc::client::renderer::trident::particle::particles
