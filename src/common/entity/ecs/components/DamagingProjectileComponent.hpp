#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 伤害型投射物组件
 *
 * 承载 DamagingProjectileEntity 的 4 字段：加速度三分量 + 伤害值。对齐 vanilla
 * AbstractHurtingProjectile（acceleration/power/damage）。
 *
 * 仅 DamagingProjectileEntity 子树 attach（Fireball/SmallFireball/DragonFireball/
 * WitherSkull 经 AbstractFireballEntity 继承）。加速度每 tick 由 tick() 重新归一化
 * 施加到速度，vanilla 持久化 acceleration_power（项目当前未持久化此族，本批 Step6 补）。
 *
 * 字段语义：
 * - m_accelerationX/Y/Z：本 tick 加速度方向（归一化前），每 tick 加到速度上。
 * - m_damage：碰撞伤害（火球默认 1.0，凋灵之首/龙息各异）。
 */
struct DamagingProjectileComponent {
    f32 m_accelerationX{0.0f};
    f32 m_accelerationY{0.0f};
    f32 m_accelerationZ{0.0f};
    f32 m_damage{0.0f};
};

} // namespace mc::ecs
