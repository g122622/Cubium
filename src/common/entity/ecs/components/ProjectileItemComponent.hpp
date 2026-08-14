#pragma once

#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 投掷物承载物品组件
 *
 * 承载 ProjectileItemEntity::m_itemStack（雪球/蛋/末影珍珠/药水/经验瓶等投掷物
 * 所承载的物品，用于显示与掉落）。对齐 vanilla ThrowableProjectile 的 Item 字段。
 *
 * 仅 ProjectileItemEntity 子树 attach（Snowball/Egg/EnderPearl/Potion/
 * ExperienceBottle 经 ProjectileItemEntity 继承，但 Potion/ExperienceBottle 另有
 * 专属组件承载额外字段，本组件承载共同的 itemStack）。
 *
 * 设计要点：m_itemStack（ItemStack 含不可移动语义）用 unique_ptr 包裹（沿用
 * AttributeComponent 范式）。
 *
 * 字段语义：
 * - m_itemStack：投掷物承载的物品（命中/消亡时掉落用，默认空）。
 */
struct ProjectileItemComponent {
    std::unique_ptr<ItemStack> m_itemStack;

    ProjectileItemComponent()
        : m_itemStack(std::make_unique<ItemStack>())
    {}
};

} // namespace mc::ecs
