#include "ZombieVillagerEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

ZombieVillagerEntity::ZombieVillagerEntity(LegacyEntityType type, EntityId id)
    : ZombieEntity(type, id)
{
    // 僵尸村民比普通僵尸慢
}

std::unique_ptr<Entity> ZombieVillagerEntity::create(IWorld* /*world*/) {
    return std::make_unique<ZombieVillagerEntity>(LegacyEntityType::Unknown, 0);
}

void ZombieVillagerEntity::setConversionTime(i32 time) {
    m_conversionTime = time;
    m_converting = time > 0;
}

void ZombieVillagerEntity::startConverting() {
    m_converting = true;
    m_conversionTime = DEFAULT_CONVERSION_TIME;
}

void ZombieVillagerEntity::stopConverting() {
    m_converting = false;
    m_conversionTime = 0;
}

void ZombieVillagerEntity::finishConverting() {
    // TODO: 生成村民实体
    // 保留职业、类型、交易等级
    // auto villager = std::make_unique<VillagerEntity>(LegacyEntityType::Unknown, 0);
    // villager->setProfession(m_profession);
    // villager->setVillagerType(m_villagerType);
    // villager->setTradingLevel(m_tradingLevel);
    // world().spawnEntity(std::move(villager), position());
    // remove();
}

f32 ZombieVillagerEntity::eyeHeight() const {
    return isChild() ? 0.93f : 1.79f;
}

void ZombieVillagerEntity::tick() {
    ZombieEntity::tick();

    // 更新治愈倒计时
    if (m_converting && m_conversionTime > 0) {
        m_conversionTime--;

        // TODO: 治愈效果（ strength 效果会加速治愈）

        if (m_conversionTime <= 0) {
            finishConverting();
        }
    }
}

void ZombieVillagerEntity::registerGoals() {
    ZombieEntity::registerGoals();

    // 僵尸村民没有额外 AI
}

void ZombieVillagerEntity::registerAttributes() {
    ZombieEntity::registerAttributes();

    // 僵尸村民的属性与普通僵尸相同
}

} // namespace mc
