#pragma once

#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/PortalComponent.hpp"
#include "common/entity/ecs/systems/base/EntityView.hpp"

namespace mc::ecs::sys {

/**
 * @brief 传送门 tick 系统（free function）
 *
 * 承载原 Entity::tick() 中 baseTick 之后的 tickPortal() 逻辑，以及原 baseTick 内的
 * m_portalCooldown 递减。注册到 SystemPhase::PostEntityTick 阶段——在 EntityTick
 * （逐实体 OOP Entity::tick()）之后执行，可读到本帧刚由环境感知产出的状态。
 *
 * 抽取自 OOP 的两段逻辑（逐字搬迁，行为等价）：
 *   1. baseTick 的 m_portalCooldown 递减（if >0 then --）；
 *   2. Entity::tickPortal()：读 inPortal → false 则 portalTime 衰减 4 → true 则重置
 *      inPortal → canTeleport 检查 → portalTime++ → 达 getMaxInPortalTime() 阈值则
 *      调 onPortalTriggered()（含 triggerPortalCooldown）。
 *
 * 多态保留：getMaxInPortalTime()/canTeleport()/onPortalTriggered() 经 EntityOwnerComponent
 * 反查 OOP Entity& 调虚函数，正确派发到 Player 等子类（Player::getMaxInPortalTime 返回
 * 1/80，基类返回 0）。原 Player::tickPortal() override 与基类逐字相同（仅注释不同），
 * 逻辑搬入本系统后该 override 删除，行为等价。
 *
 * 跨帧延迟（用户已接受）：portal 计时递进结果下帧 baseTick 才读到，单帧 50ms 玩家无感。
 *
 * ## 签名与依赖推导
 * 参数为 entt 原生 basic_view（经 mc::ecs::EntityView 别名绑定 EntityId——entt::view 默认绑
 * entt::entity 与本项目 basic_registry<EntityId> 不匹配，见 base/EntityView.hpp）。
 * organizer 从 PortalComponent&（非 const，rw）推导写依赖，从 EntityOwnerComponent&
 * （非 const，rw）推导反查句柄依赖。view 显式列出真正读写的组件以正确建图。
 */
void portalTick(entt::basic_registry<EntityId>& registry,
    mc::ecs::EntityView<entt::get_t<PortalComponent, EntityOwnerComponent>> view);

} // namespace mc::ecs::sys
