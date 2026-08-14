#pragma once

#include "common/core/Types.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 漏斗矿车组件
 *
 * 承载 HopperMinecartEntity 的 5 格库存、吸取冷却与红石禁用状态。
 * 仅 HopperMinecartEntity attach。
 *
 * 字段语义：
 * - m_inventory：5 格库存（INVENTORY_SIZE=5）。SimpleInventory 禁拷贝但 noexcept 可移动，
 *   沿用原 OOP 成员的 unique_ptr 持有模式（对象地址稳定，ContainerListener* 与
 *   LootUnpackCallback 捕获 this 不因 entt pool 扩容移动而失效）。
 * - m_suckCooldown：吸取/传输冷却（tick），递减至 0 可动作。TRANSFER_COOLDOWN=4（常量留 OOP）。
 * - m_disabled：红石禁用状态。onActivatorRailPass 据激活铁轨充能置位（powered=true 禁用）。
 *
 * m_inventory 初始为空，由 HopperMinecartEntity 构造函数 attach 后赋值
 * make_unique<SimpleInventory>(5)（库存大小构造期定，见 HopperMinecartEntity 构造）。
 */
struct HopperMinecartComponent {
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;
    i32 m_suckCooldown{0};
    bool m_disabled{false}; ///< 红石禁用状态
};

} // namespace mc::ecs
