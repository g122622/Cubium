/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so so, subject to the following conditions:
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

#include "SpiderEntity.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/MeleeAttackGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/movement/MovementGoals.hpp"
#include "../../../ai/goal/goals/target/TargetGoals.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../core/MobEntity.hpp"
#include "../../../entities/passive/golem/IronGolemEntity.hpp"
#include "../../../entities/player/Player.hpp"
#include "../../../registry/VanillaEntityTypeKeys.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include <cmath>
#include <memory>

namespace mc {

// ==================== SpiderAttackGoal ====================
// 蜘蛛专用的近战攻击目标，只在黑暗中继续攻击

/**
 * @brief 蜘蛛近战攻击目标
 *
 * 继承自MeleeAttackGoal，添加了光照条件检测：
 * - 只在亮度 < 0.5F 时继续攻击
 * - 在明亮环境中有1%概率放弃目标
 */
class SpiderAttackGoal : public entity::ai::goal::MeleeAttackGoal {
public:
    explicit SpiderAttackGoal(SpiderEntity* spider)
        : MeleeAttackGoal(spider, 1.0, true)
        , m_spider(spider)
    {}

    [[nodiscard]] bool shouldExecute() override
    {
        return MeleeAttackGoal::shouldExecute() && !m_spider->isBeingRidden();
    }

    [[nodiscard]] bool shouldContinueExecuting() override
    {
        // 检查光照条件，在明亮环境中有概率停止攻击
        f32 brightness = m_spider->getBrightness();
        if (brightness >= 0.5F && m_spider->getRandom().nextInt(100) == 0) {
            m_spider->setAttackTarget(nullptr);
            return false;
        }
        return MeleeAttackGoal::shouldContinueExecuting();
    }

protected:
    [[nodiscard]] f32 getAttackReachSqr(LivingEntity* target) const override { return 4.0F + target->width(); }

private:
    SpiderEntity* m_spider;
};

// ==================== SpiderTargetGoal ====================
// 蜘蛛专用的目标选择，只在黑暗中选择目标

/**
 * @brief 蜘蛛目标选择目标
 *
 * 继承自NearestAttackableTargetGoal，添加了光照条件检测：
 * - 只在亮度 < 0.5F 时选择攻击目标
 */
template <typename T>
class SpiderTargetGoal : public entity::ai::goal::NearestAttackableTargetGoal<T> {
public:
    SpiderTargetGoal(SpiderEntity* spider)
        : entity::ai::goal::NearestAttackableTargetGoal<T>(spider, true)
        , m_spider(spider)
    {}

    [[nodiscard]] bool shouldExecute() override
    {
        // 只在黑暗中选择目标
        f32 brightness = m_spider->getBrightness();
        if (brightness >= 0.5F) {
            return false;
        }
        return entity::ai::goal::NearestAttackableTargetGoal<T>::shouldExecute();
    }

private:
    SpiderEntity* m_spider;
};

// ==================== SpiderEntity ====================

SpiderEntity::SpiderEntity(EntityInstanceId id)
    : MonsterEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> SpiderEntity::create(IWorld* /*world*/)
{
    return std::make_unique<SpiderEntity>(EntityInstanceId(0));
}

bool SpiderEntity::shouldAttack(LivingEntity* target) const
{
    // 蜘蛛只在黑暗中攻击（光照等级 < 7）
    if (m_world != nullptr) {
        u8 lightLevel = m_world->getLightSubtracted(BlockPos(static_cast<i32>(std::floor(m_position.x)),
                                                        static_cast<i32>(std::floor(m_position.y)),
                                                        static_cast<i32>(std::floor(m_position.z))),
            0);
        if (lightLevel < 7) {
            return MonsterEntity::shouldAttack(target);
        }
        return false;
    }
    return MonsterEntity::shouldAttack(target);
}

void SpiderEntity::tick()
{
    MonsterEntity::tick();

    // 更新攀爬状态：蜘蛛在碰到墙壁时可以攀爬
    m_climbing = collidedHorizontally();

    m_wasOnGround = onGround();
}

void SpiderEntity::registerGoals()
{
    // 调用父类方法
    MonsterEntity::registerGoals();

    // 行为目标 (goalSelector)
    // 优先级 1: 游泳
    m_goalSelector.addGoal(1, new entity::ai::goal::SwimGoal(this));

    // 优先级 3: 跳向目标（力度 0.4F）
    m_goalSelector.addGoal(3, new entity::ai::goal::LeapAtTargetGoal(this, 0.4F));

    // 优先级 4: 近战攻击（蜘蛛专用，带光照检测）
    m_goalSelector.addGoal(4, new SpiderAttackGoal(this));

    // 优先级 5: 避水随机行走（速度 0.8D）
    m_goalSelector.addGoal(5, new entity::ai::goal::WaterAvoidingRandomWalkingGoal(this, 0.8));

    // 优先级 6: 看向玩家（8格距离）
    m_goalSelector.addGoal(
        6, new entity::ai::goal::LookAtGoal(this, 8.0F, 0.02F, [](const LivingEntity* entity) -> bool {
            return entity != nullptr && entity->entityType() == entity::VanillaEntityTypeKeys::PLAYER;
        }));

    // 优先级 6: 随机看向
    m_goalSelector.addGoal(6, new entity::ai::goal::LookRandomlyGoal(this));

    // 目标选择 (targetSelector)
    // 优先级 1: 被攻击后反击
    m_targetSelector.addGoal(1, new entity::ai::goal::HurtByTargetGoal(this, false));

    // 优先级 2: 攻击玩家（蜘蛛专用，带光照检测）
    m_targetSelector.addGoal(2, new SpiderTargetGoal<Player>(this));

    // 优先级 3: 攻击铁傀儡（蜘蛛专用，带光照检测）
    m_targetSelector.addGoal(3, new SpiderTargetGoal<IronGolemEntity>(this));
}

void SpiderEntity::registerAttributes()
{
    // 调用父类方法
    MonsterEntity::registerAttributes();

    // 蜘蛛的属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 16.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
}

} // namespace mc
