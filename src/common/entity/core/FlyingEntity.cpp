#include "FlyingEntity.hpp"
#include "../ai/controller/MovementController.hpp"

namespace mc {

FlyingEntity::FlyingEntity(LegacyEntityType type, EntityId id)
    : MobEntity(type, id) {
    // 飞行生物默认不受重力影响
}

void FlyingEntity::travel(f32 x, f32 y, f32 z) {
    // 飞行移动逻辑
    if (m_flying) {
        // 空中移动，使用飞行速度
        f32 speed = static_cast<f32>(getAttributeValue("movement_speed", 0.1));
        // TODO: 实现飞行移动逻辑
        // 暂时使用基类方法
        MobEntity::travel(x, y, z);
    } else {
        // 使用默认陆地移动
        MobEntity::travel(x, y, z);
    }
}

} // namespace mc
