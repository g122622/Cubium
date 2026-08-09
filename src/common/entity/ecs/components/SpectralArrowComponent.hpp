#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 光灵箭状态组件
 *
 * 承载 SpectralArrowEntity::m_glowDuration。对齐 vanilla SpectralArrow 命中后
 * 给目标施加发光效果的字段。
 *
 * 仅 SpectralArrowEntity attach。光灵箭命中实体时按 m_glowDuration 施加发光状态
 * 效果（默认 200 tick = 10 秒）。
 *
 * 字段语义：
 * - m_glowDuration：发光持续时间（ticks，默认 200）。
 */
struct SpectralArrowComponent {
    i32 m_glowDuration{200};
};

} // namespace mc::ecs
