/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include <functional>
#include <memory>
#include <unordered_set>

#include "ContainerListener.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ItemStack.hpp"

namespace mc {

// Forward declarations
class Player;
class Item;

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
     * @return 默认 DEFAULT_MAX_STACK_SIZE
     */
    [[nodiscard]] virtual i32 getMaxStackSize() const { return mc::item::DEFAULT_MAX_STACK_SIZE; }

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

    // ========== 变更通知 ==========

    /**
     * @brief 标记容器已更改
     *
     * 子类应重写此方法以通知所有注册的 ContainerListener。
     * 默认实现为空操作。
     */
    virtual void setChanged() {}

    /**
     * @brief 添加容器变更监听器
     *
     * 监听器在容器内容变更（setItem、removeItem、clear 等）时通过
     * containerChanged() 被通知。removeItemNoUpdate 不触发通知。
     *
     * 参考: net.minecraft.world.SimpleContainer.addListener()
     *
     * @param listener 监听器指针（调用方负责确保指针生命周期有效）
     */
    virtual void addListener(ContainerListener* listener) { (void)listener; }

    /**
     * @brief 移除容器变更监听器
     *
     * 参考: net.minecraft.world.SimpleContainer.removeListener()
     *
     * @param listener 要移除的监听器指针
     */
    virtual void removeListener(ContainerListener* listener) { (void)listener; }

    // ========== 玩家访问控制 ==========

    /**
     * @brief 检查玩家是否可以使用此容器
     * @param player 玩家
     * @return 是否可以使用
     *
     * 默认实现返回 true，子类应重写以检查距离。
     * 例如：箱子检查玩家是否在8格范围内。
     */
    [[nodiscard]] virtual bool isUsableByPlayer(const Player& player) const;

    /**
     * @brief 玩家打开容器时调用
     * @param player 玩家
     *
     * 用于实现容器打开计数、音效等功能。
     */
    virtual void openInventory(Player& player);

    /**
     * @brief 玩家关闭容器时调用
     * @param player 玩家
     *
     * 用于实现容器关闭计数、物品返还等功能。
     */
    virtual void closeInventory(Player& player);

    // ========== 物品查找 ==========

    /**
     * @brief 查找第一个空槽位
     * @return 空槽位索引，没有空槽位返回-1
     */
    [[nodiscard]] virtual i32 getFirstEmptySlot() const
    {
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
    [[nodiscard]] virtual i32 countItem(const Item& item) const
    {
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
    [[nodiscard]] virtual bool hasItem(const Item& item) const
    {
        for (i32 i = 0; i < getContainerSize(); ++i) {
            if (getItem(i).getItem() == &item) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 检查是否包含物品集合中的任意物品
     * @param items 物品集合
     * @return 是否包含
     */
    [[nodiscard]] virtual bool hasAny(const std::unordered_set<const Item*>& items) const
    {
        for (i32 i = 0; i < getContainerSize(); ++i) {
            const ItemStack& stack = getItem(i);
            if (!stack.isEmpty() && items.count(stack.getItem()) > 0) {
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
    [[nodiscard]] virtual i32 findSlot(const Item& item) const
    {
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
    [[nodiscard]] virtual bool canPlaceItem(i32 slot, const ItemStack& stack) const
    {
        (void)slot;
        (void)stack;
        return true;
    }

    /**
     * @brief 添加物品到第一个可用槽位
     * @param stack 要添加的物品
     * @return 剩余未添加的物品（如果背包满了）
     *
     * 默认实现：尝试堆叠到已有物品，然后放入空槽位
     */
    virtual ItemStack addItem(const ItemStack& stack);

    /**
     * @brief 检查是否可以添加物品
     * @param stack 要检查的物品
     * @return 如果可以完全添加返回true
     *
     * 默认实现：检查是否有空槽位或可堆叠物品
     */
    [[nodiscard]] virtual bool canAddItem(const ItemStack& stack) const;
};

} // namespace mc
