/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "ProjectileEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"

// 前向声明粒子类型（与 common/particle/ParticleTypes.hpp 中的定义一致）
namespace mc {
namespace particle {
enum class ParticleTypeId : u16;
}
} // namespace mc

namespace mc {
namespace entity {

/**
 * @brief 带加速度的投掷物基类
 *
 * 火球类实体的基类，包含加速度和伤害属性。
 *
 * 子类粒子效果：
 * - FireballEntity: 使用默认 SMOKE 粒子
 * - DragonFireballEntity: 重写 getParticleType() 返回 DRAGON_BREATH
 * - WitherSkullEntity: 使用默认 SMOKE 粒子
 */
class DamagingProjectileEntity : public ProjectileEntity {
public:
    ~DamagingProjectileEntity() override = default;

    void tick() override;

    [[nodiscard]] f32 accelerationX() const { return m_accelerationX; }
    [[nodiscard]] f32 accelerationY() const { return m_accelerationY; }
    [[nodiscard]] f32 accelerationZ() const { return m_accelerationZ; }

    void setAcceleration(f32 x, f32 y, f32 z)
    {
        m_accelerationX = x;
        m_accelerationY = y;
        m_accelerationZ = z;
    }

    [[nodiscard]] f32 damage() const { return m_damage; }
    void setDamage(f32 damage) { m_damage = damage; }

    [[nodiscard]] bool canBeCollidedWith() const override { return true; }
    [[nodiscard]] f32 getCollisionBorderSize() const override { return 1.0f; }

protected:
    explicit DamagingProjectileEntity(EntityInstanceId id);

    [[nodiscard]] virtual bool isFiery() const { return true; }
    [[nodiscard]] virtual f32 getMotionFactor() const { return 0.95f; }

    /**
     * @brief 获取拖尾粒子类型
     *
     * 子类可重写此方法返回不同的粒子类型：
     * - 默认返回 SMOKE（火球、凋灵之首）
     * - DragonFireballEntity 返回 DRAGON_BREATH
     *
     * @return 粒子类型ID
     */
    [[nodiscard]] virtual particle::ParticleTypeId getParticleType() const;

    /**
     * @brief 生成拖尾粒子
     *
     * 在实体飞行时生成拖尾粒子效果。
     *
     * @param position 粒子生成位置
     */
    virtual void spawnTrailParticles(const Vector3& position);

    /**
     * @brief 生成水下气泡粒子
     *
     * 当实体在水中时生成气泡粒子尾迹。
     */
    void spawnWaterParticles();

    f32 m_accelerationX = 0.0f;
    f32 m_accelerationY = 0.0f;
    f32 m_accelerationZ = 0.0f;
    f32 m_damage = 0.0f;
};

} // namespace entity
} // namespace mc
