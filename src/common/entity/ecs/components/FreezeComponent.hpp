#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 冰冻状态组件
 *
 * 承载 Entity::m_ticksFrozen / m_isInPowderSnow。对齐基岩版 FreezingComponent
 * （mc/entity/components/FreezingComponent.h）。
 *
 * m_ticksFrozen 是同步字段（DATA_TICKS_FROZEN_PARAM id=7），本批组件化时以本组件为
 * 真相源，DataParameter 退为同步镜像：setTicksFrozen 同时写组件 + DataParameter，
 * 消除首批的双写（原 setTicksFrozen 既写 m_ticksFrozen 成员又写 DataParameter）。
 * 反序列化统一走 setter，不再从同步层回填成员字段。
 *
 * 字段语义：
 * - m_ticksFrozen：冰冻计时器。正值=冰冻进度，达 getTicksRequiredToFreeze() 时完全冰冻
 *   （每 40 tick 1 点伤害）。细雪中每 tick +1（PowderSnowBlock::onEntityCollision），
 *   离开后 tickFreeze 每 tick -2。
 * - m_isInPowderSnow：本 tick 是否处于细雪中。每帧 baseTick 重置为 false，由
 *   PowderSnowBlock::onEntityCollision 设置为 true，供 tickFreeze 判定递增还是递减。
 */
struct FreezeComponent {
    i32 m_ticksFrozen{0};         ///< 冰冻计时器（正值=冰冻进度，达到阈值时完全冰冻）
    bool m_isInPowderSnow{false}; ///< 当前 tick 是否处于细雪中（每帧重置）
};

} // namespace mc::ecs
