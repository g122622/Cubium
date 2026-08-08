#include "common/entity/ecs/systems/FireTickSystem.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/ecs/components/EntityOwnerComponent.hpp"
#include "common/entity/ecs/components/FireComponent.hpp"

namespace mc::ecs {

void FireTickSystem::tick(EntityRegistry& registry)
{
    // 遍历所有挂 FireComponent 的实体。EntityOwnerComponent 持 OOP Entity&，
    // 供调用虚函数 isImmuneToFire/isInWater/isInLava/isInRain/hurt 等。
    // m_fire 递减直接读写 FireComponent。
    auto view = registry.raw().view<FireComponent, EntityOwnerComponent>();
    for (auto [entityId, fire, owner] : view.each()) {
        Entity* entity = owner.tryGetEntity();
        if (entity == nullptr || entity->isRemoved()) {
            continue;
        }

        // 1. fire 链（原 baseTick 行 751-772，逐字搬迁）。
        //    正值 m_fire = 燃烧剩余 tick，负值 = 火焰免疫期倒计时。
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

        // 2. 雨中扑灭（原 baseTick 行 774-779，逐字搬迁）。
        //    在雨中熄灭火焰时，播放音效并设置免疫期。
        if (entity->isInRain() && entity->isOnFire()) {
            entity->extinguishFire();
            entity->setFireImmunityCooldown();
        }
    }
}

} // namespace mc::ecs
