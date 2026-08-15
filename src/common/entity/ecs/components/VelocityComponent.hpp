#pragma once

#include "common/util/math/Vector3.hpp"

namespace mc::ecs {

/**
 * @brief 速度组件
 *
 * 承载 Entity::m_velocity。基岩版将速度并入 StateVectorComponent，本项目因
 * m_velocity 调用面广（addVelocity/multiplyVelocity 等数十处），单独拆出便于
 * 渐进迁移与系统按需查询。
 */
struct VelocityComponent {
    // entt 指针稳定性：in_place_delete=true 保证本组件地址在 registry 生命周期内稳定
    // （erase 走 in_place_pop 不重排），是 Entity::m_builtIn.velocity 裸指针缓存的前提。
    // 详见 StateVectorComponent 同名注释与 BuiltInEntityComponents.hpp 契约。
    static constexpr bool in_place_delete = true;

    Vector3 m_velocity{0.0f, 0.0f, 0.0f};
};

} // namespace mc::ecs
