#pragma once

#include "common/util/math/Vector2.hpp"

namespace mc::ecs {

/**
 * @brief 实体旋转组件
 *
 * 承载 Entity::m_yaw/m_pitch/m_prevYaw/m_prevPitch。对齐基岩版 ActorRotationComponent
 * （mc/entity/components/ActorRotationComponent.h，基岩版用 Vec2 mRot/mRotPrev）。
 *
 * Vector2.x = yaw（偏航），Vector2.y = pitch（俯仰）。
 */
struct EntityRotationComponent {
    Vector2 m_rot{0.0f, 0.0f};      // 当前 yaw/pitch
    Vector2 m_rotPrev{0.0f, 0.0f};  // 上一帧 yaw/pitch
};

} // namespace mc::ecs
