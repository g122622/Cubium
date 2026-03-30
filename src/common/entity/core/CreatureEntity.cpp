#include "CreatureEntity.hpp"
#include "../ai/controller/MovementController.hpp"
#include "../ai/pathfinding/PathNavigator.hpp"

namespace mc {

CreatureEntity::CreatureEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id)
{
}

bool CreatureEntity::tryMoveTo(f64 x, f64 y, f64 z, f64 speed) {
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

f32 CreatureEntity::getPathWeight(f32 /*x*/, f32 /*y*/, f32 /*z*/) const {
    // 默认实现：返回0表示中性权重
    // 子类应该重写此方法来提供更准确的权重
    return 0.0f;
}

bool CreatureEntity::canSpawnAt(f32 /*x*/, f32 y, f32 /*z*/) const {
    // 默认实现：检查是否在有效位置
    // 子类应该重写此方法
    return y >= 0.0f;
}

} // namespace mc
