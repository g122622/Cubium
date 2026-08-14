#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类加速组件（A 类本地字段）
 *
 * 承载 AbstractHorseEntity 的加速状态。仅 AbstractHorseEntity 子树 attach。
 *
 * 字段分类：A 类纯本地无同步无持久化——boostTime/isBoosting 为运行时加速状态，
 * vanilla AbstractHorse 不存盘。updateBoost() 每 tick 递减 boostTime，归零清 isBoosting。
 */
struct HorseBoostComponent {
    i32 m_boostTime{0};    ///< 剩余加速时间（tick）
    bool m_isBoosting{false}; ///< 是否正在加速
};

} // namespace mc::ecs
