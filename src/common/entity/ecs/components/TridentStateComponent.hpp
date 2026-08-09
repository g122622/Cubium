#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 三叉戟状态组件
 *
 * 承载 TridentEntity 的 6 字段状态。对齐 vanilla ThrownTrident 同步字段
 * （ID_LOYALTY / ID_FOIL）与持久化键（Trident item "Trident" / DealtDamage）。
 *
 * 仅 TridentEntity attach。**dealtDamage 不在本组件**：vanilla AbstractArrow 也有
 * dealtDamage，三叉戟复用父类字段（见 ProjectileArrowStateComponent::m_dealtDamage），
 * 由序列化器 load 顺序保证：本组件 priority=0 先 load item 重算 loyalty，再由
 * ArrowState priority=10 load dealtDamage。
 *
 * 设计要点：m_tridentStack（ItemStack 含不可移动语义）用 unique_ptr 包裹（沿用
 * AttributeComponent / ProjectileArrowStateComponent 范式）。
 *
 * 字段语义：
 * - m_tridentStack：三叉戟物品（持久化 "Trident" 键，附魔/耐久载体）。
 * - m_hitBlock：是否击中方块（返回逻辑判定）。
 * - m_returning：是否在返回射手中（同步 DATA_FOIL 由 loyalty 派生，本字段独立）。
 * - m_hitBlockPos：击中方块坐标。
 * - m_loyaltyLevel：忠诚附魔等级（同步 DATA_LOYALTY；持久化时不存盘，从 item 重算）。
 * - m_returningTicks：返回计时器。
 */
struct TridentStateComponent {
    std::unique_ptr<ItemStack> m_tridentStack;
    bool m_hitBlock{false};
    bool m_returning{false};
    BlockPos m_hitBlockPos{};
    u8 m_loyaltyLevel{0};
    i32 m_returningTicks{0};

    TridentStateComponent()
        : m_tridentStack(std::make_unique<ItemStack>())
    {}
};

} // namespace mc::ecs
