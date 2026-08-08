#pragma once

#include "common/core/Types.hpp"

namespace mc {

/**
 * @brief 装备槽位
 *
 * 定义实体可穿戴的装备槽位。
 * Head/Chest/Legs/Feet 为玩家护甲槽位（与 ArmorSlot 一一对应）。
 * Body 为非玩家实体护甲槽位（狼铠、鹦鹉螺铠甲、马铠等动物护甲）。
 * Saddle 为鞍槽，对应 MC 1.21.11 EquipmentSlot.SADDLE；铜傀儡的天线槽
 * (CopperGolemEntity::EQUIPMENT_SLOT_ANTENNA) 复用此槽位，存放铁傀儡
 * 赠予的罂粟花（ItemTags.SHEARABLE_FROM_COPPER_GOLEM），可被剪刀剪下。
 * 参考: net.minecraft.world.entity.EquipmentSlot
 *
 * 独立成头：原定义在 LivingEntity.hpp，但 EquipmentSlot 是被 95+ 文件引用的基础
 * 枚举，且 ECS 组件（EquipmentComponent）需依赖它，独立头避免循环依赖。
 */
enum class EquipmentSlot : u8 {
    MainHand = 0, // 主手
    OffHand = 1,  // 副手
    Feet = 2,     // 靴子
    Legs = 3,     // 护腿
    Chest = 4,    // 胸甲
    Head = 5,     // 头盔
    Body = 6,     // 身体护甲（非玩家实体专用，如狼铠、鹦鹉螺铠甲、马铠）
    Saddle = 7,   // 鞍槽（铜傀儡天线槽，复用于存放罂粟花等可剪切物品）
    Count = 8     // 槽位数量
};

} // namespace mc
