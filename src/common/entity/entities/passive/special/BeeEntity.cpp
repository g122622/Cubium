#include "BeeEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../../world/IWorld.hpp"
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 静态数据参数定义
// ============================================================================

// MC 1.16.5 BeeEntity 数据参数
entity::DataParameter<i8> BeeEntity::DATA_FLAGS_PARAM{0};
entity::DataParameter<i32> BeeEntity::ANGER_TIME_PARAM{1};

// ============================================================================
// 构造与生命周期
// ============================================================================

BeeEntity::BeeEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> BeeEntity::create(IWorld* /*world*/) {
    return std::make_unique<BeeEntity>(LegacyEntityType::Unknown, 0);
}

void BeeEntity::registerData() {
    AnimalEntity::registerData();

    // MC 1.16.5 BeeEntity.registerData()
    m_dataManager.registerParam(DATA_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(ANGER_TIME_PARAM, static_cast<i32>(0));
}

// ============================================================================
// 数据参数辅助方法
// ============================================================================

bool BeeEntity::getBeeFlag(i8 flag) const {
    return (m_dataManager.get(DATA_FLAGS_PARAM) & flag) != 0;
}

void BeeEntity::setBeeFlag(i8 flag, bool value) {
    i8 flags = m_dataManager.get(DATA_FLAGS_PARAM);
    if (value) {
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(flags | flag));
    } else {
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(flags & ~flag));
    }
}

// ============================================================================
// 花粉状态（使用 DataParameter 同步）
// ============================================================================

bool BeeEntity::hasNectar() const {
    return getBeeFlag(FLAG_HAS_NECTAR);
}

void BeeEntity::setHasNectar(bool nectar) {
    if (nectar != m_hasNectar) {
        m_hasNectar = nectar;
        setBeeFlag(FLAG_HAS_NECTAR, nectar);
    }
}

bool BeeEntity::hasStung() const {
    return getBeeFlag(FLAG_HAS_STUNG);
}

void BeeEntity::setHasStung(bool stung) {
    if (stung != m_hasStung) {
        m_hasStung = stung;
        setBeeFlag(FLAG_HAS_STUNG, stung);
    }
}

// ============================================================================
// IAngerable 接口实现
// ============================================================================

i32 BeeEntity::getAngerTime() const {
    return m_dataManager.get(ANGER_TIME_PARAM);
}

void BeeEntity::setAngerTime(i32 time) {
    m_angerTime = time;
    m_dataManager.set(ANGER_TIME_PARAM, time);
}

void BeeEntity::setAngry(bool angry) {
    if (angry) {
        // MC 1.16.5: 设置随机愤怒时间 (20-39 ticks)
        // 这里简化为设置最大愤怒时间
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
    }
}

void BeeEntity::setRevengeTarget(LivingEntity* target) {
    m_attackTarget = target;
    if (target != nullptr) {
        setAngry(true);
    }
}

void BeeEntity::updateAnger() {
    i32 angerTime = getAngerTime();
    if (angerTime > 0) {
        setAngerTime(angerTime - 1);
        if (getAngerTime() == 0) {
            // 愤怒结束，清除攻击目标
            m_attackTarget = nullptr;
            m_attacking = false;
            m_targetPlayerId = 0;
        }
    }
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::setHivePos(const BlockPos& pos) {
    m_hivePos = pos;
    m_hasHive = true;
}

void BeeEntity::setFlowerPos(const BlockPos& pos) {
    m_flowerPos = pos;
    m_hasFlower = true;
}

bool BeeEntity::isBreedingItem(const ItemStack& itemStack) const {
    // TODO: 检查是否是花朵
    // return itemStack.getItem().isIn(ItemTags::FLOWERS);
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> BeeEntity::spawnBaby(AnimalEntity& partner) {
    // TODO: 创建小蜜蜂
    // auto baby = std::make_unique<BeeEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // return baby;
    (void)partner;
    return nullptr;
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::tick() {
    AnimalEntity::tick();

    // MC 1.16.5: 更新愤怒计时器
    updateAnger();

    // 螫刺后死亡处理
    if (m_hasStung) {
        // 螫刺后逐渐死亡
        // TODO: 每 tick 有概率死亡
    }

    // 水下计时
    // TODO: 检查是否在水中
    // if (isInWater()) {
    //     m_underWaterTimer++;
    //     if (m_underWaterTimer > 20) {
    //         // 开始溺水
    //     }
    // } else {
    //     m_underWaterTimer = 0;
    // }
}

void BeeEntity::registerGoals() {
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 蜜蜂特有目标
    // 优先级 3: 食物诱惑（花朵）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isFlowerPredicate));

    // TODO: 蜜蜂特有目标
    // - BeeFindFlowerGoal: 寻找花朵
    // - BeePollinateGoal: 授粉
    // - BeeReturnToHiveGoal: 返回蜂巢
    // - BeeAttackGoal: 攻击目标
    // - BeeWanderGoal: 飞行漫步
}

void BeeEntity::registerAttributes() {
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 蜜蜂的属性
    // 参考 MC 1.16.5 蜜蜂属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);

    // 蜜蜂飞行速度较高
    // TODO: 设置飞行速度属性
}

} // namespace mc
