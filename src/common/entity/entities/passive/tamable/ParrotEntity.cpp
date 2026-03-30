#include "ParrotEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include <random>

namespace mc {

ParrotEntity::ParrotEntity(LegacyEntityType type, EntityId id)
    : TameableEntity(type, id)
{
    // 随机选择变种
    randomizeVariant();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> ParrotEntity::create(IWorld* /*world*/) {
    return std::make_unique<ParrotEntity>(LegacyEntityType::Unknown, 0);
}

void ParrotEntity::randomizeVariant() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, 4);
    m_variant = static_cast<ParrotVariant>(dist(gen));
}

bool ParrotEntity::mountShoulder(u64 playerId) {
    if (!isTamed() || isSitting()) {
        return false;
    }

    m_onShoulder = true;
    m_shoulderPlayerId = playerId;
    return true;
}

void ParrotEntity::dismountShoulder() {
    m_onShoulder = false;
    m_shoulderPlayerId = 0;
}

bool ParrotEntity::isTameItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是种子
    // return itemStack.getItem().isIn(ItemTags::PARROT_FOOD);
    (void)itemStack;
    return false;
}

void ParrotEntity::tick() {
    TameableEntity::tick();

    // 站肩膀时不更新其他状态
    if (m_onShoulder) {
        return;
    }

    // 更新模仿计时器
    if (m_imitating) {
        m_imitateTimer--;
        if (m_imitateTimer <= 0) {
            m_imitating = false;
            m_imitatingTarget = 0;
        }
    }

    // 随机模仿附近生物
    // TODO: 检测附近的敌对生物并模仿
    if (!m_imitating && isTamed()) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<i32> dist(1, 100);
        if (dist(gen) == 1) {
            // 开始模仿
            m_imitateTimer = 60;
        }
    }

    // 更新扑翼计时器
    if (m_flying) {
        m_flapSpeed = FLAP_SPEED_FLYING;
    } else {
        m_flapSpeed = FLAP_SPEED_GROUND;
    }
}

void ParrotEntity::registerGoals() {
    // 调用父类方法（已包含 SwimGoal, PanicGoal, BreedGoal, FollowParentGoal, RandomWalkingGoal, LookAtGoal, LookRandomlyGoal）
    TameableEntity::registerGoals();

    // 鹦鹉特有目标
    // 注意：不要重复注册父类已注册的Goal

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 2: 食物诱惑（种子）
    // m_goalSelector.addGoal(2, new entity::ai::goal::TemptGoal(this, 1.0, isSeedPredicate));

    // 优先级 3: 跟随主人（驯服后）- 鹦鹉可以飞行跟随
    // m_goalSelector.addGoal(3, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 5.0f, 1.0f, true));

    // TODO: 鹦鹉特有目标
    // - ParrotSitOnShoulderGoal: 站肩膀
    // - ParrotFlyGoal: 飞行
    // - ParrotDanceGoal: 唱片机旁跳舞
}

void ParrotEntity::registerAttributes() {
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 鹦鹉的属性
    // 参考 MC 1.16.5 鹦鹉属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.2);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.4);
}

void ParrotEntity::onTamed(bool tamed) {
    // 驯服后改变变种（可选）
    if (tamed) {
        // 驯服时可以随机改变颜色
        // randomizeVariant();
    }
}

} // namespace mc
