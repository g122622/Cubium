#include "PigEntity.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include <memory>

namespace mc {

std::unique_ptr<Entity> PigEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<PigEntity>(LegacyEntityType::Unknown, 0);
}

PigEntity::PigEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();
}

bool PigEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 猪用胡萝卜繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::CARROT;
}

bool PigEntity::canMateWith(const AnimalEntity& other) const {
    // 检查是否是猪
    // TODO: 类型检查
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> PigEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // 创建小猪
    auto baby = std::make_unique<PigEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void PigEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 猪特有目标：食物诱惑（胡萝卜）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.2, [](const ItemStack& stack) { return stack.getItem() == Items::CARROT; }));
}

void PigEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 猪的属性
    // 参考 MC 1.16.5 PigEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

} // namespace mc
