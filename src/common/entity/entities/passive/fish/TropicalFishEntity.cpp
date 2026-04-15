#include "TropicalFishEntity.hpp"

#include "../../../attribute/Attributes.hpp"

#include <random>

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
    static std::random_device randomDevice;
    static std::mt19937 generator(randomDevice());

    std::uniform_int_distribution<int> shapeDistribution(0, 11);
    std::uniform_int_distribution<int> colorDistribution(0, 15);

    const u8 shape = static_cast<u8>(shapeDistribution(generator));
    const u8 baseColor = static_cast<u8>(colorDistribution(generator));
    const u8 patternColor = static_cast<u8>(colorDistribution(generator));

    m_variant = shape | (baseColor << 8) | (patternColor << 16);
}

void TropicalFishEntity::registerAttributes()
{
    AbstractGroupFishEntity::registerAttributes();
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
