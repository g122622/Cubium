#pragma once

#include "../../Particle.hpp"
#include "../../../../../../common/world/block/Block.hpp"
#include "../../../../../../common/util/assert/AssertAll.hpp"

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 挖掘粒子（破坏方块时产生）
 *
 * 参考 MC 1.16.5 DiggingParticle / TerrainParticle
 *
 * 特性：
 * - 使用方块纹理
 * - 从方块位置向四周散射
 * - 受重力影响
 */
class DiggingParticle : public Particle {
public:
    DiggingParticle(const glm::vec3& pos, const glm::vec3& velocity, const BlockState& blockState);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        ClientWorld* world);

    static std::unique_ptr<Particle> createWithBlock(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        const BlockState& blockState);

    void tick(ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::TERRAIN_SHEET;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override;

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.03f;
    static constexpr f64 DEFAULT_SIZE = 0.1f;
    static constexpr f64 DEFAULT_LIFETIME = 20.0f;

    BlockState m_blockState;
};

} // namespace mc::client::renderer::trident::particle::particles
