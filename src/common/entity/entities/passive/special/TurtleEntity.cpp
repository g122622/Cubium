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

#include "TurtleEntity.hpp"
#include "../../../../core/Types.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../ai/goal/GoalSelector.hpp"
#include "../../../ai/goal/goals/BreedGoal.hpp"
#include "../../../ai/goal/goals/LookAtGoal.hpp"
#include "../../../ai/goal/goals/PanicGoal.hpp"
#include "../../../ai/goal/goals/RandomWalkingGoal.hpp"
#include "../../../ai/goal/goals/SwimGoal.hpp"
#include "../../../ai/goal/goals/TemptGoal.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"

namespace mc {

TurtleEntity::TurtleEntity(LegacyEntityType type, EntityId id)
    : AnimalEntity(type, id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();
}

std::unique_ptr<Entity> TurtleEntity::create(IWorld* /*world*/)
{
    return std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
}

void TurtleEntity::setHomePos(const BlockPos& pos)
{
    m_homePos = pos;
    m_hasHomePos = true;
}

bool TurtleEntity::isInWater() const
{
    // MC 1.16.5: 海龟在水中
    return Entity::isInWater();
}

bool TurtleEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // TODO: 检查是否是海草
    // return itemStack.getItem() == Items::SEAGRASS;
    (void)itemStack;
    return false;
}

std::unique_ptr<AnimalEntity> TurtleEntity::spawnBaby(AnimalEntity& partner)
{
    // TODO: 创建小海龟
    // auto baby = std::make_unique<TurtleEntity>(LegacyEntityType::Unknown, 0);
    // baby->setChild(true);
    // baby->setHomePos(m_homePos); // 继承出生地
    // return baby;
    (void)partner;
    return nullptr;
}

void TurtleEntity::tick()
{
    AnimalEntity::tick();

    // 更新产卵计时器
    if (m_layingEgg && m_layEggTimer > 0) {
        m_layEggTimer--;
        if (m_layEggTimer <= 0) {
            // 产卵完成
            m_layingEgg = false;
            m_hasEgg = false;
            // TODO: 在脚下生成海龟蛋方块
        }
    }
}

void TurtleEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    // AnimalEntity 已经注册了基础目标
    AnimalEntity::registerGoals();

    // 海龟特有目标
    // 优先级 3: 食物诱惑（海草）
    // m_goalSelector.addGoal(3, new entity::ai::goal::TemptGoal(this, 1.0, isSeagrassPredicate));

    // TODO: 海龟特有目标
    // - TurtleGoHomeGoal: 返回出生地
    // - TurtleLayEggGoal: 产卵
    // - TurtleTravelGoal: 前往海里
}

void TurtleEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // 海龟的属性
    // 参考 MC 1.16.5 海龟属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 30.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.25);
    // 陆地上移动更慢
    // TODO: 在陆地上时减慢速度
}

} // namespace mc
