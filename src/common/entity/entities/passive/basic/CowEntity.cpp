#include "CowEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include <memory>

namespace mc {

std::unique_ptr<Entity> CowEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<CowEntity>(LegacyEntityType::Unknown, 0);
}

CowEntity::CowEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
}

bool CowEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 牛用小麦繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT;
}

bool CowEntity::canMateWith(const AnimalEntity& other) const {
    // 检查是否是牛
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> CowEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // 创建小牛
    auto baby = std::make_unique<CowEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void CowEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 牛特有目标：食物诱惑（小麦）
    // MC 1.16.5: 优先级3，速度1.0
    m_goalSelector.addGoal(3, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.0,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && item == Items::WHEAT;
        },
        false));  // scaredByMovement
}

void CowEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 牛的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
}

} // namespace mc
