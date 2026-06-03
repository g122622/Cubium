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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "CatEntity.hpp"

#include "core/Types.hpp"
#include "entity/ai/goal/GoalSelector.hpp"
#include "entity/ai/goal/goals/BreedGoal.hpp"
#include "entity/ai/goal/goals/FollowParentGoal.hpp"
#include "entity/ai/goal/goals/LookAtGoal.hpp"
#include "entity/ai/goal/goals/PanicGoal.hpp"
#include "entity/ai/goal/goals/RandomWalkingGoal.hpp"
#include "entity/ai/goal/goals/SwimGoal.hpp"
#include "entity/ai/goal/goals/interact/TameableGoals.hpp"
#include "entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "entity/attribute/Attributes.hpp"
#include "entity/core/EntityRegistry.hpp"
#include "entity/core/LivingEntity.hpp"
#include "entity/core/MobEntity.hpp"
#include "entity/entities/player/Player.hpp"
#include "item/Items.hpp"
#include "item/core/ItemStack.hpp"
#include "sound/SoundEvents.hpp"
#include "util/math/random/Random.hpp"

namespace mc {

// ============================================================================
// CatTemptGoal 实现
// ============================================================================

CatEntity::CatTemptGoal::CatTemptGoal(CatEntity* cat, f64 speed, ItemPredicate itemPredicate, bool scaredByMovement)
    : TemptGoal(cat, speed, std::move(itemPredicate), scaredByMovement)
    , m_cat(cat)
{
    // 继承自 TemptGoal，重写 shouldExecute() 使其只在未驯服时执行
}

bool CatEntity::CatTemptGoal::shouldExecute()
{
    // 只有未驯服的猫才会被诱惑
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return TemptGoal::shouldExecute();
}

// ============================================================================
// CatAvoidPlayerGoal 实现
// ============================================================================

CatEntity::CatAvoidPlayerGoal::CatAvoidPlayerGoal(CatEntity* cat, f32 avoidDistance, f64 farSpeed, f64 nearSpeed)
    : AvoidEntityGoal(cat,
          avoidDistance,
          farSpeed,
          nearSpeed,
          // 只避开可以作为 AI 目标的玩家
          [](const LivingEntity* entity) -> bool {
              if (entity == nullptr) {
                  return false;
              }
              // 检查是否是玩家
              const Player* player = dynamic_cast<const Player*>(entity);
              if (player == nullptr) {
                  return false;
              }
              // 创造模式玩家也应该被避开
              return !player->isSpectator() && player->isAlive();
          })
    , m_cat(cat)
{
    // 继承自 AvoidEntityGoal，重写 shouldExecute() 和 shouldContinueExecuting() 使其只在未驯服时执行
}

bool CatEntity::CatAvoidPlayerGoal::shouldExecute()
{
    // 只有未驯服的猫才会避开玩家
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return AvoidEntityGoal::shouldExecute();
}

bool CatEntity::CatAvoidPlayerGoal::shouldContinueExecuting()
{
    // 只有未驯服的猫才会继续避开玩家
    if (m_cat == nullptr || m_cat->isTamed()) {
        return false;
    }
    return AvoidEntityGoal::shouldContinueExecuting();
}

// ============================================================================
// CatEntity 实现
// ============================================================================

CatEntity::CatEntity(EntityId id)
    : TameableEntity(id)
{
    // 随机设置皮肤类型
    setRandomCatType();

    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> CatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<CatEntity>(0);
}

void CatEntity::setRandomCatType()
{
    math::Random rng = getRandom();
    m_catType = static_cast<CatType>(rng.nextInt(0, 10));
}

bool CatEntity::isTameItem(const ItemStack& itemStack) const
{
    // 猫用生鳕鱼或生鲑鱼驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::COD || item == Items::SALMON;
}

bool CatEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 驯服后用生鱼繁殖
    return isTameItem(itemStack);
}

bool CatEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 同繁殖物品
    return isTameItem(itemStack);
}

std::unique_ptr<AnimalEntity> CatEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // 创建小猫
    auto baby = std::make_unique<CatEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void CatEntity::registerGoals()
{
    // 注意：AnimalEntity 基类不注册任何 goal，所以这里需要注册完整的 AI 目标列表

    // 优先级 0: 游泳（最高优先级）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑（受到伤害或着火时）
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 1.5));

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 2: 繁殖（驯服后且成体）
    m_goalSelector.addGoal(2, new entity::ai::goal::BreedGoal(this, 0.8));

    // 优先级 3: 食物诱惑（生鱼用于驯服）
    // 注意：scaredByMovement = true，猫会被玩家快速移动吓跑
    m_temptGoal = new CatTemptGoal(
        this,
        TEMPT_SPEED,
        [](const ItemStack& stack) -> bool {
            const Item* item = stack.getItem();
            return item != nullptr && (item == Items::COD || item == Items::SALMON);
        },
        true); // scaredByMovement = true
    m_goalSelector.addGoal(3, m_temptGoal);

    // 优先级 4: 避开玩家（未驯服时）- 在 _setupTamedAI() 中动态添加
    // 初始时根据驯服状态添加
    _setupTamedAI();

    // 优先级 5: 跟随父母（幼体行为）
    m_goalSelector.addGoal(5, new entity::ai::goal::FollowParentGoal(this, 1.0));

    // 优先级 6: 跟随主人（驯服后）
    m_goalSelector.addGoal(6, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 5.0f, 10.0f, 32.0f));

    // 优先级 10: 避水随机漫步
    m_goalSelector.addGoal(10, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.8, 1.0000001E-5f));

    // 优先级 12: 看向玩家
    m_goalSelector.addGoal(12, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 13: 随机看向
    m_goalSelector.addGoal(13, new entity::ai::goal::LookRandomlyGoal(this));
}

void CatEntity::_setupTamedAI()
{
    // 动态添加/移除 AvoidPlayerGoal

    if (m_avoidPlayerGoal == nullptr) {
        // 创建避开玩家目标
        m_avoidPlayerGoal = new CatAvoidPlayerGoal(this, AVOID_DISTANCE, AVOID_FAR_SPEED, AVOID_NEAR_SPEED);
    }

    // 先移除已有的 AvoidPlayerGoal
    m_goalSelector.removeGoal(m_avoidPlayerGoal);

    // 如果未驯服，添加避开玩家目标
    if (!isTamed()) {
        m_goalSelector.addGoal(4, m_avoidPlayerGoal);
    }
}

void CatEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 猫的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
}

void CatEntity::onTamed(bool tamed)
{
    if (tamed) {
        // 驯服后增加生命值
        m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
        setHealth(10.0f);

        // 礼物计时器初始化
        m_giftTimer = GIFT_INTERVAL;
    } else {
        m_giftTimer = 0;
    }

    // 更新 AI（添加/移除 AvoidPlayerGoal）
    _setupTamedAI();
}

std::optional<ResourceLocation> CatEntity::getAmbientSound() const
{
    // 驯服后的猫使用 ENTITY_CAT_AMBIENT
    // 未驯服的流浪猫使用 ENTITY_CAT_STRAY_AMBIENT
    if (isTamed()) {
        return SoundEvents::ENTITY_CAT_AMBIENT;
    }
    return SoundEvents::ENTITY_CAT_STRAY_AMBIENT;
}

std::optional<ResourceLocation> CatEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_CAT_HURT;
}

std::optional<ResourceLocation> CatEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_CAT_DEATH;
}

void CatEntity::playEatSound()
{
    playSound(SoundEvents::ENTITY_CAT_EAT, 1.0f, 1.0f);
}

} // namespace mc
