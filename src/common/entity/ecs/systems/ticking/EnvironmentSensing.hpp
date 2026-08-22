#pragma once

#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/EnvironmentStateComponent.hpp"
#include "common/entity/ecs/systems/base/EntityView.hpp"

namespace mc::ecs::sys {

/**
 * @brief 环境感知 tick 系统（free function）
 *
 * 承载原 Entity::updateEnvironmentState()（Entity.cpp 旧函数体）逻辑，逐字搬迁：遍历实体
 * 碰撞箱内方块流体，计算水/岩浆浸入高度与眼睛浸入判定，重写 EnvironmentStateComponent 七字段。
 * 注册到 SystemPhase::EnvironmentSense 阶段——在 EntityTick（逐实体 OOP Entity::tick()）之前
 * 执行，同帧产出同帧消费：baseTick:815 的岩浆坠距减半、PostEntityTick 的 fireTick 经
 * isInWater()/isInLava() 读本组件，均无跨帧延迟（比 portal/fire 的 1 tick 延迟更优）。
 *
 * 每帧无条件重写组件（与原 baseTick 每帧调 updateEnvironmentState 重置语义一致）：system 先把
 * 七字段重置为默认值，再遍历流体方块累加。故外部对本组件的写入（若有）每帧被覆写。
 *
 * 多态保留：eyeHeight()/boundingBox() 经 EntityOwnerComponent 反查 OOP Entity& 调虚函数，正确
 * 派发到各子类（eyeHeight 有几十处 override，boundingBox 读 m_builtIn.aabbShape）。world()/
 * physicsEngine() 取世界/物理引擎句柄，getFluidState 查世界流体。原函数体经 m_world/m_physicsEngine
 * 的 protected 成员访问，迁入本 system 后改走 public getter world()/physicsEngine()。
 *
 * 迁移时丢弃原函数体死代码 eyeBlockY（计算后未使用）。
 *
 * ## 签名与依赖推导
 * 参数为 entt 原生 basic_view（经 mc::ecs::EntityView 别名绑定 EntityId——entt::view 默认绑
 * entt::entity 与本项目 basic_registry<EntityId> 不匹配，见 base/EntityView.hpp）。organizer
 * 从 EnvironmentStateComponent&（非 const，rw）推导写依赖，从 EntityOwnerComponent&（非 const，
 * rw）推导反查句柄依赖。view 显式列出真正读写的组件以正确建图。
 */
void environmentSensing(entt::basic_registry<EntityId>& registry,
    mc::ecs::EntityView<entt::get_t<EnvironmentStateComponent, EntityOwnerComponent>> view);

} // namespace mc::ecs::sys
