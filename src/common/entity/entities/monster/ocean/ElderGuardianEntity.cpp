#include "ElderGuardianEntity.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "../../../entities/player/Player.hpp"

namespace mc {

ElderGuardianEntity::ElderGuardianEntity(LegacyEntityType type, EntityId id)
    : GuardianEntity(type, id)
{
    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ElderGuardianEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ElderGuardianEntity>(LegacyEntityType::Unknown, 0);
}

void ElderGuardianEntity::tick()
{
    GuardianEntity::tick();

    // 更新挖掘疲劳
    m_fatigueTimer++;
    if (m_fatigueTimer >= FATIGUE_INTERVAL) {
        m_fatigueTimer = 0;
        // MC 1.16.5: 给附近的玩家挖掘疲劳效果
        // 参考 ElderGuardianEntity.addEffectToTargetWithinRange()
        if (m_world != nullptr) {
            std::vector<Entity*> nearbyEntities = m_world->getEntitiesInRange(m_position, MINING_FATIGUE_RANGE);
            for (Entity* entity : nearbyEntities) {
                // 只对玩家生效
                if (auto* player = dynamic_cast<Player*>(entity)) {
                    // MC 1.16.5: Mining Fatigue III，持续 6000 tick (5分钟)
                    // amplifier = 2 表示等级 III (0=I, 1=II, 2=III)
                    entity::effect::EffectInstance fatigue(entity::effect::EffectType::MiningFatigue,
                        6000,  // 持续时间: 5分钟
                        2,     // 等级 III
                        false, // 非环境效果（显示粒子）
                        true,  // 显示粒子
                        true   // 显示图标
                    );
                    player->addEffect(std::move(fatigue));
                }
            }
        }
    }
}

void ElderGuardianEntity::registerAttributes()
{
    // 调用父类方法
    GuardianEntity::registerAttributes();

    // 远古守卫者的属性
    // 参考 MC 1.16.5 远古守卫者属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 80.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 5.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 16.0);
}

} // namespace mc
