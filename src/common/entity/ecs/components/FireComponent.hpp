#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 火焰状态组件
 *
 * 承载 Entity::m_fire。
 *
 * 字段语义（与原 Entity::m_fire 完全一致，符号编码状态）：
 * - 正值：燃烧剩余 tick。每 tick -1，>0 期间 isOnFire() 返回 true，每 20 tick 造成 1 点火焰伤害。
 * - 0：既不燃烧也不免疫。
 * - 负值：火焰免疫期倒计时。setFireImmunityCooldown 设为负值，期间 setFire/setRemainingFireTicks
 *   仅在新值大于当前负值时才覆盖（见 Entity.hpp:1484/1502 的 if (m_fire < ticks) 守卫），
 *   防止刚离开火源又被瞬间点燃。每 tick 负值 +1 趋向 0（递减绝对值）。
 *
 * 由 ecs::sys::fireTick（PostEntityTick 阶段）执行递减与伤害判定，原 baseTick 的 fire 链已迁出。
 */
struct FireComponent {
    i32 m_fire{0}; // 剩余着火时间（tick），正值=燃烧，负值=火焰免疫期倒计时
};

} // namespace mc::ecs
