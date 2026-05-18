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

#include "BatEntity.hpp"
#include "../../../ai/goal/goals/special/BatGoals.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

BatEntity::BatEntity(EntityId id)
    : AmbientEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 默认飞行
    m_flying = true;
}

std::unique_ptr<Entity> BatEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BatEntity>(0);
}

bool BatEntity::canRest() const
{
    // MC 1.16.5: 检查上方是否有固体方块
    // 参考 BatEntity.canRest() 第82-88行
    if (m_world == nullptr) {
        return false;
    }
    BlockPos above(static_cast<i32>(std::floor(m_position.x)),
        static_cast<i32>(std::floor(m_position.y + 1.0)),
        static_cast<i32>(std::floor(m_position.z)));
    const BlockState* state = m_world->getBlockState(above);
    if (state == nullptr) {
        return false;
    }
    // 检查方块是否是固体的（可以挂在上面的）
    return state->getBlock().isSolid(*state);
}

void BatEntity::tick()
{
    AmbientEntity::tick();

    // 蝙蝠的飞行行为由AI目标系统控制
    // BatRandomFlyGoal 和 BatRestGoal 处理所有行为
    // 这里只需要处理一些状态同步

    // MC 1.16.5: 蝙蝠飞行时的垂直阻尼
    // 参考 BatEntity.tick() 第113行
    // this.setMotion(this.getMotion().mul(1.0D, 0.6D, 1.0D));
    if (m_flying && !m_resting) {
        // 飞行时Y轴速度保留60%
        math::Vector3 vel = velocity();
        setVelocity(vel.x, vel.y * 0.6f, vel.z);
    }

    // 休息状态下的位置对齐
    if (m_resting) {
        // 保持静止
        setVelocity(math::Vector3(0.0f, 0.0f, 0.0f));
    }
}

void BatEntity::registerGoals()
{
    // MC 1.16.5 蝙蝠AI目标
    // 注意：MC原版蝙蝠实际上不使用传统AI目标系统，
    // 而是在 updateAITasks() 中直接实现行为。
    // 这里将其拆分为独立的Goal类以遵循项目架构风格。

    // 优先级 0: 随机飞行目标
    // 飞行时选择随机目标点，平滑转向飞行
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::BatRandomFlyGoal>(this));

    // 优先级 1: 挂墙休息目标
    // 白天尝试挂墙休息，玩家靠近或失去支撑时飞走
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::BatRestGoal>(this));
}

void BatEntity::registerAttributes()
{
    // 调用父类方法
    AmbientEntity::registerAttributes();

    // 蝙蝠的属性
    // 参考 MC 1.16.5 蝙蝠属性
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, FLY_SPEED);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, FLY_SPEED);
}

} // namespace mc
