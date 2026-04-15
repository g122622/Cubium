#pragma once

#include "ProjectileEntity.hpp"

namespace mc {
namespace entity {

/**
 * @brief 带加速度的投掷物基类
 *
 * 当前实现对齐 1.16.5 `DamagingProjectileEntity` 的运动语义，
 * 同时把火球类基础伤害合并到这一层，便于现有 C++ 分层复用。
 */
class DamagingProjectileEntity : public ProjectileEntity {
public:
    ~DamagingProjectileEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 accelerationX() const { return m_accelerationX; }
    [[nodiscard]] f32 accelerationY() const { return m_accelerationY; }
    [[nodiscard]] f32 accelerationZ() const { return m_accelerationZ; }

    void setAcceleration(f32 x, f32 y, f32 z) {
        m_accelerationX = x;
        m_accelerationY = y;
        m_accelerationZ = z;
    }

    [[nodiscard]] f32 damage() const { return m_damage; }
    void setDamage(f32 damage) { m_damage = damage; }

    [[nodiscard]] bool canBeCollidedWith() const override { return true; }
    [[nodiscard]] f32 getCollisionBorderSize() const override { return 1.0f; }

protected:
    DamagingProjectileEntity(LegacyEntityType type, EntityId id);

    [[nodiscard]] virtual bool isFiery() const { return true; }
    [[nodiscard]] virtual f32 getMotionFactor() const { return 0.95f; }
    virtual void spawnTrailParticles(const Vector3& position);

    f32 m_accelerationX = 0.0f;
    f32 m_accelerationY = 0.0f;
    f32 m_accelerationZ = 0.0f;
    f32 m_damage = 0.0f;
};

} // namespace entity
} // namespace mc
