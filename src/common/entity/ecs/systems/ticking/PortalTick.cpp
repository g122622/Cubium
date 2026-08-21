#include "common/entity/ecs/systems/ticking/PortalTick.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/PortalComponent.hpp"

#include <algorithm>

namespace mc::ecs::sys {

void portalTick(entt::basic_registry<EntityId>& /*registry*/,
    mc::ecs::EntityView<entt::get_t<PortalComponent, EntityOwnerComponent>> view)
{
    // 遍历所有挂 PortalComponent 的实体。EntityOwnerComponent 持 OOP Entity&，
    // 供调用虚函数 getMaxInPortalTime/canTeleport/onPortalTriggered（这些保留多态）。
    // portal 冷却递减与 portalTime 计时直接读写组件（纯数据操作，无需多态）。
    for (auto [entityId, portal, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 1. 传送冷却递减（原 baseTick 行 736-739）。
        if (portal.m_portalCooldown > 0) {
            portal.m_portalCooldown--;
        }

        // 2. tickPortal 逻辑（原 Entity::tickPortal 行 808-839，逐字搬迁）。
        //    inPortal 每帧重置，由 NetherPortalBlock::onEntityCollision 重新设置。
        if (!portal.m_inPortal) {
            if (portal.m_portalTime > 0) {
                portal.m_portalTime = std::max(0, portal.m_portalTime - 4);
            }
            continue;
        }

        // 无论是否传送，都重置 inPortal。
        portal.m_inPortal = false;

        // canTeleport 经 OOP 句柄调用（保留多态可能性，当前基类读 portalCooldown<=0）。
        if (!entity->canTeleport()) {
            continue;
        }

        // 递增计时并检查阈值。getMaxInPortalTime 是虚函数，经 OOP 句柄派发到 Player 等。
        portal.m_portalTime++;

        const i32 maxPortalTime = entity->getMaxInPortalTime();
        if (portal.m_portalTime >= maxPortalTime) {
            portal.m_portalTime = maxPortalTime;
            // 达阈值触发传送：onPortalTriggered 内部重置 inPortal/portalTime +
            // triggerPortalCooldown（设 portalCooldown = getPortalCooldown()）。
            // 注意：onPortalTriggered 写的是 OOP 成员路径，组件化后须改写组件（见字段迁移）。
            entity->onPortalTriggered();
        }
    }
}

} // namespace mc::ecs::sys
