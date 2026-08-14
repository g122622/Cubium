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
#include "common/core/Types.hpp"
#include "common/entity/ai/controller/MovementController.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = MobEntity::classInfo()）。
// 本类无同步字段，classInfo 仅作父链遍历节点：子类 ClassRegisterGuard 沿父链查找最高 id
// 时穿过本类（lastAssignedId=-1）直达父链已分配 id 的基类，子类首字段续接其后。
const entity::EntityClassInfo& CreatureEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"CreatureEntity", &MobEntity::classInfo()};
    return s_classInfo;
}

CreatureEntity::CreatureEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : MobEntity(id, registry)
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

f32 CreatureEntity::getPathWeight(f32 x, f32 y, f32 z) const
{
    // 默认实现：返回0.0f（中性权重）
    // 子类应根据环境偏好重写此方法：
    // - AnimalEntity: 草方块返回10.0f，否则返回亮度相关值
    // - MonsterEntity: 返回 0.5f - 亮度（偏好黑暗）
    // - WaterMobEntity: 水中返回10.0f，否则返回0.0f
    // - 特殊实体（守卫者、炽足兽、疣猪兽等）有各自的偏好
    return 0.0f;
}

f32 CreatureEntity::getPathWeight(const BlockPos& pos) const
{
    return getPathWeight(static_cast<f32>(pos.x), static_cast<f32>(pos.y), static_cast<f32>(pos.z));
}

bool CreatureEntity::canSpawnAt(f32 x, f32 y, f32 z) const
{
    // 检查当前位置的路径权重是否非负
    // 对应 MC PathfinderMob.checkSpawnRules:
    // return this.getWalkTargetValue(this.blockPosition(), level) >= 0.0F;
    // 当 getPathWeight >= 0 时，表示该位置适合生成
    return getPathWeight(x, y, z) >= 0.0f;
}

} // namespace mc
