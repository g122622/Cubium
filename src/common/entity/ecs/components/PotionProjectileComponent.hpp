#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 药水投掷物组件
 *
 * 承载 PotionEntity（ProjectileItemEntity 子类）的 lingering 标志：该药水是喷溅型
 * 还是滞留型。对齐 vanilla ThrownPotion 的物品类型判定（滞留药水命中生成滞留云）。
 *
 * 仅 PotionEntity attach（雪球/蛋/末影珍珠/经验瓶不挂本组件）。
 *
 * 字段语义：
 * - m_lingering：是否为滞留药水（true=滞留型命中生成 AreaEffectCloud，false=喷溅型即时生效）。
 */
struct PotionProjectileComponent {
    bool m_lingering{false};
};

} // namespace mc::ecs
