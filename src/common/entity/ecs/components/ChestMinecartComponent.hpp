#pragma once

#include "common/core/Types.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc::ecs {

/**
 * @brief 箱子矿车组件
 *
 * 承载 ChestMinecartEntity 的 27 格库存。仅 ChestMinecartEntity attach。
 *
 * SimpleInventory 禁止拷贝（含 std::function），但 entt 组件池仅要求可移动。
 * unique_ptr 本身可移动 noexcept，隐式合成的移动构造/赋值满足 entt 要求
 * （沿用 AttributeComponent 用 unique_ptr<AttributeMap> 范式）。SimpleInventory
 * 完整定义经 include 可见，unique_ptr 默认析构能正确 delete。
 *
 * m_inventory 初始为空，由 ChestMinecartEntity 构造函数 attach 后赋值
 * make_unique<SimpleInventory>(27)（库存在构造期定大小，见 ChestMinecartEntity 构造）。
 */
struct ChestMinecartComponent {
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;
};

} // namespace mc::ecs
