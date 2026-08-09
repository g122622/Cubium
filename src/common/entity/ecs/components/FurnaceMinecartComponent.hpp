#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 熔炉矿车组件
 *
 * 承载 FurnaceMinecartEntity 的燃料与推动方向状态。仅 FurnaceMinecartEntity attach。
 *
 * 字段语义：
 * - m_fuel：剩余燃料 tick 数。tick 时递减，耗尽则清推动方向。MAX_FUEL=32000（常量留 OOP）。
 * - m_pushX/m_pushZ：推动方向单位向量。熔炉矿车自动沿推动方向加速（applyDrag 应用 0.1 推力）。
 *   由 updatePushDirection/onActivatorRailPass 据当前速度更新。
 *
 * 纯 POD（i32+f32×3），隐式可移动，满足 entt 组件池要求。
 * 注意：本组件原 m_pushX/m_pushZ 在 B7.1-Step0 已删除基类 AbstractMinecartEntity 的同名死字段
 * （基类那对是 1.16.5 残留未用 ghost），本组件的是 FurnaceMinecart 真实使用的推动方向，
 * 两者无关联——FurnaceMinecart 字段从 id 8 起占自己的组件槽。
 */
struct FurnaceMinecartComponent {
    i32 m_fuel{0};
    f32 m_pushX{0.0f};
    f32 m_pushZ{0.0f};
};

} // namespace mc::ecs
