#include "CatEntity.hpp"
#include "../../../core/Types.hpp"
#include "../../../item/ItemStack.hpp"
#include "../../core/EntityRegistry.hpp"
#include "../../ai/goal/GoalSelector.hpp"
#include "../../ai/goal/goals/SwimGoal.hpp"
#include "../../ai/goal/goals/PanicGoal.hpp"
#include "../../ai/goal/goals/BreedGoal.hpp"
#include "../../ai/goal/goals/TemptGoal.hpp"
#include "../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../ai/goal/goals/LookAtGoal.hpp"
#include "../../ai/goal/goals/AvoidEntityGoal.hpp"
#include "../../ai/goal/goals/interact/TameableGoals.hpp"
#include "../../attribute/Attributes.hpp"
#include "../../core/MobEntity.hpp"
#include <random>

namespace mc {

CatEntity::CatEntity(LegacyEntityType type, EntityId id)
    : TameableEntity(type, id)
{
    // 随机设置皮肤类型
    setRandomCatType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CatEntity::create(IWorld* /*world*/) {
    return std::make_unique<CatEntity>(LegacyEntityType::Unknown, 0);
}

void CatEntity::setRandomCatType() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<u8> dist(0, 10);
    m_catType = static_cast<CatType>(dist(gen));
}

bool CatEntity::isTameItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是生鳕鱼或生鲑鱼
    // return itemStack.getItem() == Items::COD || itemStack.getItem() == Items::SALMON;
    (void)itemStack;
    return false;
}

bool CatEntity::isBreedingItem(const ItemStack& itemStack) const {
    // 驯服后用生鱼繁殖
    // TODO: 同上
    (void)itemStack;
    return false;
}

bool CatEntity::isFoodItem(const ItemStack& itemStack) const {
    // TODO: 同上
    (void)itemStack;
    return false;
}

void CatEntity::registerGoals() {
    // 调用父类方法
    TameableEntity::registerGoals();

    // 猫特有目标
    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 1: 坐下目标（驯服后）
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 2: 繁殖（当处于爱心状态时）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 1.0));

    // 优先级 3: 跟随主人（驯服后）
    m_goalSelector.addGoal(3, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 5.0f, 10.0f, 32.0f));

    // 优先级 4: 食物诱惑（生鱼用于驯服）
    // TODO: 需要实现鱼类诱惑
    // m_goalSelector.addGoal(4, new entity::ai::goal::TemptGoal(this, 0.6, isFishPredicate));

    // 优先级 5: 跟随父母（幼体行为）
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 6: 逃离玩家（未驯服时）
    // 未驯服的猫会逃离玩家
    // TODO: 需要 AvoidEntityGoal 支持
    // m_goalSelector.addGoal(6, new entity::ai::goal::AvoidEntityGoal(this, Player.class, 16.0f, 0.8, 1.33));

    // 优先级 7: 随机漫步
    m_goalSelector.addGoal(7, new entity::ai::goal::RandomWalkingGoal(this, 0.8));

    // 优先级 8: 看向玩家
    m_goalSelector.addGoal(8, new entity::ai::goal::LookAtGoal(this, 10.0f));

    // 优先级 9: 随机看向
    m_goalSelector.addGoal(9, new entity::ai::goal::LookRandomlyGoal(this));
}

void CatEntity::registerAttributes() {
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 猫的属性
    // 参考 MC 1.16.5 猫属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void CatEntity::onTamed(bool tamed) {
    if (tamed) {
        // 驯服后增加生命值
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
        setHealth(10.0f);

        // 礼物计时器初始化
        m_giftTimer = GIFT_INTERVAL;
    } else {
        m_giftTimer = 0;
    }
}

} // namespace mc
