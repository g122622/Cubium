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
    Vector3 m_velocity{0.0f, 0.0f, 0.0f};
};

} // namespace mc::ecs
