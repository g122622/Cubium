#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 受击/死亡状态组件
 *
 * 承载 LivingEntity::m_absorption / m_hurtTime / m_maxHurtTime / m_deathTime 四字段。
 * 对齐基岩版 MobHurtTimeComponent（mc/entity/components/MobHurtTimeComponent.h）与
 * DeathTickingComponent 族。
 *
 * 仅 LivingEntity（含其子类 MobEntity/Player）attach，普通 Entity 不持有此组件。
 * 四字段全为纯本地状态（无 DataParameter 同步、仅 NBT 持久化），故可安全组件化。
 *
 * 字段语义：
 * - m_absorption：吸收值（金苹果等提供），actuallyHurt 中先扣吸收再扣生命。
 * - m_hurtTime：受击红屏摇晃剩余 tick，每 tick -1，受击时置为 m_maxHurtTime。
 * - m_maxHurtTime：最大受击摇晃时长（受击时恒为 10），hurtDuration 复用此值。
 * - m_deathTime：死亡消散计时。tickDeath 每 tick +1，达 20 后 remove()。
 */
struct HurtStateComponent {
    f32 m_absorption{0.0f}; // 吸收值（金苹果）
    i32 m_hurtTime{0};      // 受伤无敌时间
    i32 m_maxHurtTime{10};  // 最大受伤无敌时间
    i32 m_deathTime{0};     // 死亡时间
};

} // namespace mc::ecs
