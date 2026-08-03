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
#include "world/block/BlockPos.hpp"
#include <memory>
#include <string>

namespace mc {

class PlayerInventory;
class BlockEntity;
class IWorld;

/**
 * @brief 铁砧容器
 *
 * 管理铁砧GUI的槽位布局和修复/重命名逻辑。
 *
 * 槽位布局：
 * - 输入槽1：1格（左侧）
 * - 输入槽2：1格（右侧）
 * - 输出槽：1格（底部中央）
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 功能：
 * - 物品修复（使用材料或相同物品）
 * - 附魔书合并
 * - 物品重命名
 * - 物品合并
 */
class AnvilContainer : public AbstractContainerMenu {
public:
    /// 输入槽1索引
    static constexpr i32 SLOT_INPUT_1 = 0;
    /// 输入槽2索引
    static constexpr i32 SLOT_INPUT_2 = 1;
    /// 输出槽索引
    static constexpr i32 SLOT_OUTPUT = 2;
    /// 铁砧槽位总数
    static constexpr i32 ANVIL_SLOTS = 3;

    /// 输入槽位置
    static constexpr i32 INPUT_SLOT_X[] = {27, 76}; // 左右两个输入槽
    static constexpr i32 INPUT_SLOT_Y = 47;
    /// 输出槽位置
    static constexpr i32 OUTPUT_SLOT_X = 134;
    static constexpr i32 OUTPUT_SLOT_Y = 47;
    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 142;

    /// 最大修复成本
    static constexpr i32 MAX_REPAIR_COST = 40;

    // ========== 构造函数 ==========

    /**
     * @brief 构造铁砧容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param position 铁砧位置（用于损坏铁砧）
     * @param world 世界指针
     */
    AnvilContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world);

    /**
     * @brief 析构函数
     */
    ~AnvilContainer() override = default;

    // ========== 修复成本 ==========

    /**
     * @brief 获取修复成本（经验等级）
     * @return 修复成本，如果超过40返回-1表示太贵
     */
    [[nodiscard]] i32 getRepairCost() const { return m_repairCost; }

    /**
     * @brief 获取材料消耗数量
     * @return 材料消耗数量
     */
    [[nodiscard]] i32 getMaterialCost() const { return m_materialCost; }

    /**
     * @brief 检查是否太贵（修复成本 >= 40）
     * @return 如果太贵返回true
     */
    [[nodiscard]] bool isTooExpensive() const { return m_repairCost >= MAX_REPAIR_COST; }

    // ========== 重命名 ==========

    /**
     * @brief 设置重命名名称
     * @param name 新名称（空字符串表示清除）
     */
    void setItemName(const std::string& name);

    /**
     * @brief 获取重命名名称
     * @return 重命名名称
     */
    [[nodiscard]] const std::string& getItemName() const { return m_itemName; }

    /**
     * @brief 检查是否只有重命名操作
     * @return 如果只有重命名返回true
     */
    [[nodiscard]] bool isRenameOnly() const;

    /**
     * @brief 检查玩家是否是创造模式
     * @return 如果关联的玩家是创造模式返回true
     */
    [[nodiscard]] bool isPlayerCreative() const;

    // ========== 槽位访问 ==========

    /**
     * @brief 获取输入槽1的物品
     */
    [[nodiscard]] ItemStack getInputSlot1() const;

    /**
     * @brief 获取输入槽2的物品
     */
    [[nodiscard]] ItemStack getInputSlot2() const;

    /**
     * @brief 获取输出槽的物品
     */
    [[nodiscard]] ItemStack getOutputSlot() const;

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问铁砧
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     */
    void slotsChanged(IInventory* inventory) override;

    /**
     * @brief 关闭容器时调用
     */
    void removed(Player& player) override;

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     */
    void _initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 更新输出结果
     */
    void _updateRepairOutput();

    /**
     * @brief 计算新的修复成本
     * @param oldRepairCost 旧修复成本
     * @return 新修复成本 = oldRepairCost * 2 + 1
     */
    [[nodiscard]] static i32 _getNewRepairCost(i32 oldRepairCost);

    /**
     * @brief 铁砧损坏判定
     *
     * 非创造模式下，从输出槽取出结果时有 12% 概率使铁砧降级：
     * anvil → chipped_anvil → damaged_anvil → 消失。
     * 创造模式玩家不会触发铁砧损坏。
     *
     * @param player 取出物品的玩家，用于判断游戏模式
     */
    void _damageAnvilIfNecessary(Player& player);

private:
    std::unique_ptr<IInventory> m_anvilInventory; ///< 铁砧背包
    BlockPos m_position;                          ///< 铁砧位置
    IWorld* m_world;                              ///< 世界指针
    i32 m_repairCost = 0;                         ///< 修复成本（经验等级）
    i32 m_materialCost = 0;                       ///< 材料消耗数量
    std::string m_itemName;                       ///< 重命名名称
};

} // namespace mc
