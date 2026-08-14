#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类属性组件（B 类属性字段）
 *
 * 承载 AbstractHorseEntity 的速度与生命值属性真相源。仅 AbstractHorseEntity 子树 attach。
 *
 * 字段分类：B 类属性——m_speed/m_horseHealth 为 NBT 真相源，与 AttributeMap
 * (MOVEMENT_SPEED/MAX_HEALTH) 双份镜像。registerAttributes 读组件写 AttributeMap，
 * 序列化器 load 后调 setBaseValue 同步 AttributeMap（priority=20 最后 load）。
 * setOffspringAttributes 遗传时同时写组件 + AttributeMap。
 *
 * 持久化：Speed（HORSE_SPEED f32）+ HorseHealth（HORSE_HEALTH f32）。
 *
 * 注意：m_horseHealth 改名规避与 LivingEntity.health 冲突（见 AbstractHorseEntity.hpp
 * 既有注释），是马特有"最大生命值"属性，与 LivingEntity 当前生命值不同。
 *
 * 对齐 vanilla 1.21.11 AbstractHorse：speed/health 走 AttributeInstance，
 * 项目用成员镜像 AttributeMap（与 jumpStrength 同范式）。
 */
struct HorseAttributeComponent {
    f32 m_speed{0.0f};        ///< 移动速度（属性真相源，镜像 AttributeMap.MOVEMENT_SPEED）
    f32 m_horseHealth{0.0f};  ///< 最大生命值（属性真相源，镜像 AttributeMap.MAX_HEALTH）
};

} // namespace mc::ecs
