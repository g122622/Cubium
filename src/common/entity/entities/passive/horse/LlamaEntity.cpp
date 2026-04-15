#include "LlamaEntity.hpp"

#include "../../../attribute/Attributes.hpp"
#include "../../../../util/math/random/Random.hpp"

#include <algorithm>

namespace mc {

LlamaEntity::LlamaEntity(LegacyEntityType type, EntityId id)
    : AbstractChestedHorseEntity(type, id)
{
    randomizeAppearance();
}

std::unique_ptr<Entity> LlamaEntity::create(IWorld* /*world*/)
{
    return std::make_unique<LlamaEntity>(LegacyEntityType::Unknown, 0);
}

void LlamaEntity::randomizeAppearance()
{
    math::Random random(ticksExisted());
    m_color = static_cast<LlamaColor>(random.nextInt(4));
    setStrength(1 + random.nextInt(5));
}

bool LlamaEntity::canBeRiddenBy(Player* player) const
{
    if (m_rider != nullptr && m_rider != player) {
        return false;
    }

    return true;
}

i32 LlamaEntity::getInventoryColumns() const
{
    return m_strength;
}

void LlamaEntity::setStrength(i32 strength)
{
    m_strength = std::clamp(strength, 1, 5);
}

bool LlamaEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // TODO: 对齐 1.16.5 的小麦 / 干草块喂食与繁殖语义。
    (void)itemStack;
    return false;
}

bool LlamaEntity::isTameItem(const ItemStack& /*itemStack*/) const
{
    return false;
}

std::unique_ptr<AnimalEntity> LlamaEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 对齐 1.16.5 的羊驼后代颜色 / 强度遗传语义。
    (void)partner;
    return nullptr;
}

void LlamaEntity::tick()
{
    AbstractChestedHorseEntity::tick();

    if (m_spitCooldown > 0) {
        --m_spitCooldown;
    }

    if (m_inCaravan && m_caravanLeader != nullptr) {
        // TODO: 补齐商队跟随逻辑。
    }
}

void LlamaEntity::registerGoals()
{
    AbstractChestedHorseEntity::registerGoals();
    // TODO: 补齐羊驼的 FollowCaravan / RangedAttack 等 Goal。
}

void LlamaEntity::registerAttributes()
{
    AbstractChestedHorseEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 15.0f + static_cast<f32>(m_strength) * 5.0f);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.175f);
}

} // namespace mc
