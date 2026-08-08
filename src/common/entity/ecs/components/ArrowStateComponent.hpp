#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 箭矢/蜂针状态组件
 *
 * 承载 LivingEntity::m_arrowCount / m_stingerCount / m_arrowHitTimer 三字段。
 * 对齐基岩版 ArrowCountComponent / StingerCountComponent 族。
 *
 * 仅 LivingEntity（含其子类 MobEntity/Player）attach，普通 Entity 不持有此组件。
 *
 * 字段语义：
 * - m_arrowCount：插在身上的箭矢数量，同步真相源（DATA_ARROW_COUNT_PARAM 退为镜像）。
 *   渲染层 ArrowLayer 据此渲染插在实体身上的箭矢。
 * - m_stingerCount：插在身上的蜂针数量，同步真相源（DATA_STINGER_COUNT_PARAM 退为镜像）。
 * - m_arrowHitTimer：箭矢自动脱落计时器，纯服务端无同步。箭矢越多脱落越快
 *   （公式 20*(30-arrowCount) ticks），归零时减一支箭。
 *
 * 箭矢/蜂针不持久化（无 NBT），仅运行时同步。
 */
struct ArrowStateComponent {
    i32 m_arrowCount{0};
    i32 m_stingerCount{0};
    i32 m_arrowHitTimer{0};
};

} // namespace mc::ecs
