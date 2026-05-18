/*
* Copyright (c) 2026 Guo Yi
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
* 
*/

#include "BeeEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/Item.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/tag/ItemTags.hpp"
#include "../../../../util/math/MathUtils.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/FollowParentGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../ai/goal/goals/special/BeeGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../damage/DamageSource.hpp"
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

BeeEntity::BeeEntity(EntityId id)
    : AnimalEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> BeeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BeeEntity>(0);
}

void BeeEntity::registerData()
{
    AnimalEntity::registerData();

    // MC 1.16.5 BeeEntity.registerData()
    m_dataManager.registerParam(DATA_FLAGS_PARAM, static_cast<i8>(0));
    m_dataManager.registerParam(ANGER_TIME_PARAM, static_cast<i32>(0));
}

// ============================================================================
// 数据参数辅助方法
// ============================================================================

bool BeeEntity::getBeeFlag(i8 flag) const
{
    return (m_dataManager.get(DATA_FLAGS_PARAM) & flag) != 0;
}

void BeeEntity::setBeeFlag(i8 flag, bool value)
{
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

bool BeeEntity::hasNectar() const
{
    return getBeeFlag(FLAG_HAS_NECTAR);
}

void BeeEntity::setHasNectar(bool nectar)
{
    if (nectar != m_hasNectar) {
        m_hasNectar = nectar;
        setBeeFlag(FLAG_HAS_NECTAR, nectar);
    }
}

bool BeeEntity::hasStung() const
{
    return getBeeFlag(FLAG_HAS_STUNG);
}

void BeeEntity::setHasStung(bool stung)
{
    if (stung != m_hasStung) {
        m_hasStung = stung;
        setBeeFlag(FLAG_HAS_STUNG, stung);
    }
}

// ============================================================================
// IAngerable 接口实现
// ============================================================================

i32 BeeEntity::getAngerTime() const
{
    return m_dataManager.get(ANGER_TIME_PARAM);
}

void BeeEntity::setAngerTime(i32 time)
{
    m_angerTime = time;
    m_dataManager.set(ANGER_TIME_PARAM, time);
}

void BeeEntity::setAngry(bool angry)
{
    if (angry) {
        // MC 1.16.5: 设置随机愤怒时间 (20-39 ticks)
        // 这里简化为设置最大愤怒时间
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
    }
}

void BeeEntity::setRevengeTarget(LivingEntity* target)
{
    m_attackTarget = target;
    if (target != nullptr) {
        setAngry(true);
        m_revengeTargetId = target->id();
        m_revengeTimer = MAX_ANGER_TIME;
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* BeeEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void BeeEntity::updateAnger()
{
    i32 angerTime = getAngerTime();
    if (angerTime > 0) {
        setAngerTime(angerTime - 1);
        if (getAngerTime() == 0) {
            // 愤怒结束，清除攻击目标
            m_attackTarget = nullptr;
            m_attacking = false;
            m_targetPlayerId = 0;
            m_revengeTargetId = std::nullopt;
        }
    }
    // 更新复仇计时器
    if (m_revengeTimer > 0) {
        m_revengeTimer--;
    }
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::setHivePos(const BlockPos& pos)
{
    m_hivePos = pos;
    m_hasHive = true;
}

void BeeEntity::setFlowerPos(const BlockPos& pos)
{
    m_flowerPos = pos;
    m_hasFlower = true;
}

bool BeeEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // MC 1.16.5: 检查物品是否在花朵标签中
    // 参考: BeeEntity.isBreedingItem(ItemStack stack)
    // return stack.getItem().isIn(ItemTags.FLOWERS);
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item->isIn(item::tag::ItemTags::FLOWERS());
}

std::unique_ptr<AnimalEntity> BeeEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // MC 1.16.5: 创建小蜜蜂
    // 参考: BeeEntity.createChild(ServerWorld, AgeableEntity)
    auto baby = std::make_unique<BeeEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::tick()
{
    AnimalEntity::tick();

    // MC 1.16.5: 更新愤怒计时器
    updateAnger();

    // MC 1.16.5: 螫刺后逐渐死亡
    // 参考 BeeEntity.updateAITasks() 中的逻辑:
    // if (flag) {
    //     ++this.timeSinceSting;
    //     if (this.timeSinceSting % 5 == 0 && this.rand.nextInt(MathHelper.clamp(1200 - this.timeSinceSting, 1, 1200)) == 0) {
    //         this.attackEntityFrom(DamageSource.GENERIC, this.getHealth());
    //     }
    // }
    if (m_hasStung) {
        ++m_timeSinceSting;
        // 每 5 tick 检查一次死亡概率
        if (m_timeSinceSting % 5 == 0 && m_world != nullptr) {
            // 概率随时间增加：MathHelper.clamp(1200 - timeSinceSting, 1, 1200)
            // 越久越容易死亡，最长存活 1200 tick = 60 秒
            i32 deathChance = math::clamp(1200 - m_timeSinceSting, 1, 1200);

            // 获取随机数生成器
            math::Random& rng = m_world->getRandom();

            // rand.nextInt(deathChance) == 0 时死亡
            if (rng.nextInt(deathChance) == 0) {
                // 造成 GENERIC 伤害，伤害量为当前生命值
                auto damageSource = DamageSources::generic();
                hurt(damageSource, health());
            }
        }
    }

    // MC 1.16.5: 水下溺水逻辑
    // 参考: BeeEntity.livingTick() 第311-319行
    // if (this.isInWaterOrBubbleColumn()) {
    //     ++this.underWaterTicks;
    // } else {
    //     this.underWaterTicks = 0;
    // }
    // if (this.underWaterTicks > 20) {
    //     this.attackEntityFrom(DamageSource.DROWN, 1.0F);
    // }
    if (isInWater()) {
        ++m_underWaterTimer;
        if (m_underWaterTimer > 20 && m_world != nullptr) {
            // 开始溺水伤害
            auto damageSource = DamageSources::drown();
            hurt(damageSource, 1.0f);
        }
    } else {
        m_underWaterTimer = 0;
    }
}

void BeeEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // MC 1.16.5 BeeEntity.registerGoals()
    // 优先级越小越高

    // ========== Goal Selector (行为目标) ==========

    // 优先级 0: 蛰刺攻击（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::BeeStingGoal>(this));

    // 优先级 1: 进入蜂巢
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::BeeEnterHiveGoal>(this));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 花朵诱惑（使用花朵物品）
    m_goalSelector.addGoal(3, std::make_unique<entity::ai::goal::TemptGoal>(
        this, 1.25, [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && item->isIn(item::tag::ItemTags::FLOWERS());
        }, false));

    // 优先级 4: 授粉
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::BeePollinateGoal>(this));

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.25));

    // 优先级 5: 更新蜂巢位置
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::BeeUpdateHiveGoal>(this));

    // 优先级 5: 寻找蜂巢
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::BeeFindHiveGoal>(this));

    // 优先级 6: 寻找花朵
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::BeeFindFlowerGoal>(this));

    // 优先级 7: 寻找授粉目标（农作物）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::BeeFindPollinationTargetGoal>(this));

    // 优先级 8: 随机飞行
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::BeeWanderGoal>(this));

    // 优先级 9: 游泳
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // ========== Target Selector (目标选择) ==========

    // 优先级 1: 愤怒复仇（被攻击时召唤其他蜜蜂）
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::BeeAngerGoal>(this));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::BeeAttackPlayerGoal>(this, 10));

    // 优先级 3: 重置愤怒
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::BeeResetAngerGoal>(this));
}

void BeeEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MC 1.16.5 BeeEntity.registerAttributes()
    // 参考: BeeEntity.java 第489行
    // MAX_HEALTH: 10.0, FLYING_SPEED: 0.6, MOVEMENT_SPEED: 0.3,
    // ATTACK_DAMAGE: 2.0, FOLLOW_RANGE: 48.0

    // 注意：AnimalEntity 不注册 FLYING_SPEED 和 ATTACK_DAMAGE
    // 需要先注册这些属性才能设置值
    m_attributes.registerAttribute(*entity::attribute::Attributes::flyingSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());

    // 设置属性值
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

} // namespace mc
