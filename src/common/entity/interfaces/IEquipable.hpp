#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace entity {

// Forward declarations
class ItemStack;

/**
 * @brief 可装备接口 - 用于可以被装备物品的实体
 *
 * 实现此接口的实体可以装备鞍、马铠等物品。
 * 例如：猪、马、驴等。
 *
 * 参考 MC 1.16.5 IEquipable
 */
class IEquipable {
public:
    virtual ~IEquipable() = default;

    /**
     * @brief 获取装备槽数量
     * @return 装备槽数量
     */
    virtual usize getEquipmentSlotCount() const = 0;

    /**
     * @brief 获取指定槽位的装备
     * @param slot 槽位索引 (0-based)
     * @return 装备物品，可能为空
     */
    virtual ItemStack* getEquipment(usize slot) = 0;

    /**
     * @brief 设置指定槽位的装备
     * @param slot 槽位索引
     * @param item 物品
     */
    virtual void setEquipment(usize slot, const ItemStack& item) = 0;

    /**
     * @brief 检查是否可以装备指定物品
     * @param item 要检查的物品
     * @param slot 目标槽位
     * @return 如果可以装备返回true
     */
    virtual bool canEquip(const ItemStack& item, usize slot) const = 0;
};

} // namespace entity
} // namespace mc
