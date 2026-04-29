#pragma once

#include "../../../entity/inventory/IInventory.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include <memory>

namespace mc {

// Forward declaration
class IInventory;

namespace blockentity {

class ChestEntity;

/**
 * @brief 双箱合并容器
 *
 * 将两个27格箱子合并成54格容器的包装类。
 * 实现委托模式，将操作转发到底层的两个箱子。
 *
 * 参考: net.minecraft.inventory.DoubleSidedInventory
 *
 * 用法示例:
 * @code
 * if (chestA.isDoubleChest(world)) {
 *     auto doubleInv = chestA.getDoubleInventory(world);
 *     // 使用54格容器
 * }
 * @endcode
 *
 * 注意:
 * - 此类不拥有两个箱子的所有权
 * - 箱子的生命周期由区块管理
 * - 打开/关闭操作会同时影响两个箱子
 */
class DoubleSidedInventory : public IInventory {
public:
    /**
     * @brief 构造函数
     * @param upper 上半部分（左侧箱子）
     * @param lower 下半部分（右侧箱子）
     *
     * 注意：upper和lower的生命周期必须长于此对象
     */
    DoubleSidedInventory(IInventory* upper, IInventory* lower);

    /**
     * @brief 析构函数
     */
    ~DoubleSidedInventory() override = default;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const override;
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] i32 getMaxStackSize() const override;
    [[nodiscard]] ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;
    void serialize(network::PacketSerializer& ser) const override;

    // ========== 双箱特有接口 ==========

    /**
     * @brief 获取上半部分容器
     * @return 上半部分容器指针
     */
    [[nodiscard]] IInventory* getUpper() const { return m_upper; }

    /**
     * @brief 获取下半部分容器
     * @return 下半部分容器指针
     */
    [[nodiscard]] IInventory* getLower() const { return m_lower; }

    /**
     * @brief 检查指定容器是否是此双箱的一部分
     * @param inventory 要检查的容器
     * @return 如果是双箱的一部分返回true
     */
    [[nodiscard]] bool isPartOfLargeChest(const IInventory* inventory) const;

    /**
     * @brief 将全局槽位索引转换为局部槽位索引和容器
     * @param globalSlot 全局槽位索引（0-53）
     * @param outContainer 输出：所属容器
     * @param outLocalSlot 输出：局部槽位索引
     * @return 如果转换成功返回true
     */
    [[nodiscard]] bool getContainerAndSlot(i32 globalSlot,
                                           IInventory** outContainer,
                                           i32& outLocalSlot) const;

    // ========== 物品查找扩展 ==========

    /**
     * @brief 查找第一个空槽位
     * @return 空槽位索引，没有空槽位返回-1
     */
    [[nodiscard]] i32 getFirstEmptySlot() const override;

    /**
     * @brief 统计指定物品的总数量
     * @param item 要统计的物品
     * @return 总数量
     */
    [[nodiscard]] i32 countItem(const Item& item) const override;

    /**
     * @brief 检查是否包含指定物品
     * @param item 要检查的物品
     * @return 是否包含
     */
    [[nodiscard]] bool hasItem(const Item& item) const override;

private:
    IInventory* m_upper;  ///< 上半部分（左侧箱子，27格）
    IInventory* m_lower;  ///< 下半部分（右侧箱子，27格）
};

} // namespace blockentity
} // namespace mc
