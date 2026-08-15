#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 物理状态组件
 *
 * 承载 Entity::m_onGround / m_fallDistance / m_collidedHorizontally / m_collidedVertically 四字段。
 * 对齐基岩版 OnGroundFlagComponent（mc/entity/components/OnGroundFlagComponent.h）与
 * FallDistanceComponent / CollisionFlagComponent 族。
 *
 * 属 B 类字段（强时序内聚）：与 move() / checkOnGround() / updateFallDistance() 强耦合，
 * 拆成独立 System 会破坏"碰撞检测→清空速度→落地标记→累积坠落距离"的即时序链。故数据
 * 搬入组件、逻辑仍留 Entity::move 等 OOP 方法。热路径方法（baseTick/move）开头取一次
 * 组件指针缓存局部变量复用，避免多次 tryGetComponent。
 *
 * 字段语义：
 * - m_onGround：本帧是否贴地（checkOnGround 设定，影响坠落伤害判定与音效）。
 * - m_fallDistance：累计坠落距离。落地时按此计算坠落伤害，岩浆中 *=0.5 削减。
 * - m_collidedHorizontally/m_collidedVertically：本帧是否发生水平/垂直碰撞（move 设定）。
 */
struct PhysicsStateComponent {
    // entt 指针稳定性：in_place_delete=true 保证本组件地址在 registry 生命周期内稳定
    // （erase 走 in_place_pop 不重排），是 Entity::m_builtIn.physicsState 裸指针缓存的前提。
    // 详见 StateVectorComponent 同名注释与 BuiltInEntityComponents.hpp 契约。
    static constexpr bool in_place_delete = true;

    bool m_onGround{false};
    bool m_collidedHorizontally{false};
    bool m_collidedVertically{false};
    f32 m_fallDistance{0.0f};
};

} // namespace mc::ecs
