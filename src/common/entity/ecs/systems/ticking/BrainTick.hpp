#pragma once

#include "common/entity/ecs/systems/scheduler/OrganizerGraph.hpp"

namespace mc::ecs::sys {

/**
 * @brief Brain tick 桥接系统（free function · sync_point）
 *
 * 将 VillagerEntity::tick() 中 m_brain->tick() 调用块抽成独立 system，注册到
 * SystemPhase::PostEntityTick 阶段。通过 payload（指向 EntityManager）委托
 * EntityManager::_tickBrains()，复用 _tickEntities 的遍历+门控框架（playerChunks 快照/
 * isRemoved 跳过/ServerPlayer 短路/模拟距离门控），避免 EntityManager ↔ Collection 循环依赖。
 *
 * 与 legacyTick 同属回调委托型范式（区别在于：
 * - legacyTick 桥接所有实体的 OOP Entity::tick() 虚函数链；
 * - brainTick 仅桥接持有 Brain 的实体（当前仅 VillagerEntity）的 m_brain->tick() 调用）。
 *
 * 时序：注册在 PortalTick/FireTick 之后，故 Brain tick 在所有实体 OOP tick
 * （含 goalSelector.tick/navigator.tick）+ portal/fire 递减之后执行。跨实体传感器
 * （NearestPlayersSensor 等）读到本帧最终状态，行为更正确。
 *
 * 设计依据：决策"AI 系统保留 OOP（Goal/Brain/Navigator/Controller 不 ECS 化），仅用 System
 * 做 tick 调度"。Brain 仍是 OOP 成员（VillagerEntity::m_brain），本 system 只搬"何时调
 * tick()"调度决策，不 ECS 化 Brain 数据。
 *
 * ## sync_point 语义
 * 同 legacyTick：签名持 Registry& 参数触发 sync_point=true，永远串行、无依赖推导。
 *
 * ## 友元
 * 本函数需访问 EntityManager 私有 _tickBrains()，故 EntityManager 友元本函数。
 */
void brainTick(const void* payload, OrganizerGraph::Registry& registry);

} // namespace mc::ecs::sys
