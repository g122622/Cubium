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
class IInventory;

namespace blockentity {
class HopperEntity;
}

/**
 * @brief 漏斗容器
 *
 * 管理漏斗GUI的槽位布局。
 *
 * 槽位布局：
 * - 漏斗：5格，1行5列
 * - 玩家背包：27格主背包 + 9格快捷栏
 */
class HopperContainer : public AbstractContainerMenu {
public:
    /// 漏斗槽位数量
    static constexpr i32 HOPPER_SIZE = 5;

    /// 漏斗槽位起始X位置
    static constexpr i32 HOPPER_SLOT_START_X = 44;
    /// 漏斗槽位Y位置
    static constexpr i32 HOPPER_SLOT_Y = 20;

    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 51;

    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 109;

    /// 槽位宽度
    static constexpr i32 SLOT_SIZE = 18;

    // ========== 构造函数 ==========

    /**
     * @brief 构造漏斗容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param hopperInventory 漏斗背包
     * @param hopperEntity 漏斗实体（可选，用于距离检查）
     */
    HopperContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* hopperInventory,
        blockentity::HopperEntity* hopperEntity = nullptr);

    /**
     * @brief 析构函数
     */
    ~HopperContainer() override = default;

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问漏斗
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     */
    void slotsChanged(IInventory* inventory) override;

    /**
     * @brief 关闭容器时调用，配对构造函数中的openInventory
     */
    void removed(Player& player) override;

    // ========== 属性访问 ==========

    /**
     * @brief 获取漏斗背包
     */
    [[nodiscard]] IInventory* getHopperInventory() const { return m_hopperInventory; }

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     * @param playerInventory 玩家背包
     */
    void _initSlots(PlayerInventory* playerInventory);

    IInventory* m_hopperInventory;             ///< 漏斗背包
    blockentity::HopperEntity* m_hopperEntity; ///< 漏斗实体（可选，用于距离检查）
};

} // namespace mc
