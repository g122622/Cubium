#pragma once

#include "common/entity/core/EntityFlags.hpp"

namespace mc::ecs {

/**
 * @brief 实体标志位组件
 *
 * 承载 Entity::m_flags（EntityFlags 位掩码）。对齐基岩版 EntityFlagsComponent
 * （mc/entity/components/EntityFlagsComponent.h）。
 *
 * 所有 Entity（含 LivingEntity/MobEntity/Player 子类）attach，Entity 层基础组件。
 *
 * 字段语义：
 * - m_flags：同步真相源（DATA_FLAGS_PARAM id=0 退为镜像）。位定义见 EntityFlags，
 *   客户端经 slot0 读取 Swimming/Glowing 等位。setFlags/addFlag/removeFlag 同时写
 *   组件 + DataParameter，消除双写。
 */
struct EntityFlagsComponent {
    EntityFlags m_flags{EntityFlags::None};
};

} // namespace mc::ecs
