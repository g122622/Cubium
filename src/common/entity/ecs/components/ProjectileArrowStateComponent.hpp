#pragma once

#include "common/core/Types.hpp"
#include "common/entity/core/PickupStatus.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <optional>
#include <unordered_set>

namespace mc::ecs {

/**
 * @brief 投射物箭矢状态组件
 *
 * 承载 AbstractArrowEntity 的 13 字段状态。对齐基岩版箭矢数据，vanilla AbstractArrow
 * 同步字段（ID_FLAGS/PIERCE_LEVEL/IN_GROUND）以本组件为真相源 + DataParameter 镜像。
 *
 * 仅 AbstractArrowEntity 子树 attach（ArrowEntity/SpectralArrowEntity/TridentEntity/
 * SpearEntity 共用）。**注意与既有 LivingEntity 的 ArrowStateComponent（箭矢计数）重名，
 * 本组件以 Projectile 前缀区分。**
 *
 * 设计要点：
 * - m_piercedEntities（unordered_set）含不可移动语义且 entt 组件池要求可移动，用
 *   unique_ptr 包裹（沿用 AttributeComponent 范式）。
 * - m_inBlockState（optional<BlockState>）同理用 unique_ptr 包裹，避免 optional 内嵌
 *   BlockState 在组件 swap 时触发重排成本与潜在不可移动问题。
 * - m_dealtDamage：vanilla AbstractArrow 也有此字段，仅 TridentEntity 用；三叉戟复用
 *   父类此字段不另存（见 TridentStateComponent 不重复声明）。
 *
 * 字段语义（对齐 vanilla AbstractArrow 持久化 life/inBlockState/shake/inGround/pickup/
 * damage/crit/PierceLevel）：
 * - m_damage：基础伤害（默认 2.0）。
 * - m_knockbackStrength：击退强度。
 * - m_critical：是否暴击（同步 DATA_ARROW_FLAGS bit0）。
 * - m_pierceLevel：穿透等级（同步 DATA_PIERCE_LEVEL）。
 * - m_inGround：是否插在方块中（同步 DATA_IN_GROUND）。
 * - m_ticksInGround：插在方块中的总时间（超时移除用）。
 * - m_timeInGround：当前连续插方块时间（三叉戟返回判定用）。
 * - m_arrowShake：箭矢抖动时间（vanilla "shake"）。
 * - m_pickupStatus：拾取状态（vanilla "pickup" byte）。
 * - m_shotFromCrossbow：是否从弩射出。
 * - m_dealtDamage：是否已造成伤害（三叉戟用，vanilla ThrownTrident "DealtDamage"）。
 */
struct ProjectileArrowStateComponent {
    f32 m_damage{2.0f};
    i32 m_knockbackStrength{0};
    bool m_critical{false};
    u8 m_pierceLevel{0};
    bool m_inGround{false};
    i32 m_ticksInGround{0};
    i32 m_timeInGround{0};
    i32 m_arrowShake{0};
    entity::PickupStatus m_pickupStatus{entity::PickupStatus::Disallowed};
    bool m_shotFromCrossbow{false};
    bool m_dealtDamage{false};

    std::unique_ptr<std::unordered_set<EntityInstanceId>> m_piercedEntities;
    std::unique_ptr<std::optional<BlockState>> m_inBlockState;

    ProjectileArrowStateComponent()
        : m_piercedEntities(std::make_unique<std::unordered_set<EntityInstanceId>>())
        , m_inBlockState(std::make_unique<std::optional<BlockState>>())
    {}
};

} // namespace mc::ecs
