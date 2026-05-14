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

#include "PolarBearEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"

namespace mc {

PolarBearEntity::PolarBearEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> PolarBearEntity::create(IWorld* /*world*/)
{
    return std::make_unique<PolarBearEntity>(LegacyEntityType::Unknown, 0);
}

void PolarBearEntity::setStanding(bool standing)
{
    m_standing = standing;
    if (standing) {
        // 设置站立持续时间
        math::Random rng = getRandom();
        m_standTimer = rng.nextInt(STAND_DURATION_MIN, STAND_DURATION_MAX);
    }
}

void PolarBearEntity::setWarning(bool warning)
{
    m_warning = warning;
    if (warning) {
        m_warningTimer = WARNING_DURATION;
    }
}

void PolarBearEntity::tick()
{
    AnimalEntity::tick();

    // 更新站立计时器
    if (m_standing && m_standTimer > 0) {
        m_standTimer--;
        if (m_standTimer <= 0) {
            m_standing = false;
        }
    }

    // 更新警告计时器
    if (m_warning && m_warningTimer > 0) {
        m_warningTimer--;
        if (m_warningTimer <= 0) {
            m_warning = false;
        }
    }
}

void PolarBearEntity::registerGoals()
{
    // 北极熊不调用 AnimalEntity::registerGoals() 因为它没有繁殖行为

    // 优先级 0: 游泳（北极熊擅长游泳）
    m_goalSelector.addGoal(0, new entity::ai::goal::SwimGoal(this));

    // 优先级 1: 恐慌逃跑
    m_goalSelector.addGoal(1, new entity::ai::goal::PanicGoal(this, 2.0));

    // 优先级 2: 随机漫步
    m_goalSelector.addGoal(2, new entity::ai::goal::RandomWalkingGoal(this, 0.8));

    // 优先级 3: 看向玩家
    m_goalSelector.addGoal(3, new entity::ai::goal::LookAtGoal(this, 8.0f));

    // 优先级 4: 随机看向
    m_goalSelector.addGoal(4, new entity::ai::goal::LookRandomlyGoal(this));

    // TODO: 北极熊特有目标
    // - PolarBearAttackGoal: 保护幼崽攻击
    // - PolarBearStandGoal: 随机站立
}

void PolarBearEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 北极熊的属性
    // 参考 MC 1.16.5 北极熊属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 6.0);
}

} // namespace mc
