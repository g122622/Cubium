#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类跳跃组件（B 类强时序字段）
 *
 * 承载 AbstractHorseEntity 的跳跃蓄力与跳跃强度。仅 AbstractHorseEntity 子树 attach。
 *
 * 字段分类：B 类强时序——跳跃蓄力（jumpPower/isJumping/allowStandSliding/jumpCooldown）
 * 在 travel()/tick() 中强耦合跳跃状态机，逻辑留 OOP，数据搬组件。jumpStrength 为属性
 * 真相源，与 AttributeMap.HORSE_JUMP_STRENGTH 双份镜像（registerAttributes 读组件写
 * AttributeMap，序列化器 load 后同步 AttributeMap）。
 *
 * 持久化：仅 jumpStrength 进 NBT（HORSE_JUMP_STRENGTH）。其余运行时状态不存盘。
 *
 * 对齐 vanilla 1.21.11 AbstractHorse：jumpStrength 走 AttributeInstance
 * (horseJumpStrength)，项目用成员镜像 AttributeMap（同 m_speed/m_horseHealth 范式）。
 */
struct HorseJumpComponent {
    i32 m_jumpPower{0};       ///< 跳跃蓄力 (0-100，MC 1.16.5 jumpPower)
    f32 m_jumpStrength{0.0f}; ///< 基础跳跃强度（属性真相源，镜像 AttributeMap）
    bool m_isJumping{false};  ///< 是否正在跳跃过程中
    bool m_allowStandSliding{false}; ///< MC 1.16.5: 允许站立滑动
    i32 m_jumpCooldown{0};    ///< 跳跃冷却（tick）
};

} // namespace mc::ecs
