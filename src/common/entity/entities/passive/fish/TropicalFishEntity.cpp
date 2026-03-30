#include "TropicalFishEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

TropicalFishEntity::TropicalFishEntity(LegacyEntityType type, EntityId id)
    : AbstractFishEntity(type, id)
{
    // 随机设置变种
    randomizeVariant();
}

std::unique_ptr<Entity> TropicalFishEntity::create(IWorld* /*world*/) {
    return std::make_unique<TropicalFishEntity>(LegacyEntityType::Unknown, 0);
}

TropicalFishEntity::FishShape TropicalFishEntity::getShape() const {
    return static_cast<FishShape>(m_variant & SHAPE_MASK);
}

u8 TropicalFishEntity::getBaseColor() const {
    return static_cast<u8>((m_variant & BASE_COLOR_MASK) >> 8);
}

u8 TropicalFishEntity::getPatternColor() const {
    return static_cast<u8>((m_variant & PATTERN_COLOR_MASK) >> 16);
}

void TropicalFishEntity::randomizeVariant() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // 随机选择形状
    std::uniform_int_distribution<int> shapeDist(0, 11);
    u8 shape = static_cast<u8>(shapeDist(gen));

    // 随机选择颜色
    std::uniform_int_distribution<int> colorDist(0, 15);
    u8 baseColor = static_cast<u8>(colorDist(gen));
    u8 patternColor = static_cast<u8>(colorDist(gen));

    // 编码变种
    m_variant = shape | (baseColor << 8) | (patternColor << 16);
}

void TropicalFishEntity::registerAttributes() {
    // 调用父类方法
    AbstractFishEntity::registerAttributes();

    // 热带鱼的属性
    // 参考 MC 1.16.5 热带鱼属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 3.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

} // namespace mc
