#pragma once

#include "common/entity/ecs/systems/scheduler/OrganizerGraph.hpp"

namespace mc::ecs::sys {

/**
 * @brief OOP Entity::tick() 桥接系统（free function · sync_point）
 *
 * 将现有 OOP Entity::tick() 虚函数链包装为一个 ECS system，注册到 SystemPhase::EntityTick
 * 阶段。本函数不自行遍历 registry 重新实现 tick 逻辑，而是通过 payload（指向 EntityManager）
 * 委托 EntityManager 已有的逐实体 tick（含模拟距离门控、ServerPlayer 永远 tick 等成熟逻辑）。
 *
 * 这样设计的目的：
 *   - 保持 EntityManager::tick 三步编排语义不变（步骤1委托本system→步骤2清graveyard→步骤3收死亡）
 *   - 避免 EntityManager ↔ Collection 循环依赖
 *   - 后续批次把 OOP tick 逐步拆成真正遍历组件的 ECS system 时，替换委托实现即可
 *
 * 强时序内聚逻辑（hurt 链/AI 决策）暂留 OOP tick 壳内，不拆成多个 system，保留调用栈
 * 可读性（混合架构不引入双重时序轴的关键，见 ecs/README 坑位）。
 *
 * ## sync_point 语义
 * 签名持 Registry& 参数（sync_point 回调），organizer 强制本顶点 sync_point=true 永远串行、
 * 无依赖推导——桥接壳委托宿主遍历方法（含模拟距离门控/ServerPlayer 短路），不是纯 view
 * 遍历，无法被 organizer 推导，强制串行是正确语义。
 *
 * ## 友元
 * 本函数需访问 EntityManager 私有 _tickEntities()，故 EntityManager 友元本函数。
 * 声明在 EntityManager.hpp，本头不 include EntityManager.hpp 以避免循环依赖。
 */
void legacyTick(const void* payload, OrganizerGraph::Registry& registry);

} // namespace mc::ecs::sys
