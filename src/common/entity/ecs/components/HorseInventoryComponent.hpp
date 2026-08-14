#pragma once

#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类库存组件（A 类本地字段，unique_ptr 包裹）
 *
 * 承载 AbstractHorseEntity 的装备栏（鞍槽 + 马铠槽，装箱子后扩展）与马铠装备状态。
 * 仅 AbstractHorseEntity 子树 attach。
 *
 * SimpleInventory 禁止拷贝（含 std::function），但 entt 组件池仅要求可移动。
 * unique_ptr 本身可移动 noexcept，隐式合成的移动构造/赋值满足 entt 要求
 * （沿用 ChestMinecartComponent/AttributeComponent 用 unique_ptr 范式）。
 *
 * m_inventory 初始为空，由 AbstractHorseEntity::initHorseChest() 在构造期与
 * readAdditionalSaveData 末尾按 getInventorySize() 重建赋值（装箱子后规模变化）。
 *
 * m_hasArmor 为马铠槽（槽位 1）是否有物的运行时镜像，setEquipment(1,...) 时同步。
 * 不同步不存盘（vanilla 马铠状态走 ItemStack 槽位，此处为简化标志）。
 *
 * 持久化：库存内容走 LootableContainer 体系（容器 NBT 由 ContainerEntity 层处理），
 * 项目当前未接通 vehicle/马匹容器持久化（TODO）。本组件不注册序列化器。
 */
struct HorseInventoryComponent {
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;
    bool m_hasArmor{false}; ///< 是否装备了马铠（槽位 1 镜像）
};

} // namespace mc::ecs
