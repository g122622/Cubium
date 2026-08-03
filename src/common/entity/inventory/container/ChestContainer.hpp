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

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "entity/inventory/AbstractContainerMenu.hpp"
#include "entity/inventory/IInventory.hpp"
#include <memory>

namespace mc {

class PlayerInventory;

namespace blockentity {

class ChestEntity;

/**
 * @brief 箱子容器
 *
 * 管理箱子GUI的槽位布局，支持单箱（27格）和双箱（54格）。
 *
 * 槽位布局：
 * - 单箱：27格，3行9列
 * - 双箱：54格，6行9列
 * - 玩家背包：27格主背包 + 9格快捷栏
 */
class ChestContainer : public AbstractContainerMenu {
public:
    /// 单箱行数
    static constexpr i32 SINGLE_CHEST_ROWS = 3;
    /// 双箱行数
    static constexpr i32 DOUBLE_CHEST_ROWS = 6;
    /// 每行槽位数
    static constexpr i32 SLOTS_PER_ROW = 9;

    // ========== 构造函数 ==========

    /**
     * @brief 构造箱子容器（单箱，无距离检查）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包容器（可以是单箱或双箱）
     * @param rows 箱子行数（3或6）
     * @deprecated 使用带 ChestEntity 参数的构造函数以支持距离检查
     */
    ChestContainer(ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, i32 rows);

    /**
     * @brief 构造单箱容器（带距离检查）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包
     * @param chestEntity 箱子实体（用于距离检查）
     */
    ChestContainer(
        ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, ChestEntity* chestEntity);

    /**
     * @brief 构造双箱容器（带距离检查）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 双箱背包
     * @param chestEntityA 第一个箱子实体（用于距离检查）
     * @param chestEntityB 第二个箱子实体（用于距离检查）
     */
    ChestContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* chestInventory,
        ChestEntity* chestEntityA,
        ChestEntity* chestEntityB);

    /**
     * @brief 构造拥有箱子背包的容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventoryOwner 箱子背包所有权
     * @param rows 箱子行数（3或6）
     */
    ChestContainer(
        ContainerId id, PlayerInventory* playerInventory, std::shared_ptr<IInventory> chestInventoryOwner, i32 rows);

    /**
     * @brief 创建单箱容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createSingle(
        ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory);

    /**
     * @brief 创建单箱容器（带距离检查）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 箱子背包
     * @param chestEntity 箱子实体
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createSingle(
        ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory, ChestEntity* chestEntity);

    /**
     * @brief 创建双箱容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 双箱背包
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createDouble(
        ContainerId id, PlayerInventory* playerInventory, IInventory* chestInventory);

    /**
     * @brief 创建双箱容器（带距离检查）
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param chestInventory 双箱背包
     * @param chestEntityA 第一个箱子实体
     * @param chestEntityB 第二个箱子实体
     * @return 容器实例
     */
    static std::unique_ptr<ChestContainer> createDouble(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* chestInventory,
        ChestEntity* chestEntityA,
        ChestEntity* chestEntityB);

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
    [[nodiscard]] i32 getChestSlotCount() const { return m_chestSlotCount; }

    [[nodiscard]] bool stillValid(const Player& player) const override;

    // ========== 容器关闭 ==========

    /**
     * @brief 容器关闭时调用
     *
     * 通知背包容器玩家已关闭，用于关闭动画、音效和打开计数递减。
     * 参考 MC Java: ChestMenu.removed() 调用 container.stopOpen(player)
     */
    void removed(Player& player) override;

    // ========== 快速移动 ==========

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void _initSlots(PlayerInventory* playerInventory);

    IInventory* m_chestInventory;                      ///< 箱子背包
    std::shared_ptr<IInventory> m_chestInventoryOwner; ///< 箱子背包所有权（可选）
    i32 m_rows;                                        ///< 箱子行数
    i32 m_chestSlotCount;                              ///< 箱子槽位数量
    ChestEntity* m_chestEntityA = nullptr;             ///< 第一个箱子实体（可选，用于距离检查）
    ChestEntity* m_chestEntityB = nullptr;             ///< 第二个箱子实体（可选，双箱时使用）
};

} // namespace blockentity
} // namespace mc
