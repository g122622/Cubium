#pragma once

#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 烟花火箭状态组件
 *
 * 承载 FireworkRocketEntity 的 5 字段：烟花物品 / 飞行等级 / 已存在时间 / 总生命时间 /
 * 是否从弩射出。对齐 vanilla FireworkRocketEntity 同步字段（DATA_FIREWORKS_ITEM /
 * DATA_ATTACHED_TO_TARGET / DATA_SHOT_AT_ANGLE）与持久化键（Life/LifeTime/
 * FireworksItem/ShotAtAngle）。
 *
 * 仅 FireworkRocketEntity attach。烟花火箭从物品 NBT 读取飞行等级，累计存活时间达
 * 总生命时爆炸。现有 FireworkRocketEntity 已有 OOP NBT override，Step6 搬到组件
 * 序列化器注册表。
 *
 * 设计要点：m_fireworkItem（ItemStack 含不可移动语义）用 unique_ptr 包裹（沿用
 * AttributeComponent 范式）。
 *
 * 字段语义：
 * - m_fireworkItem：烟花火箭物品（爆炸效果数据载体，同步 DATA_FIREWORKS_ITEM）。
 * - m_flightTime：飞行等级（从物品 NBT Fireworks.Flight 读取，决定总生命）。
 * - m_lifetime：已存在时间（每 tick 递增）。
 * - m_lifeTime：总生命时间（爆炸阈值，-1 表示尚未计算）。
 * - m_shotFromCrossbow：是否从弩射出（影响伤害逻辑）。
 */
struct FireworkRocketComponent {
    std::unique_ptr<ItemStack> m_fireworkItem;
    i32 m_flightTime{1};
    i32 m_lifetime{0};
    i32 m_lifeTime{-1};
    bool m_shotFromCrossbow{false};

    FireworkRocketComponent()
        : m_fireworkItem(std::make_unique<ItemStack>())
    {}
};

} // namespace mc::ecs
