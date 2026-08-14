#pragma once

#include "common/core/Types.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include <memory>
#include <vector>

namespace mc::ecs {

/**
 * @brief 药水箭效果组件
 *
 * 承载 ArrowEntity 的 3 字段：药水箭颜色 / 是否发光（药水箭特效）/ 药水效果列表。
 * 对齐 vanilla ArrowEntity 的药水箭数据（颜色由药水效果派生，效果列表持久化）。
 *
 * 仅 ArrowEntity attach（普通弓箭无此组件，药水箭/Tipped Arrow 才挂）。
 *
 * 设计要点：m_effects（vector<EffectInstance>）含不可移动语义（EffectInstance 可能
 * 含不可移动成员），用 unique_ptr 包裹（沿用 AttributeComponent 范式）。
 *
 * 字段语义：
 * - m_color：箭矢颜色（默认 0xFFFFFFFF 不染色，由药水效果派生）。
 * - m_glowing：是否发光（药水箭粒子特效门控）。
 * - m_effects：药水效果列表（命中实体时施加）。
 */
struct ArrowEffectsComponent {
    u32 m_color{0xFFFFFFFF};
    bool m_glowing{false};
    std::unique_ptr<std::vector<entity::effect::EffectInstance>> m_effects;

    ArrowEffectsComponent()
        : m_effects(std::make_unique<std::vector<entity::effect::EffectInstance>>())
    {}
};

} // namespace mc::ecs
