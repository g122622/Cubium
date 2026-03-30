#include "SheepEntity.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../attribute/Attributes.hpp"
#include <memory>

namespace mc {

std::unique_ptr<Entity> SheepEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<SheepEntity>(LegacyEntityType::Unknown, 0);
}

SheepEntity::SheepEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
    // SheepEntity 有特殊的 EatGrassGoal，后续添加
}

i32 SheepEntity::shear() {
    if (!m_hasWool) {
        return 0;
    }

    m_hasWool = false;

    // 返回羊毛数量（1-3个，受幸运影响）
    // TODO: 实际掉落逻辑
    return 1;
}

bool SheepEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 羊用小麦繁殖
    // TODO: 检查是否是小麦
    (void)itemStack;
    return false;
}

bool SheepEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SheepEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // TODO: 创建小羊，继承父母颜色混合
    return nullptr;
}

void SheepEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 羊特有目标：食物诱惑（小麦）
    // m_goalSelector.addGoal(3, std::make_shared<entity::ai::goal::TemptGoal>(
    //     this, 1.0, [](const ItemStack& stack) { return stack.getItem() == Items::WHEAT; }));

    // TODO: 添加吃草目标 (EatGrassGoal)
}

void SheepEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 羊的属性
    // 参考 MC 1.16.5 SheepEntity 属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.23);
}

void SheepEntity::tick() {
    // 吃草动画更新
    if (m_eatAnimationTimer > 0) {
        --m_eatAnimationTimer;
    }

    // 羊毛重新生长
    if (!m_hasWool) {
        // 每吃一次草有概率长出羊毛
        // TODO: 实现羊毛重新生长
    }

    AnimalEntity::tick();
}

} // namespace mc
