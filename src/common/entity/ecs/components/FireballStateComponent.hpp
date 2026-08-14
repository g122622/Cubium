#pragma once

#include "common/core/Types.hpp"

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
 */
struct FireballStateComponent {
    i32 m_explosionPower{1};
    bool m_blue{false};
};

} // namespace mc::ecs
