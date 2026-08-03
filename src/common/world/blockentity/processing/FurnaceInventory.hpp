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
#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerListener.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include <array>
#include <functional>
#include <utility>
#include <vector>

namespace mc {
namespace blockentity {

/**
 * @brief 熔炉背包
 *
 * 专门为熔炉设计的3槽背包：
 * - 槽位0：输入槽（待熔炼物品）
 * - 槽位1：燃料槽（煤炭、木炭等）
 * - 槽位2：输出槽（熔炼产物）
 *
 * 参考: net.minecraft.inventory.FurnaceInventory
 */
class FurnaceInventory : public IInventory {
public:
    /// 槽位索引常量
    static constexpr i32 SLOT_INPUT = 0;  ///< 输入槽
    static constexpr i32 SLOT_FUEL = 1;   ///< 燃料槽
    static constexpr i32 SLOT_OUTPUT = 2; ///< 输出槽
    static constexpr i32 SLOT_COUNT = 3;  ///< 槽位数量

    /**
     * @brief 构造函数
     */
    FurnaceInventory();

    /**
     * @brief 构造函数（带变更回调）
     * @param onChanged 变更回调函数
     */
    explicit FurnaceInventory(std::function<void()> onChanged);

    /**
     * @brief 析构函数
     */
    ~FurnaceInventory() override = default;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const override { return SLOT_COUNT; }
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] i32 getMaxStackSize() const override { return mc::item::DEFAULT_MAX_STACK_SIZE; }
    [[nodiscard]] ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;

    // ========== 便捷方法 ==========

    /**
     * @brief 获取输入槽物品
     */
    [[nodiscard]] ItemStack getInputItem() const { return getItem(SLOT_INPUT); }

    /**
     * @brief 获取燃料槽物品
     */
    [[nodiscard]] ItemStack getFuelItem() const { return getItem(SLOT_FUEL); }

    /**
     * @brief 获取输出槽物品
     */
    [[nodiscard]] ItemStack getOutputItem() const { return getItem(SLOT_OUTPUT); }

    /**
     * @brief 设置输入槽物品
     */
    void setInputItem(const ItemStack& stack) { setItem(SLOT_INPUT, stack); }

    /**
     * @brief 设置燃料槽物品
     */
    void setFuelItem(const ItemStack& stack) { setItem(SLOT_FUEL, stack); }

    /**
     * @brief 设置输出槽物品
     */
    void setOutputItem(const ItemStack& stack) { setItem(SLOT_OUTPUT, stack); }

    /**
     * @brief 检查输入槽是否为空
     */
    [[nodiscard]] bool isInputEmpty() const { return m_items[SLOT_INPUT].isEmpty(); }

    /**
     * @brief 检查燃料槽是否为空
     */
    [[nodiscard]] bool isFuelEmpty() const { return m_items[SLOT_FUEL].isEmpty(); }

    /**
     * @brief 检查输出槽是否为空
     */
    [[nodiscard]] bool isOutputEmpty() const { return m_items[SLOT_OUTPUT].isEmpty(); }

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
     * @brief 消耗输入槽物品
     * @param count 消耗数量
     * @return 被消耗的物品
     */
    ItemStack consumeInput(i32 count) { return removeItem(SLOT_INPUT, count); }

    /**
     * @brief 消耗燃料槽物品
     * @param count 消耗数量（通常为1）
     * @return 被消耗的物品
     */
    ItemStack consumeFuel(i32 count = 1) { return removeItem(SLOT_FUEL, count); }

    /**
     * @brief 向输出槽添加物品
     * @param stack 要添加的物品
     * @return 剩余未添加的物品
     */
    ItemStack addToOutput(const ItemStack& stack);

    /**
     * @brief 检查输出槽是否可以接受物品
     * @param stack 要检查的物品
     * @return 如果可以接受返回true
     */
    [[nodiscard]] bool canAcceptOutput(const ItemStack& stack) const;

private:
    /**
     * @brief 验证槽位索引
     */
    [[nodiscard]] bool _isValidSlot(i32 slot) const { return slot >= 0 && slot < SLOT_COUNT; }

    /**
     * @brief 触发变更回调
     */
    void _onChanged();

    std::array<ItemStack, SLOT_COUNT> m_items;
    std::function<void()> m_onChanged;
    std::vector<ContainerListener*> m_listeners; ///< 容器变更监听器列表
};

} // namespace blockentity
} // namespace mc
