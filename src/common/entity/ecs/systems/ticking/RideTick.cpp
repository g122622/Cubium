#include "common/entity/ecs/systems/ticking/RideTick.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"

namespace mc::ecs::sys {

void rideTick(entt::basic_registry<EntityId>& /*registry*/, mc::ecs::EntityView<entt::get_t<EntityOwnerComponent>> view)
{
    // 遍历所有实体（EntityOwnerComponent 所有实体 attach）。仅对有乘客的载具（hasPassengers）
    // 调 updatePassengers 同步乘客位置，避免无乘客实体无谓遍历 m_passengers。
    // updatePassengers 内部经虚 updatePassengerPosition 派发到各载具子类 override（船/马）或
    // 基类 positionRider（猪/炽足兽/矿车），详见 RideTick.hpp 注释。
    for (auto [entityId, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 仅载具（有乘客）才同步。hasPassengers() 读 m_passengers.empty()。
        if (!entity->hasPassengers()) {
            continue;
        }

        // 同步所有乘客位置到载具当前位置（载具本帧 OOP tick 已移动，PostEntityTick 在其后）。
        entity->updatePassengers();
    }
}

} // namespace mc::ecs::sys
