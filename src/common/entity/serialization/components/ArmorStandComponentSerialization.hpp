#pragma once

#include "common/entity/serialization/components/ComponentSerializerRegistry.hpp"

namespace mc::entity::serialization::components {

/**
 * @brief 盔甲架组件序列化器
 *
 * 把 ArmorStandEntity 的特有持久化字段搬迁为 ECS 组件承载后，按组件注册 NBT 读写序列化器：
 *
 * ArmorStandStateComponent 承载 Invisible / Small / ShowArms / DisabledSlots / NoBasePlate /
 * Pose 六个 NBT 键（ArmorStandStateComponent.hpp 定义字段语义）。装备（equipment 复合标签）由
 * LivingEntityComponentSerialization 的 EquipmentComponent 序列化器统一处理，盔甲架构造期挂载
 * 同一组件即可参与读写。序列化器内部 dynamic_cast<ArmorStandEntity*> 早退。
 */
void registerArmorStandComponentSerializers(ComponentSerializerRegistry& registry);

} // namespace mc::entity::serialization::components