#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 云朵粒子
 *
 * 参考 MC 1.16.5 CloudParticle
 *
 * 特性：
 * - 无物理碰撞
 * - 随年龄扩大
 * - 对附近玩家反应
 * - 淡出消失
 */
class CloudParticle : public Particle {
public:
    CloudParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/cloud");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    f64 m_initialSize;
};

/**
 * @brief 屏障粒子
 *
 * 参考 MC 1.16.5 BarrierParticle
 *
 * 特性：
 * - 显示屏障方块
 * - 使用方块纹理
 * - 固定大小
 * - 不移动
 */
class BarrierParticle : public Particle {
public:
    BarrierParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::TERRAIN_SHEET;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/barrier");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 80.0;
};

/**
 * @brief 水花粒子
 *
 * 参考 MC 1.16.5 WaterWakeParticle
 *
 * 特性：
 * - 水面涟漪效果
 * - 随年龄扩大
 * - 在水面形成
 */
class WaterWakeParticle : public Particle {
public:
    WaterWakeParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/water_wake");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 海豚粒子
 *
 * 参考 MC 1.16.5 DolphinParticle
 *
 * 特性：
 * - 海豚游泳时产生
 * - 快速气泡效果
 * - 向上漂浮
 */
class DolphinParticle : public Particle {
public:
    DolphinParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_OPAQUE;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/dolphin");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 10.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
