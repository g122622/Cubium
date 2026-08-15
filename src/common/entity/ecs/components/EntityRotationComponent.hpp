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
    // entt 指针稳定性：in_place_delete=true 保证本组件地址在 registry 生命周期内稳定
    // （erase 走 in_place_pop 不重排），是 Entity::m_builtIn.rotation 裸指针缓存的前提。
    // 详见 StateVectorComponent 同名注释与 BuiltInEntityComponents.hpp 契约。
    static constexpr bool in_place_delete = true;

    Vector2 m_rot{0.0f, 0.0f};     // 当前 yaw/pitch
    Vector2 m_rotPrev{0.0f, 0.0f}; // 上一帧 yaw/pitch
};

} // namespace mc::ecs
