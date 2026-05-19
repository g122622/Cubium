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

#include "CreatureEntity.hpp"
#include "../../core/Constants.hpp"
#include "../../world/block/BlockPos.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"

namespace mc {

CreatureEntity::CreatureEntity(EntityId id)
    : MobEntity(id)
{}

bool CreatureEntity::tryMoveTo(f64 x, f64 y, f64 z, f64 speed)
{
    // 首先尝试使用导航器（如果可用且有路径）
    if (m_navigator) {
        if (m_navigator->moveTo(x, y, z, speed)) {
            return true;
        }
    }

    // 如果导航失败或不可用，直接使用移动控制器
    // 这允许实体在没有完整寻路系统的情况下也能移动
    if (m_moveController) {
        m_moveController->setMoveTo(x, y, z, speed);
        return true;
    }

    return false;
}

f32 CreatureEntity::getPathWeight(f32 /*x*/, f32 /*y*/, f32 /*z*/) const
{
    // 默认实现：返回0表示中性权重
    // 子类应该重写此方法来提供更准确的权重
    // 参考 MC 1.16.5 CreatureEntity.getBlockPathWeight()
    return 0.0f;
}

f32 CreatureEntity::getPathWeight(const BlockPos& pos) const
{
    return getPathWeight(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
}

bool CreatureEntity::canSpawnAt(f32 /*x*/, f32 y, f32 /*z*/) const
{
    // 默认实现：检查是否在有效位置
    // 子类应该重写此方法
    return y >= static_cast<f32>(world::MIN_BUILD_HEIGHT);
}

} // namespace mc
