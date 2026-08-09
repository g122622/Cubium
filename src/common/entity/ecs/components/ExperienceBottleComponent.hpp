#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 经验瓶投掷物组件
 *
 * 承载 ExperienceBottleEntity（ProjectileItemEntity 子类）的 experience 字段：
 * 命中时释放的经验值。对齐 vanilla ThrownExperienceBottle 命中破碎释放经验。
 *
 * 仅 ExperienceBottleEntity attach。
 *
 * 字段语义：
 * - m_experience：命中释放的经验值量（默认由破碎时随机 3-11，本字段承载运行时值）。
 */
struct ExperienceBottleComponent {
    i32 m_experience{0};
};

} // namespace mc::ecs
