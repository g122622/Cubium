#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 火球族状态组件
 *
 * 承载 AbstractFireballEntity::m_explosionPower（Fireball 用）与
 * WitherSkullEntity::m_blue（凋灵之首是否蓝色/危险）。对齐 vanilla Fireball
 * DATA_ITEM_STACK 与 WitherSkull DATA_DANGEROUS。
 *
 * Fireball 与 WitherSkull 分属不同类树（Fireball 经 AbstractFireball 继承，
 * WitherSkull 经 AbstractFireball 继承但 m_blue 在 WitherSkullEntity 自身），
 * 因字段稀疏且语义同属「火球族可视状态」，合并为单组件由两类共用：
 * - FireballEntity attach 时仅用 m_explosionPower。
 * - WitherSkullEntity attach 时仅用 m_blue（m_explosionPower 默认 1 不用）。
 *
 * 字段语义：
 * - m_explosionPower：爆炸威力（Fireball 默认 1，可由发射者蓄力提升）。
 * - m_blue：凋灵之首是否为蓝色（危险变体，破坏方块范围更大/同步 DATA_DANGEROUS）。
 * - m_itemStack：火球本体携带的物品（vanilla Fireball 的 Item 字段）。发射者手持火焰弹
 *   发射时写入，命中消亡时作为掉落物返还玩家。WitherSkull 不使用此字段。
 */
struct FireballStateComponent {
    i32 m_explosionPower{1};
    bool m_blue{false};
    std::unique_ptr<ItemStack> m_itemStack;

    FireballStateComponent()
        : m_itemStack(std::make_unique<ItemStack>())
    {}
};

} // namespace mc::ecs
