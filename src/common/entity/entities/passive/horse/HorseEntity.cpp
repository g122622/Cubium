#include "HorseEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

HorseEntity::HorseEntity(LegacyEntityType type, EntityId id)
    : AbstractHorseEntity(type, id)
{
    // 随机设置外观
    randomizeAppearance();
}

std::unique_ptr<Entity> HorseEntity::create(IWorld* /*world*/) {
    return std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
}

void HorseEntity::randomizeAppearance() {
    math::Random rng(ticksExisted());

    // 随机选择颜色和花纹
    m_color = static_cast<HorseColor>(rng.nextInt(7));
    m_marking = static_cast<HorseMarking>(rng.nextInt(5));
}

bool HorseEntity::isTameItem(const ItemStack& /*itemStack*/) const {
    // 马不响应特定驯服物品
    // 驯服是通过骑乘来完成的
    return false;
}

bool HorseEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是金苹果或金胡萝卜
    // return itemStack.getItem() == Items::GOLDEN_APPLE ||
    //        itemStack.getItem() == Items::GOLDEN_CARROT;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> HorseEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小马
    // auto baby = std::make_unique<HorseEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    //
    // // 遗传父母的属性
    // HorseEntity* horsePartner = dynamic_cast<HorseEntity*>(&partner);
    // if (horsePartner) {
    //     math::Random rng(ticksExisted());
    //     // 随机继承父母的一方
    //     if (rng.nextFloat() < 0.5f) {
    //         baby->setColor(m_color);
    //     } else {
    //         baby->setColor(horsePartner->getColor());
    //     }
    //     if (rng.nextFloat() < 0.5f) {
    //         baby->setMarking(m_marking);
    //     } else {
    //         baby->setMarking(horsePartner->getMarking());
    //     }
    // }
    //
    // return baby;
    (void)partner;
    return nullptr;
}

void HorseEntity::tick() {
    AbstractHorseEntity::tick();

    // 更新前腿站立状态
    if (m_isRearing) {
        m_rearingCounter--;
        if (m_rearingCounter <= 0) {
            m_isRearing = false;
        }
    }

    // 未驯服时，被骑乘会前腿站立
    if (!isTame() && isBeingRidden() && !m_isRearing) {
        math::Random rng(ticksExisted());
        if (rng.nextFloat() < 0.02f) {
            m_isRearing = true;
            m_rearingCounter = 20;
            // TODO: 甩下骑乘者
        }
    }
}

void HorseEntity::registerGoals() {
    // 调用父类方法
    AbstractHorseEntity::registerGoals();

    // TODO: 马特有 AI 目标
    // - RunAroundLikeCrazyGoal (未驯服时甩人)
}

void HorseEntity::registerAttributes() {
    AbstractHorseEntity::registerAttributes();

    // 马的基础属性已在父类初始化
    // 这里可以覆盖特定值
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, m_horseHealth);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, m_speed);
}

} // namespace mc
