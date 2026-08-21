#include "common/entity/ecs/systems/ticking/FireTick.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"

namespace mc::ecs::sys {

void fireTick(entt::basic_registry<EntityId>& /*registry*/,
    mc::ecs::EntityView<entt::get_t<FireComponent, EntityOwnerComponent>> view)
{
    // 遍历所有挂 FireComponent 的实体。EntityOwnerComponent 持 OOP Entity&，
    // 供调用虚函数 isImmuneToFire/isInWater/isInLava/isInRain/hurt 等。
    // m_fire 递减直接读写 FireComponent。
    for (auto [entityId, fire, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 1. fire 链（原 baseTick 行 751-772，逐字搬迁）。
        //    正值 m_fire = 燃烧剩余 tick，负值 = 火焰免���期倒计时。
        if (fire.m_fire > 0) {
            if (entity->isImmuneToFire()) {
                // 免疫火焰的实体立即清除火焰。
                entity->clearFire();
            } else if (entity->isInWater()) {
                // 水中直接熄灭，播放音效并设置免疫期。
                entity->extinguishFire();
                entity->setFireImmunityCooldown();
            } else {
                // 燃烧伤害：每 20 tick（1 秒）造成 1 点 onFire 伤害。
                // 注意：在岩浆中时不造成燃烧伤害，因为岩浆伤害由 lavaHurt() 单独处理。
                if (fire.m_fire % 20 == 0 && !entity->isInLava()) {
                    auto onFireSource = DamageSources::onFire();
                    entity->hurt(onFireSource, 1.0f);
                }
                fire.m_fire--;
            }
        }
        // TODO(fire-immunity-decay): FireComponent 负值（火焰免疫期倒计时）按设计应每 tick +1
        // 趋向 0（递减绝对值），但本逐字搬迁自原 baseTick 的逻辑仅处理 m_fire>0 分支，
        // 负值免疫期在此系统内不被递减。这是从原 OOP 逻辑继承的预存缺陷，保留原行为不扩大
        // 范围，待后续单独修复（修复点：补 else if (fire.m_fire < 0) fire.m_fire++; 分支）。

        // 2. 雨中扑灭（原 baseTick 行 774-779，逐字搬迁）。
        //    在雨中熄灭火焰时，播放音效并设置免疫期。
        if (entity->isInRain() && entity->isOnFire()) {
            entity->extinguishFire();
            entity->setFireImmunityCooldown();
        }
    }
}

} // namespace mc::ecs::sys
