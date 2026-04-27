#include "ChickenEntity.hpp"
#include "../../item/ItemEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/Items.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../util/math/random/Random.hpp"

#include <memory>

namespace mc {

std::unique_ptr<Entity> ChickenEntity::create(IWorld* /*world*/) {
    // 使用临时ID 0，实际ID由 EntityManager 分配
    // 注意：不要使用静态计数器，以避免线程安全问题和ID冲突
    return std::make_unique<ChickenEntity>(LegacyEntityType::Unknown, 0);
}

ChickenEntity::ChickenEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册属性
    registerAttributes();
    // 注册 AI 目标
    registerGoals();
    // 初始化下蛋计时器
    resetEggTimer();
}

void ChickenEntity::resetEggTimer() {
    math::Random rng(ticksExisted());
    // MC 1.16.5: 6000-12000 ticks = 5-10 分钟
    m_eggTimer = EGG_TIME_MIN + rng.nextInt(EGG_TIME_MAX - EGG_TIME_MIN);
}

bool ChickenEntity::isBreedingItem(const ItemStack& itemStack) const {
    // MC 1.16.5: 鸡用种子繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::WHEAT_SEEDS
        || item == Items::PUMPKIN_SEEDS
        || item == Items::MELON_SEEDS
        || item == Items::BEETROOT_SEEDS;
}

bool ChickenEntity::canMateWith(const AnimalEntity& other) const {
    return AnimalEntity::canMateWith(other);
}

std::unique_ptr<AnimalEntity> ChickenEntity::spawnBaby(AnimalEntity& /*partner*/) {
    // 创建小鸡
    auto baby = std::make_unique<ChickenEntity>(LegacyEntityType::Unknown, 0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void ChickenEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 鸡特有目标：食物诱惑（种子）
    // MC 1.16.5: 优先级3，速度1.0
    m_goalSelector.addGoal(3, std::make_unique<::mc::entity::ai::goal::TemptGoal>(
        this, 1.0,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (
                item == Items::WHEAT_SEEDS ||
                item == Items::PUMPKIN_SEEDS ||
                item == Items::MELON_SEEDS ||
                item == Items::BEETROOT_SEEDS
            );
        },
        false));  // scaredByMovement
}

void ChickenEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 鸡的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 4.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
}

void ChickenEntity::tick() {
    // 保存上一帧翅膀角度
    m_prevWingRotation = m_wingRotation;

    AnimalEntity::tick();

    // 翅膀动画
    // MC 1.16.5: 翅膀拍打动画
    constexpr f32 WING_FLAP_SPEED = 1.0f;
    constexpr f32 WING_DAMPING = 0.9f;
    m_wingRotDelta += WING_FLAP_SPEED;
    m_wingRotation += m_wingRotDelta;
    m_wingRotDelta *= WING_DAMPING;

    // 下蛋逻辑（仅服务端、仅成体、非鸡骑士）
    if (!isChild() && !m_chickenJockey && m_eggTimer > 0) {
        --m_eggTimer;

        if (m_eggTimer <= 0 && world() != nullptr) {
            // MC 1.16.5: 下蛋
            auto egg = std::make_unique<ItemEntity>(
                0,
                ItemStack(Items::EGG, 1),
                x(),
                y() + 0.2f,
                z());

            // 播放下蛋音效
            playSound(*makeSoundEventId("egg"), 1.0f, (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f + 1.0f);

            world()->spawnEntity(std::move(egg));
            resetEggTimer();
        }
    }
}

} // namespace mc
