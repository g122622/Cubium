#pragma once

#include "../../core/Types.hpp"
#include "../../item/core/ItemStack.hpp"
#include <functional>

namespace mc {

// Forward declarations
class Player;

/**
 * @brief 背包接口
 *
 * 所有背包容器的基础接口，定义了背包的基本操作。
 * 参考: net.minecraft.inventory.IInventory
 *
 * 用法示例:
 * @code
 * class ChestInventory : public IInventory {
 * public:
 *     i32 getContainerSize() const override { return 27; }
 *     ItemStack getItem(i32 slot) const override { return m_items[slot]; }
 *     // ... 其他方法
 * private:
 *     std::array<ItemStack, 27> m_items;
 * };
 * @endcode
 */
class IInventory {
public:
    virtual ~IInventory() = default;

    // ========== 容量查询 ==========

    /**
     * @brief 获取容器大小（槽位数量）
     */
    [[nodiscard]] virtual i32 getContainerSize() const = 0;

    /**
     * @brief 检查容器是否为空
     */
    [[nodiscard]] virtual bool isEmpty() const = 0;

    /**
     * @brief 获取最大堆叠数量
     * @return 默认64
     */
    [[nodiscard]] virtual i32 getMaxStackSize() const { return 64; }

    // ========== 物品操作 ==========

    /**
     * @brief 获取指定槽位的物品
     * @param slot 槽位索引
     * @return 物品堆，空槽位返回 ItemStack::EMPTY
     */
    [[nodiscard]] virtual ItemStack getItem(i32 slot) const = 0;

    /**
     * @brief 设置指定槽位的物品
     * @param slot 槽位索引
     * @param stack 物品堆
     */
    virtual void setItem(i32 slot, const ItemStack& stack) = 0;

    /**
     * @brief 从槽位移除指定数量的物品
     * @param slot 槽位索引
     * @param count 要移除的数量
     * @return 被移除的物品堆
     */
    virtual ItemStack removeItem(i32 slot, i32 count) = 0;

    /**
     * @brief 移除整个槽位的物品
     * @param slot 槽位索引
     * @return 被移除的物品堆
     */
    virtual ItemStack removeItemNoUpdate(i32 slot) = 0;

    // ========== 容器操作 ==========

    /**
     * @brief 清空容器
     */
    virtual void clear() = 0;

    /**
     * @brief 标记容器已更改
     */
    virtual void setChanged() {}

    // ========== 物品查找 ==========

    /**
     * @brief 查找第一个空槽位
     * @return 空槽位索引，没有空槽位返回-1
     */
    [[nodiscard]] virtual i32 getFirstEmptySlot() const {
        for (i32 i = 0; i < getContainerSize(); ++i) {
            if (getItem(i).isEmpty()) {
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief 统计指定物品的总数量
     * @param item 要统计的物品
     * @return 总数量
     */
    [[nodiscard]] virtual i32 countItem(const Item& item) const {
        i32 total = 0;
        for (i32 i = 0; i < getContainerSize(); ++i) {
            const ItemStack& stack = getItem(i);
            if (stack.getItem() == &item) {
                total += stack.getCount();
            }
        }
        return total;
    }

    /**
     * @brief 检查是否包含指定物品
     * @param item 要检查的物品
     * @return 是否包含
     */
    [[nodiscard]] virtual bool hasItem(const Item& item) const {
        for (i32 i = 0; i < getContainerSize(); ++i) {
            if (getItem(i).getItem() == &item) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 查找指定物品的槽位
     * @param item 要查找的物品
     * @return 槽位索引，未找到返回-1
     */
    [[nodiscard]] virtual i32 findSlot(const Item& item) const {
        for (i32 i = 0; i < getContainerSize(); ++i) {
            if (getItem(i).getItem() == &item) {
                return i;
            }
        }
        return -1;
    }

    /**
     * @brief 检查槽位是否可以放置物品
     * @param slot 槽位索引
     * @param stack 要放置的物品
     * @return 是否可以放置
     */
    [[nodiscard]] virtual bool canPlaceItem(i32 slot, const ItemStack& stack) const {
        (void)slot;
        (void)stack;
        return true;
    }

    // ========== 序列化 ==========

    /**
     * @brief 序列化背包数据
     */
    virtual void serialize(network::PacketSerializer& ser) const = 0;

    /**
     * @brief 反序列化背包数据
     */
    [[nodiscard]] static Result<std::unique_ptr<IInventory>> deserialize(network::PacketDeserializer& deser);
};

} // namespace mc
