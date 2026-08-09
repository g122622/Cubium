#pragma once

#include "common/core/Types.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief TNT 矿车组件
 *
 * 承载 TNTMinecartEntity 的引信与引爆来源状态。仅 TNTMinecartEntity attach。
 *
 * 字段语义：
 * - m_fuse：剩余引信 tick 数。-1 表示未点燃；prime() 设为正数启动倒计时，tick 时递减至 0 爆炸。
 * - m_ignitionSource：首次点燃时设置的伤害来源（用于爆炸归因，对应 vanilla ignitionSource）。
 *   首次设置后不再覆盖（_ignite 逻辑）。用 unique_ptr 持有（沿用原 OOP 成员模式），
 *   对象地址稳定，爆炸时传裸指针给 createExplosionWithSource（内部自行 clone）。
 *
 * unique_ptr<DamageSource> 经 include 完整头可见，默认析构能正确 delete（沿用
 * TntMinecartEntity 原 m_ignitionSource 范式）。unique_ptr noexcept 可移动满足 entt 要求。
 */
struct TntMinecartComponent {
    i32 m_fuse{-1}; ///< -1 表示未点燃
    std::unique_ptr<DamageSource> m_ignitionSource;
};

} // namespace mc::ecs
