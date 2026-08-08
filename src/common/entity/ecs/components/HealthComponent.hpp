#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 生命值状态组件
 *
 * 承载 LivingEntity::m_health / m_lastHealth / m_healthSynced 三字段。
 * 对齐基岩版 HealthComponent（mc/entity/components/HealthComponent.h）。
 *
 * 仅 LivingEntity（含其子类 MobEntity/Player）attach，普通 Entity 不持有此组件。
 *
 * 字段语义：
 * - m_health：当前生命值，同步真相源（DATA_HEALTH_PARAM 退为镜像）。
 * - m_lastHealth：上一tick生命值，战斗追踪器记录伤害用，纯本地。
 * - m_healthSynced：首帧生命值同步兜底标志。构造期 registerAttributes 因虚函数
 *   时序拿不到派生类 MAX_HEALTH，m_health 停在默认 20.0 违反 health<=maxHealth
 *   不变式；tick 首帧检测未同步则 setHealth(maxHealth()) 兜底。NBT 加载的 health
 *   是权威值，置 true 避免首帧覆盖。
 */
struct HealthComponent {
    f32 m_health{20.0f};
    f32 m_lastHealth{20.0f};
    bool m_healthSynced{false};
};

} // namespace mc::ecs
