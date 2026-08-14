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

#include "common/entity/entities/passive/ambient/BatEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/goals/special/BatGoals.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/entities/passive/ambient/AmbientEntity.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include <cmath>
#include <memory>

namespace mc {

BatEntity::BatEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : AmbientEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 默认飞行
    m_flying = true;
}

std::unique_ptr<Entity> BatEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<BatEntity>(0, registry);
}

bool BatEntity::canRest() const
{
    // 检查上方是否有固体方块可以倒挂
    if (m_world == nullptr) {
        return false;
    }
    BlockPos above(static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.x)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.y + 1.0)),
        static_cast<i32>(std::floor(m_builtIn.stateVector->m_pos.z)));
    const BlockState* state = m_world->getBlockState(above);
    if (state == nullptr) {
        return false;
    }
    return state->getBlock().isSolid(*state);
}

void BatEntity::tick()
{
    AmbientEntity::tick();

    // 蝙蝠的飞行行为由AI目标系统控制
    // BatRandomFlyGoal 和 BatRestGoal 处理所有行为

    // 飞行时Y轴速度保留60%（垂直阻尼效果）
    if (m_flying && !m_resting) {
        math::Vector3 vel = velocity();
        setVelocity(vel.x, vel.y * 0.6f, vel.z);
    }

    // 休息状态下保持静止
    if (m_resting) {
        setVelocity(math::Vector3(0.0f, 0.0f, 0.0f));
    }
}

void BatEntity::registerGoals()
{
    // 蝙蝠使用独立的Goal类实现行为，遵循项目架构风格

    // 优先级 0: 随机飞行目标
    // 飞行时选择随机目标点，平滑转向飞行
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::BatRandomFlyGoal>(this));

    // 优先级 1: 挂墙休息目标
    // 白天尝试挂墙休息，玩家靠近或失去支撑时飞走
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::BatRestGoal>(this));
}

void BatEntity::registerAttributes()
{
    AmbientEntity::registerAttributes();

    // 蝙蝠属性：低生命值，固定飞行速度
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 6.0);
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, FLY_SPEED);
    attributes().setBaseValue(entity::attribute::Attributes::FLYING_SPEED, FLY_SPEED);
}

} // namespace mc
