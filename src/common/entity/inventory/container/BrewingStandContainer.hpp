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
class BrewingStandEntity;
}

/**
 * @brief 酿造台容器
 *
 * 管理酿造台GUI的槽位布局。
 *
 * 槽位布局：
 * - 药水槽：3格（左侧，从上到下）
 * - 材料槽：1格（顶部中央）
 * - 燃料槽：1格（左侧，青色）
 * - 玩家背包：27格主背包 + 9格快捷栏
 *
 * 酿造机制：
 * - 烈焰粉作为燃料（每次酿造消耗1点，每份燃料提供20点）
 * - 酿造时间：400 ticks（20秒）
 */
class BrewingStandContainer : public AbstractContainerMenu {
public:
    /// 药水槽起始索引
    static constexpr i32 SLOT_POTION_START = 0;
    /// 药水槽数量
    static constexpr i32 POTION_SLOTS = 3;
    /// 材料槽索引
    static constexpr i32 SLOT_INGREDIENT = 3;
    /// 燃料槽索引
    static constexpr i32 SLOT_FUEL = 4;
    /// 酿造台槽位总数
    static constexpr i32 BREWING_SLOTS = 5;

    /// 药水槽位置
    static constexpr i32 POTION_SLOT_X[] = {56, 79, 102};
    static constexpr i32 POTION_SLOT_Y[] = {51, 58, 51};
    /// 材料槽位置
    static constexpr i32 INGREDIENT_SLOT_X = 79;
    static constexpr i32 INGREDIENT_SLOT_Y = 17;
    /// 燃料槽位置
    static constexpr i32 FUEL_SLOT_X = 17;
    static constexpr i32 FUEL_SLOT_Y = 17;
    /// 玩家背包起始Y位置
    static constexpr i32 PLAYER_INV_Y = 84;
    /// 快捷栏Y位置
    static constexpr i32 HOTBAR_Y = 142;

    // ========== 构造函数 ==========

    /**
     * @brief 构造酿造台容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param brewingStandInventory 酿造台背包
     * @param brewingStandEntity 酿造台实体（可选，用于同步酿造进度）
     */
    BrewingStandContainer(ContainerId id,
        PlayerInventory* playerInventory,
        IInventory* brewingStandInventory,
        blockentity::BrewingStandEntity* brewingStandEntity = nullptr);

    /**
     * @brief 析构函数
     */
    ~BrewingStandContainer() override = default;

    // ========== 酿造状态 ==========

    /**
     * @brief 获取酿造时间
     * @return 酿造剩余时间（ticks），0表示未在酿造
     */
    [[nodiscard]] i32 getBrewTime() const { return m_brewTime; }

    /**
     * @brief 获取燃料等级
     * @return 燃料等级（0-20）
     */
    [[nodiscard]] i32 getFuelLevel() const { return m_fuel; }

    /**
     * @brief 设置酿造时间（用于客户端同步）
     * @param time 酿造时间
     */
    void setBrewTime(i32 time) { m_brewTime = time; }

    /**
     * @brief 设置燃料等级（用于客户端同步）
     * @param fuel 燃料等级
     */
    void setFuel(i32 fuel) { m_fuel = fuel; }

    // ========== 容器接口 ==========

    /**
     * @brief 检查玩家是否仍可访问酿造台
     */
    [[nodiscard]] bool stillValid(const Player& player) const override;

    /**
     * @brief 容器内容变化时调用
     */
    void slotsChanged(IInventory* inventory) override;

    /**
     * @brief 获取酿造台背包
     */
    [[nodiscard]] IInventory* getBrewingStandInventory() const { return m_brewingStandInventory; }

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     */
    void _initSlots(PlayerInventory* playerInventory);

private:
    IInventory* m_brewingStandInventory;                   ///< 酿造台背包
    blockentity::BrewingStandEntity* m_brewingStandEntity; ///< 酿造台实体
    i32 m_brewTime = 0;                                    ///< 酿造时间
    i32 m_fuel = 0;                                        ///< 燃料等级
};

} // namespace mc
