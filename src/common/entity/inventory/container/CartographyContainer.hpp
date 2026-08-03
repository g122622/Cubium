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

namespace mc {

class PlayerInventory;
class IWorld;

/**
 * @brief 制图台容器
 *
 * 管理制图台GUI的槽位布局和地图操作逻辑。
 *
 * 槽位布局：
 * - 槽位0: 地图槽 (左侧上方)
 * - 槽位1: 材料槽 (左侧下方，纸/玻璃板/空地图)
 * - 槽位2: 结果槽 (右侧)
 * - 槽位3-29: 玩家主背包 (3x9)
 * - 槽位30-38: 玩家快捷栏 (1x9)
 *
 * 三种操作：
 * - 纸 + 地图 → 扩展地图（缩放级别+1）
 * - 玻璃板 + 地图 → 锁定地图
 * - 空地图 + 地图 → 复制地图
 */
class CartographyContainer : public AbstractContainerMenu {
public:
    /// 地图槽索引
    static constexpr i32 SLOT_MAP = 0;
    /// 材料槽索引
    static constexpr i32 SLOT_MATERIAL = 1;
    /// 结果槽索引
    static constexpr i32 SLOT_RESULT = 2;
    /// 制图台槽位数量
    static constexpr i32 CARTOGRAPHY_SLOTS = 3;

    /// 槽位位置 (GUI坐标)
    static constexpr i32 MAP_SLOT_X = 15;
    static constexpr i32 MAP_SLOT_Y = 15;
    static constexpr i32 MATERIAL_SLOT_X = 15;
    static constexpr i32 MATERIAL_SLOT_Y = 52;
    static constexpr i32 RESULT_SLOT_X = 145;
    static constexpr i32 RESULT_SLOT_Y = 33;
    static constexpr i32 PLAYER_INV_X = 8;
    static constexpr i32 PLAYER_INV_Y = 84;
    static constexpr i32 HOTBAR_Y = 142;

    // ========== 构造函数 ==========

    /**
     * @brief 构建制图台容器
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param position 方块位置
     * @param world 世界指针
     */
    CartographyContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& position, IWorld* world);

    ~CartographyContainer() override = default;

    // ========== 容器接口 ==========

    [[nodiscard]] bool stillValid(const Player& player) const override;
    void slotsChanged(IInventory* inventory) override;
    void removed(Player& player) override;

    // ========== 制图台操作 ==========

    /**
     * @brief 根据输入槽内容更新结果槽
     */
    void updateResult();

    /**
     * @brief 结果槽索引
     */
    [[nodiscard]] i32 getResultSlotIndex() const override { return SLOT_RESULT; }

protected:
    ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

private:
    /**
     * @brief 初始化槽位布局
     */
    void _initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 检查材料槽中是否为纸
     */
    [[nodiscard]] bool _hasPaper() const;

    /**
     * @brief 检查材料槽中是否为玻璃板
     */
    [[nodiscard]] bool _hasGlassPane() const;

    /**
     * @brief 检查材料槽中是否为空地图
     */
    [[nodiscard]] bool _hasEmptyMap() const;

    /**
     * @brief 检查地图槽中是否有已填充地图
     */
    [[nodiscard]] bool _hasFilledMap() const;

    /**
     * @brief 检查地图是否可以扩展（缩放级别 < 4 且非探险地图）
     */
    [[nodiscard]] bool _canExtendMap() const;

    /**
     * @brief 检查地图是否可以锁定
     */
    [[nodiscard]] bool _canLockMap() const;

    /**
     * @brief 检查地图是否可以复制
     */
    [[nodiscard]] bool _canCopyMap() const;

private:
    std::unique_ptr<IInventory> m_cartographyInventory; ///< 制图台3格背包
    BlockPos m_position;                                ///< 制图台方块位置
    IWorld* m_world;                                    ///< 世界指针
};

} // namespace mc
