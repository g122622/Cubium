#pragma once

#include "common/util/AxisAlignedBB.hpp"

namespace mc::ecs {

/**
 * @brief 轴对齐包围盒组件
 *
 * 承载 Entity::m_boundingBox。对齐基岩版 AABBShapeComponent
 * （mc/deps/vanilla_components/AABBShapeComponent.h）。
 *
 * 基岩版另存 mBBDim（宽高 Vec2），项目尺寸由 width()/height() 虚函数驱动、
 * AABB::fromPosition 派生，首批不将尺寸虚函数组件化，故仅存 AABB 本体；
 * 尺寸可通过 AABB::width()/height() 派生访问。
 *
 * 默认值对齐 Entity::m_boundingBox 默认值（宽 0.6 × 高 1.8，底面中心 (0,0,0)）。
 */
struct AABBShapeComponent {
    // entt 指针稳定性：in_place_delete=true 保证本组件地址在 registry 生命周期内稳定
    // （erase 走 in_place_pop 不重排），是 Entity::m_builtIn.aabbShape 裸指针缓存的前提。
    // 详见 StateVectorComponent 同名注释与 BuiltInEntityComponents.hpp 契约。
    static constexpr bool in_place_delete = true;

    AxisAlignedBB m_aabb{-0.3f, 0.0f, -0.3f, 0.3f, 1.8f, 0.3f};
    // TODO: 后续将 Entity::width()/height() 虚函数结果固化为 m_bbDim，供批量系统访问
};

} // namespace mc::ecs
