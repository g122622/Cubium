#include "TropicalFishEntity.hpp"

#include "../../../../util/math/random/Random.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

TropicalFishEntity::TropicalFishEntity(LegacyEntityType type, EntityId id)
    : AbstractGroupFishEntity(type, id)
{
    randomizeVariant();
}

std::unique_ptr<Entity> TropicalFishEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TropicalFishEntity>(LegacyEntityType::Unknown, 0);
}

TropicalFishEntity::FishShape TropicalFishEntity::getShape() const
{
    return static_cast<FishShape>(m_variant & SHAPE_MASK);
}

u8 TropicalFishEntity::getBaseColor() const
{
    return static_cast<u8>((m_variant & BASE_COLOR_MASK) >> 8);
}

u8 TropicalFishEntity::getPatternColor() const
{
    return static_cast<u8>((m_variant & PATTERN_COLOR_MASK) >> 16);
}

void TropicalFishEntity::randomizeVariant()
{
    math::Random rng = getRandom();

    const u8 shape = static_cast<u8>(rng.nextInt(0, 11));
    const u8 baseColor = static_cast<u8>(rng.nextInt(0, 15));
    const u8 patternColor = static_cast<u8>(rng.nextInt(0, 15));

    m_variant = shape | (baseColor << 8) | (patternColor << 16);
}

void TropicalFishEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
