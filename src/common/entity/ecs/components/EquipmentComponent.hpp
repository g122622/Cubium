#pragma once

#include "common/entity/core/EquipmentSlot.hpp"
#include "common/item/core/ItemStack.hpp"
#include <array>
#include <cstddef>

namespace mc::ecs {

/**
 * @brief 装备状态组件
 *
 * 承载 LivingEntity::m_equipment / m_lastEquipment 两字段。
 * 对齐基岩版 EquipmentComponent / MobArmorComponent 族。
 *
 * 仅 LivingEntity（含其子类 MobEntity/Player）attach，普通 Entity 不持有此组件。
 * 装备无独立 DataParameter（装备同步走实体追踪器路径，不经 EntityDataManager），
 * setEquipment 单写组件，无镜像，最简单的组件化场景。
 *
 * 字段语义：
 * - m_equipment：当前装备数组（8 槽：MainHand/OffHand/Feet/Legs/Chest/Head/Body/Saddle）。
 * - m_lastEquipment：上一tick装备快照，detectEquipmentUpdates 检测变化用，纯服务端。
 *   默认初始化为全空（同 vanilla lastEquipmentItems 声明期 = ItemStack.EMPTY），首帧
 *   detectEquipmentUpdates 即用全空快照对比当前装备，正确应用 spawn 时已装备物品的修饰符。
 *
 * 注意：Player 子类重写 getMutableEquipment/setEquipment 委托到 PlayerInventory，
 * 不读写本组件；本组件仅承载非 Player 的 LivingEntity 装备。
 */
struct EquipmentComponent {
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_equipment{};
    std::array<ItemStack, static_cast<size_t>(EquipmentSlot::Count)> m_lastEquipment{};
};

} // namespace mc::ecs
