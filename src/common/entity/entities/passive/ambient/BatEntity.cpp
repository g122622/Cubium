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
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/Block.hpp"
#include "../../../attribute/Attributes.hpp"

namespace mc {

BatEntity::BatEntity(LegacyEntityType type, EntityId id)
    : AmbientEntity(type, id)
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
    return std::make_unique<BatEntity>(LegacyEntityType::Unknown, 0);
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

    // MC 1.16.5: 检查是否是白天
    // 参考 BatEntity.tick() 第109-132行
    bool isDay = false;
    if (m_world != nullptr) {
        // dayTime() 返回一天中的时间 (0-23999)
        // 白天: 0-11999, 夜晚: 12000-23999
        i64 timeOfDay = m_world->dayTime() % 24000;
        isDay = timeOfDay < 12000;
    }

    // 简化逻辑：随机决定是否休息
    if (m_flying && !m_resting) {
        m_flyTimer++;

        // 检查是否可以休息
        if (canRest()) {
            math::Random rng = getRandom();
            if (rng.nextInt(1, 100) == 1) {
                m_resting = true;
                m_flying = false;
            }
        }
    }

    // 休息状态更新
    if (m_resting) {
        // MC 1.16.5: 在夜间或附近有玩家时唤醒
        // 暂时只检查白天/夜晚
        if (isDay) {
            // 白天保持休息
        } else {
            // 夜间唤醒并开始飞行
            m_resting = false;
            m_flying = true;
        }
    }
}

void BatEntity::registerGoals()
{
    // TODO: 蝙蝠 AI 目标
    // - BatFlyGoal: 随机飞行
    // - BatRestGoal: 休息
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
