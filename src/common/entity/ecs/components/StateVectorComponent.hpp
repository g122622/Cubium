#pragma once

#include "common/util/math/Vector3.hpp"

namespace mc::ecs {

/**
 * @brief 位置状态向量组件
 *
 * 持有实体的当前位置、上一帧位置。对齐基岩版 StateVectorComponent
 * （mc/deps/vanilla_components/StateVectorComponent.h），承载 Entity::m_position
 * 与 m_prevPosition 的高频数据。
 *
 * 基岩版另有 mPosDelta（本帧位移增量，用于插值），项目当前未实现插值位移，
 * 此字段暂留 TODO，后续批次接入客户端插值时补全。
 *
 * 精度：Vector3 为 f32，与现有 Entity::m_position 一致。
 */
struct StateVectorComponent {
    // entt 指针稳定性：in_place_delete=true 使本组件在实体被 erase 时走 in_place_pop
    // （原地标记 tombstone，不移动 packed array 中其他元素），保证已 emplace 组件的
    // 数据地址在 registry 生命周期内稳定。这是 Entity::m_builtIn 缓存裸指针的前提——
    // 默认 in_place_delete=false（可移动类型）时 erase 走 swap_and_pop，会把末尾元素
    // move 到被删位置，导致缓存了末尾元素地址的其他实体 m_builtIn 指针悬垂（实测：全批
    // GameTest 共享 EntityManager 时，某实体 erase 触发重排，存活实体的 m_pos 读到别的
    // 实体数据）。详见 BuiltInEntityComponents.hpp 指针稳定性契约。
    static constexpr bool in_place_delete = true;

    Vector3 m_pos{0.0f, 0.0f, 0.0f};
    Vector3 m_posPrev{0.0f, 0.0f, 0.0f};
    // TODO: 接入客户端位置插值时补 m_posDelta（本帧位移增量）
};

} // namespace mc::ecs
