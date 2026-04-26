#include "SheepEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include <memory>
#include <vector>

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

std::vector<ItemStack> SheepEntity::shear(Player* /*player*/) {
    std::vector<ItemStack> drops;

    if (!m_hasWool) {
        return drops;
    }

    m_hasWool = false;

    // 根据羊毛颜色掉落对应羊毛
    // TODO: 使用正确的羊毛物品映射
    // 参考 MC 1.16.5: 掉落1-3个羊毛
    // 注：Items::WOOL 或类似物品尚未定义，暂时返回空
    // 实际应该根据 m_woolColor 获取对应颜色的羊毛

    // math::Random rng;
    // i32 woolCount = 1 + rng.nextInt(3);
    // drops.emplace_back(Items::getWoolByColor(m_woolColor), static_cast<u8>(woolCount));

    return drops;
}

bool SheepEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 羊用小麦繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT;
}

bool SheepEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> SheepEntity::spawnBaby(AnimalEntity& partner) {
    // 创建小羊
    auto baby = std::make_unique<SheepEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // TODO: 继承父母颜色混合
    // SheepEntity* partnerSheep = dynamic_cast<SheepEntity*>(&partner);
    // if (partnerSheep) {
    //     baby->setWoolColor(blendColors(m_woolColor, partnerSheep->getWoolColor()));
    // }

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void SheepEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 羊特有目标：食物诱惑（小麦）
    // MC 1.16.5: 优先级3，速度1.0
    m_goalSelector.addGoal(3, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.0,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && item == Items::WHEAT;
        },
        false));  // scaredByMovement

    // TODO: 添加吃草目标 (EatGrassGoal) 优先级5
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

    // 剪毛冷却更新
    if (m_shearCooldown > 0) {
        --m_shearCooldown;
    }

    // 羊毛重新生长
    if (!m_hasWool && m_shearCooldown <= 0) {
        // 吃草后有概率长出羊毛
        // TODO: 实现基于吃草的羊毛重新生长
    }

    AnimalEntity::tick();
}

} // namespace mc
