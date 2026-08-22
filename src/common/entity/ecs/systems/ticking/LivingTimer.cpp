#include "common/entity/ecs/systems/ticking/LivingTimer.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/CombatTracker.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/LivingTimerComponent.hpp"

namespace mc::ecs::sys {

void livingTimerTick(entt::basic_registry<EntityId>& /*registry*/,
    mc::ecs::EntityView<entt::get_t<LivingTimerComponent, EntityOwnerComponent>> view)
{
    // 遍历所有挂 LivingTimerComponent 的实体（仅 LivingEntity attach）。
    // EntityOwnerComponent 持 OOP Entity&，供调用 ticksExisted()/isElytraFlying()/
    // sendEndCombat() 等（sendEndCombat 需 LivingEntity 句柄，经 dynamic_cast 下行）。
    for (auto [entityId, timer, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 1. 受伤无敌帧递减（原 LivingEntity::tick 行 914-916 逐字搬迁）。
        if (timer.hurtResistantTime > 0) {
            --timer.hurtResistantTime;
        }

        // 2. 战斗超时检查（原 LivingEntity::tick 行 963-966 逐字搬迁）。
        //    超时则退出战斗状态并调 sendEndCombat() 虚回调（经 LivingEntity 句柄派发，
        //    当前无子类 override，空操作；ServerPlayer 未来可 override 发包）。
        if (timer.inCombat && entity->ticksExisted() - timer.lastDamageTimestamp > CombatTracker::COMBAT_TIMEOUT) {
            timer.inCombat = false;
            auto* living = dynamic_cast<LivingEntity*>(entity);
            if (living != nullptr) {
                living->sendEndCombat();
            }
        }

        // 3. 鞘翅飞行计时器递增/归零（原 LivingEntity::tick 行 976-980 逐字搬迁）。
        //    isElytraFlying() 读 EntityFlags::FallFlying 标志位（Entity 基类公共方法）。
        //    时序约束见头文件注释：递增须在 EntityTick 之后（本 PostEntityTick 阶段），
        //    使 updateFallFlying（EntityTick 内 aiStep）读到上一帧递增后的值 + 1，
        //    复刻原 OOP 末尾递增语义。
        if (entity->isElytraFlying()) {
            ++timer.fallFlyTicks;
        } else {
            timer.fallFlyTicks = 0;
        }
    }
}

} // namespace mc::ecs::sys
