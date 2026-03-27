#pragma once

#include "entity/inventory/Container.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;

namespace blockentity {

/**
 * @brief 箱子容器
 *
 * 管理箱子GUI的槽位布局，支持单箱（27格）和双箱（54格）。
 *
 * 槽位布局：
 * - 单箱：27格，3行9列
 * - 双箱：54格，6行9列
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 参考: net.minecraft.inventory.container.ChestContainer
 */
class ChestContainer : public Container {
public:
    /// 单箱行数
    static constexpr i32 SINGLE_CHEST_ROWS = 3;
    /// 双箱行数
    static constexpr i32 DOUBLE_CHEST_ROWS = 6;
    /// 每行槽位数
    static constexpr i32 SLOTS_PER_ROW = 9;

    // ========== 构造函数 ==========

    /**
     * @brief 构造箱子容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包容器（可以是单箱或双箱）
     * @param rows 箱子行数（3或6）
     */
    ChestContainer(ContainerId id,
                   PlayerInventory* playerInventory,
                   IInventory* chestInventory,
                   i32 rows);

    /**
     * @brief 创建单箱容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createSingle(
        ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* chestInventory);

    /**
     * @brief 创建双箱容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 双箱背包
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createDouble(
        ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* chestInventory);

    // ========== 属性访问 ==========

    /**
     * @brief 获取箱子行数
     */
    [[nodiscard]] i32 getRowCount() const { return m_rows; }

    /**
     * @brief 获取箱子背包
     */
    [[nodiscard]] IInventory* getChestInventory() const { return m_chestInventory; }

    /**
     * @brief 获取箱子槽位数量
     */
    [[nodiscard]] i32 getChestSlotCount() const { return m_rows * SLOTS_PER_ROW; }

    // ========== 快速移动 ==========

protected:
    ItemStack doQuickMove(i32 slotIndex, ItemStack cursorItem) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void initSlots(PlayerInventory* playerInventory);

    IInventory* m_chestInventory;  ///< 箱子背包
    i32 m_rows;                    ///< 箱子行数
};

} // namespace blockentity
} // namespace mc
