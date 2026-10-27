#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 伤害型投射物组件
 *
 * 承载 DamagingProjectileEntity 的状态：飞行方向、加速力与伤害值。对齐 vanilla
 * AbstractHurtingProjectile（direction / accelerationPower / damage）。
 *
 * 仅 DamagingProjectileEntity 子树 attach（Fireball/SmallFireball/DragonFireball/
 * WitherSkull 经 AbstractFireballEntity 继承）。
 *
 * 字段语义（对齐 vanilla AbstractHurtingProjectile）：
 * - m_directionX/Y/Z：发射时指定的飞行方向（vanilla 构造参数，不单独持久化；取消插值后
 *   被合并进速度向量，读档时与速度同向，因此无需额外存盘）。
 * - m_accelerationPower：加速力大小。每 tick 速度按
 *   `(v + normalize(v) * power) * inertia` 演化，inertia 空中 0.95、水中 0.8。
 *   持久化键 "acceleration_power"（vanilla addAdditionalSaveData）。
 * - m_damage：碰撞伤害（火球默认 1.0，凋灵之首/龙息各异）。
 */
struct DamagingProjectileComponent {
    f32 m_directionX{0.0f};
    f32 m_directionY{0.0f};
    f32 m_directionZ{0.0f};
    f32 m_accelerationPower{0.1f}; // vanilla AbstractHurtingProjectile.accelerationPower 默认 0.1
    f32 m_damage{0.0f};
};

} // namespace mc::ecs
