#include "ZombieEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

ZombieEntity::ZombieEntity(LegacyEntityType type, EntityId id)
    : MonsterEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 随机设置增援能力
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<i32> dist(1, 100);
    m_canSummonReinforcements = dist(gen) <= 5; // 5% 概率
}

std::unique_ptr<Entity> ZombieEntity::create(IWorld* /*world*/) {
    return std::make_unique<ZombieEntity>(LegacyEntityType::Unknown, 0);
}

void ZombieEntity::trySummonReinforcements() {
    if (!m_canSummonReinforcements || m_summoningReinforcements) {
        return;
    }

    // TODO: 在附近召唤另一个僵尸
    m_summoningReinforcements = true;
}

void ZombieEntity::tick() {
    MonsterEntity::tick();

    // 更新转化计时器
    if (m_converting && m_conversionTime > 0) {
        m_conversionTime--;
        if (m_conversionTime <= 0) {
            // TODO: 转化为溺尸
            m_converting = false;
        }
    }
}

void ZombieEntity::registerGoals() {
    // 调用父类方法
    MonsterEntity::registerGoals();

    // TODO: 僵尸 AI 目标
    // - ZombieAttackGoal: 近战攻击
    // - MoveThroughVillageGoal: 穿越村庄
    // - BreakDoorGoal: 破门
}

void ZombieEntity::registerAttributes() {
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 僵尸的属性
    // 参考 MC 1.16.5 僵尸属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::ARMOR, 2.0);
}

} // namespace mc
