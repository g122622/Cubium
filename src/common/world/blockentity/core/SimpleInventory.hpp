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

#include "common/core/Constants.hpp"
#include "entity/inventory/ContainerListener.hpp"
#include "entity/inventory/IInventory.hpp"
#include <functional>
#include <vector>

namespace mc {
namespace blockentity {

/**
 * @brief 简单背包实现
 *
 * 提供通用的物品存储功能，可用于箱子、漏斗等容器方块实体。
 * 参考: net.minecraft.inventory.Inventory
 *
 * 用法示例:
 * @code
 * class ChestEntity : public ContainerBlockEntity {
 *     SimpleInventory m_inventory{27};  // 27格背包
 * public:
 *     IInventory* getInventory() override { return &m_inventory; }
 * };
 * @endcode
 *
 * 线程安全:
 * - 非线程安全，需要在正确的线程上操作
 * - 方块实体的tick和保存/加载会在不同线程调用
 */
class SimpleInventory : public IInventory {
public:
    /**
     * @brief 构造函数
     * @param size 背包大小（槽位数量）
     */
    explicit SimpleInventory(i32 size);

    /**
     * @brief 构造函数（带变更回调）
     * @param size 背包大小
     * @param onChanged 变更回调函数
     */
    SimpleInventory(i32 size, std::function<void()> onChanged);

    /**
     * @brief 析构函数
     */
    ~SimpleInventory() override = default;

    // ========== 移动语义 ==========

    SimpleInventory(SimpleInventory&& other) noexcept;
    SimpleInventory& operator=(SimpleInventory&& other) noexcept;

    // 禁止拷贝（包含 std::function）
    SimpleInventory(const SimpleInventory&) = delete;
    SimpleInventory& operator=(const SimpleInventory&) = delete;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const noexcept override { return static_cast<i32>(m_items.size()); }
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] i32 getMaxStackSize() const noexcept override { return m_maxStackSize; }
    [[nodiscard]] ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;
    void serialize(network::PacketSerializer& ser) const override;

    // ========== 额外功能 ==========

    /**
     * @brief 设置最大堆叠数量
     * @param maxStackSize 最大堆叠数量（默认 DEFAULT_MAX_STACK_SIZE）
     */
    void setMaxStackSize(i32 maxStackSize) noexcept { m_maxStackSize = maxStackSize; }

    /**
     * @brief 设置变更回调
     * @param callback 回调函数
     *
     * @note 此方法为便捷接口，内部将 callback 包装为 ContainerListener 注册。
     *       如果需要更精细的控制，请直接使用 addListener()/removeListener()。
     */
    void setOnChanged(std::function<void()> callback) { m_onChanged = std::move(callback); }

    /**
     * @brief 添加容器变更监听器
     *
     * 监听器在容器内容变更时通过 containerChanged() 被通知。
     * 参考: net.minecraft.world.SimpleContainer.addListener()
     *
     * @param listener 监听器指针（调用方负责确保指针在移除前有效）
     */
    void addListener(ContainerListener* listener) override;

    /**
     * @brief 移除容器变更监听器
     *
     * 参考: net.minecraft.world.SimpleContainer.removeListener()
     *
     * @param listener 要移除的监听器指针
     */
    void removeListener(ContainerListener* listener) override;

    /**
     * @brief 添加物品到第一个可用槽位
     * @param stack 要添加的物品
     * @return 剩余未添加的物品（如果背包满了）
     */
    ItemStack addItem(const ItemStack& stack) override;

    /**
     * @brief 检查是否可以添加物品
     * @param stack 要检查的物品
     * @return 如果可以完全添加返回true
     */
    [[nodiscard]] bool canAddItem(const ItemStack& stack) const override;

    /**
     * @brief 从指定槽位提取物品
     * @param slot 槽位索引
     * @return 槽位中的物品（槽位被清空）
     */
    ItemStack extractItem(i32 slot);

    /**
     * @brief 检查槽位是否为空
     * @param slot 槽位索引
     * @return 如果为空返回true
     */
    [[nodiscard]] bool isSlotEmpty(i32 slot) const noexcept;

    /**
     * @brief 获取非空槽位数量
     * @return 非空槽位数量
     */
    [[nodiscard]] i32 getNonEmptySlotCount() const noexcept;

    /**
     * @brief 遍历所有物品
     * @param consumer 消费函数，返回false停止遍历
     */
    void forEachItem(std::function<bool(i32 slot, const ItemStack& stack)> consumer) const;

    // ========== 序列化 ==========

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     */
    void load(const nlohmann::json& data);

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const;

private:
    /**
     * @brief 验证槽位索引
     * @param slot 槽位索引
     * @return 如果有效返回true
     */
    [[nodiscard]] bool _isValidSlot(i32 slot) const noexcept
    {
        return slot >= 0 && slot < static_cast<i32>(m_items.size());
    }

    /**
     * @brief 触发变更回调
     */
    void _onChanged();

    std::vector<ItemStack> m_items;
    i32 m_maxStackSize = mc::item::DEFAULT_MAX_STACK_SIZE;
    std::function<void()> m_onChanged;
    std::vector<ContainerListener*> m_listeners; ///< 容器变更监听器列表
};

} // namespace blockentity
} // namespace mc
