#pragma once

#include "../../Particle.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::client {
class ClientWorld;
}

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 药水效果粒子基类
 *
 * 参考 MC 1.16.5 SpellParticle
 *
 * 特性：
 * - 向上漂浮
 * - 无物理
 * - 半透明
 */
class SpellParticle : public Particle {
public:
    /**
     * @brief 构造药水粒子
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param color 粒子颜色（ARGB）
     */
    SpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/spell");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

protected:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 瞬间药水效果粒子
 *
 * 参考 MC 1.16.5 InstantSpellParticle
 * 与 SpellParticle 类似但颜色更亮。
 */
class InstantSpellParticle : public Particle {
public:
    InstantSpellParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/instant_spell");
    }

    [[nodiscard]] u32 getLightColor(mc::client::ClientWorld* world) const override {
        MC_UNUSED(world);
        return 0xF0;
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 实体效果粒子
 *
 * 参考 MC 1.16.5 EntityEffectParticle
 * 用于实体上的药水效果。
 */
class EntityEffectParticle : public Particle {
public:
    EntityEffectParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/entity_effect");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

/**
 * @brief 环境实体效果粒子
 *
 * 参考 MC 1.16.5 AmbientEntityEffectParticle
 * 信标效果粒子，更透明、更慢。
 */
class AmbientEntityEffectParticle : public Particle {
public:
    AmbientEntityEffectParticle(const glm::vec3& pos, const glm::vec3& velocity, const glm::vec4& color);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/ambient_entity_effect");
    }

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

private:
    static constexpr f64 DEFAULT_LIFETIME = 12.0;
    f64 m_initialSize;
};

/**
 * @brief 巫师粒子
 *
 * 参考 MC 1.16.5 WitchParticle
 * 紫色的药水效果粒子。
 */
class WitchParticle : public Particle {
public:
    WitchParticle(const glm::vec3& pos, const glm::vec3& velocity);

    static std::unique_ptr<Particle> create(
        const glm::vec3& pos,
        const glm::vec3& velocity,
        mc::client::ClientWorld* world);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override {
        return ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
    }

    [[nodiscard]] ResourceLocation getTextureLocation() const override {
        return ResourceLocation("minecraft:particle/witch");
    }

private:
    static constexpr f64 DEFAULT_LIFETIME = 8.0;
    f64 m_initialSize;
};

} // namespace mc::client::renderer::trident::particle::particles
