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
    Vector3 m_pos{0.0f, 0.0f, 0.0f};
    Vector3 m_posPrev{0.0f, 0.0f, 0.0f};
    // TODO: 接入客户端位置插值时补 m_posDelta（本帧位移增量）
};

} // namespace mc::ecs
