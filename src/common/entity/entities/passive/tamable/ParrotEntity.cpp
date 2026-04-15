#include "ParrotEntity.hpp"

#include "../../../attribute/Attributes.hpp"

#include <random>

namespace mc {

ParrotEntity::ParrotEntity(LegacyEntityType type, EntityId id)
    : ShoulderRidingEntity(type, id)
{
    randomizeVariant();
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> ParrotEntity::create(IWorld* /*world*/)
{
    return std::make_unique<ParrotEntity>(LegacyEntityType::Unknown, 0);
}

void ParrotEntity::randomizeVariant()
{
    static std::random_device randomDevice;
    static std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<int> distribution(0, 4);
    m_variant = static_cast<ParrotVariant>(distribution(generator));
}

bool ParrotEntity::isTameItem(const ItemStack& itemStack) const
{
    // TODO: 对齐 1.16.5 的种子驯服标签判断。
    (void)itemStack;
    return false;
}

void ParrotEntity::tick()
{
    ShoulderRidingEntity::tick();

    if (isOnShoulder()) {
        return;
    }

    if (m_imitating) {
        --m_imitateTimer;
        if (m_imitateTimer <= 0) {
            m_imitating = false;
            m_imitatingTarget = 0;
        }
    }

    if (!m_imitating && isTamed()) {
        static std::random_device randomDevice;
        static std::mt19937 generator(randomDevice());
        std::uniform_int_distribution<i32> distribution(1, 100);
        if (distribution(generator) == 1) {
            m_imitateTimer = 60;
        }
    }

    if (m_flying) {
        m_flapSpeed = FLAP_SPEED_FLYING;
        ++m_flapTimer;
    } else {
        m_flapSpeed = FLAP_SPEED_GROUND;
        m_flapTimer = 0;
    }
}

void ParrotEntity::registerGoals()
{
    ShoulderRidingEntity::registerGoals();

    // TODO: 接入鹦鹉专属 Goal。
    // - LandOnOwnersShoulderGoal
    // - WaterAvoidingRandomFlyingGoal
    // - FollowOwnerGoal / FollowMobGoal
}

void ParrotEntity::registerAttributes()
{
    ShoulderRidingEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.4);
}

void ParrotEntity::onTamed(bool tamed)
{
    if (tamed) {
        // TODO: 如后续需要，可在此接入驯服粒子或额外同步。
    }
}

} // namespace mc
