#pragma once

#include "../../../entity/inventory/IInventory.hpp"
#include <vector>
#include <functional>

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

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const override { return static_cast<i32>(m_items.size()); }
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] i32 getMaxStackSize() const override { return m_maxStackSize; }
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
     * @param maxStackSize 最大堆叠数量（默认64）
     */
    void setMaxStackSize(i32 maxStackSize) { m_maxStackSize = maxStackSize; }

    /**
     * @brief 设置变更回调
     * @param callback 回调函数
     */
    void setOnChanged(std::function<void()> callback) { m_onChanged = std::move(callback); }

    /**
     * @brief 添加物品到第一个可用槽位
     * @param stack 要添加的物品
     * @return 剩余未添加的物品（如果背包满了）
     */
    ItemStack addItem(const ItemStack& stack);

    /**
     * @brief 检查是否可以添加物品
     * @param stack 要检查的物品
     * @return 如果可以完全添加返回true
     */
    [[nodiscard]] bool canAddItem(const ItemStack& stack) const;

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
    [[nodiscard]] bool isSlotEmpty(i32 slot) const;

    /**
     * @brief 获取非空槽位数量
     * @return 非空槽位数量
     */
    [[nodiscard]] i32 getNonEmptySlotCount() const;

    /**
     * @brief 遍历所有物品
     * @param consumer 消费函数，返回false停止遍历
     */
    void forEachItem(std::function<bool(i32 slot, const ItemStack& stack)> consumer) const;

private:
    /**
     * @brief 验证槽位索引
     * @param slot 槽位索引
     * @return 如果有效返回true
     */
    [[nodiscard]] bool isValidSlot(i32 slot) const {
        return slot >= 0 && slot < static_cast<i32>(m_items.size());
    }

    /**
     * @brief 触发变更回调
     */
    void onChanged();

    std::vector<ItemStack> m_items;
    i32 m_maxStackSize = 64;
    std::function<void()> m_onChanged;
};

} // namespace blockentity
} // namespace mc
