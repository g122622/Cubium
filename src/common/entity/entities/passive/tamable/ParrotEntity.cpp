#include "ParrotEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../../item/Items.hpp"
#include "../../../../util/math/random/Random.hpp"

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
    math::Random rng = getRandom();
    m_variant = static_cast<ParrotVariant>(rng.nextInt(0, 4));
}

bool ParrotEntity::isTameItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: 鹦鹉用种子驯服
    // 参考: net.minecraft.entity.passive.ParrotEntity.TAME_ITEMS
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item == Items::WHEAT_SEEDS
        || item == Items::PUMPKIN_SEEDS
        || item == Items::MELON_SEEDS
        || item == Items::BEETROOT_SEEDS;
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
        math::Random rng = getRandom();
        if (rng.nextInt(1, 100) == 1) {
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
