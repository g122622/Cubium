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
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
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
#include "entity/inventory/Slot.hpp"
#include "util/color/DyeColor.hpp"
#include "world/block/BlockPos.hpp"
#include "world/blockentity/interactive/BannerPattern.hpp"
#include <memory>

namespace mc {

class Player;
class PlayerInventory;
class IInventory;

namespace entity {
namespace inventory {
namespace container {

/**
 * @brief 织布机容器
 *
 * 织布机的屏幕处理器，管理3个输入槽位和1个输出槽位。
 *
 * 槽位布局：
 * - 槽位0：旗帜槽（只接受BannerItem）
 * - 槽位1：染料槽（只接受DyeItem）
 * - 槽位2：图案物品槽（只接受BannerPatternItem）
 * - 槽位3：输出槽（禁止放入）
 * - 槽位4-30：玩家主背包
 * - 槽位31-39：玩家快捷栏
 *
 * 参考: net.minecraft.inventory.container.LoomContainer
 */
class LoomContainer : public AbstractContainerMenu {
public:
    // ========== 槽位常量 ==========

    static constexpr i32 SLOT_BANNER = 0;  ///< 旗帜槽
    static constexpr i32 SLOT_DYE = 1;     ///< 染料槽
    static constexpr i32 SLOT_PATTERN = 2; ///< 图案物品槽
    static constexpr i32 SLOT_RESULT = 3;  ///< 输出槽
    static constexpr i32 LOOM_SLOTS = 4;   ///< 织布机总槽位数

    // ========== GUI坐标常量 ==========

    static constexpr i32 BANNER_SLOT_X = 31;
    static constexpr i32 BANNER_SLOT_Y = 22;
    static constexpr i32 DYE_SLOT_X = 31;
    static constexpr i32 DYE_SLOT_Y = 53;
    static constexpr i32 PATTERN_SLOT_X = 7;
    static constexpr i32 PATTERN_SLOT_Y = 53;
    static constexpr i32 RESULT_SLOT_X = 145;
    static constexpr i32 RESULT_SLOT_Y = 33;

    // ========== 图案常量 ==========

    /// 基础图案数量（不含Base，不需要图案物品的图案）
    static constexpr i32 PATTERN_COUNT = static_cast<i32>(blockentity::BannerPatternType::Count) - 1;
    /// 需要图案物品的图案数量
    static constexpr i32 PATTERNS_WITH_ITEMS = 6;
    /// 第一个需要图案物品的图案索引-1
    static constexpr i32 PATTERN_ITEM_INDEX = PATTERN_COUNT - PATTERNS_WITH_ITEMS - 1;

    // ========== 构造函数 ==========

    /**
     * @brief 服务端构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param inventory 织布机输入库存
     * @param pos 织布机方块位置
     */
    LoomContainer(
        ContainerId id, PlayerInventory* playerInventory, std::unique_ptr<IInventory> inventory, const BlockPos& pos);

    /**
     * @brief 客户端构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     * @param pos 织布机方块位置
     */
    LoomContainer(ContainerId id, PlayerInventory* playerInventory, const BlockPos& pos);

    ~LoomContainer() override = default;

    // ========== AbstractContainerMenu接口 ==========

    [[nodiscard]] bool stillValid(const Player& player) const override;
    void slotsChanged(IInventory* inventory) override;
    void removed(Player& player) override;

    /**
     * @brief 处理图案选择按钮点击
     *
     * 客户端发送选中的图案ID，服务端验证并更新输出。
     *
     * @param player 玩家
     * @param id 选中的图案ID（1-based索引）
     * @return 如果选择有效返回true
     */
    bool clickMenuButton(Player& player, i32 id);

    /**
     * @brief Shift+点击快速移动
     */
    [[nodiscard]] ItemStack quickMoveStack(i32 slotIndex, Player& player) override;

    // ========== 访问器 ==========

    /**
     * @brief 获取选中的图案索引
     * @return 图案索引（0=无选择）
     */
    [[nodiscard]] i32 getSelectedPattern() const noexcept { return m_selectedPattern; }

    /**
     * @brief 获取输入库存
     */
    [[nodiscard]] IInventory& getInputInventory() { return *m_inputInventory; }

    /**
     * @brief 获取输出库存
     */
    [[nodiscard]] IInventory& getOutputInventory() { return *m_outputInventory; }

private:
    /**
     * @brief 初始化槽位
     */
    void _initSlots(PlayerInventory* playerInventory);

    /**
     * @brief 更新输出槽位
     *
     * 根据输入物品和选中的图案生成输出旗帜。
     */
    void _updateResult();

    /**
     * @brief 检查图案选择是否有效
     * @param patternIndex 图案索引
     * @return 如果有效返回true
     */
    [[nodiscard]] bool _isValidPattern(i32 patternIndex) const;

    /// 输入库存（3格：旗帜、染料、图案物品）
    std::unique_ptr<IInventory> m_inputInventory;
    /// 输出库存（1格）
    std::unique_ptr<IInventory> m_outputInventory;
    /// 选中的图案索引（0=无选择）
    i32 m_selectedPattern = 0;
    /// 织布机方块位置
    BlockPos m_pos;
};

// ========== 自定义槽位类 ==========

/**
 * @brief 织布机旗帜槽
 * 只接受BannerItem
 */
class LoomBannerSlot : public Slot {
public:
    using Slot::Slot;

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;
};

/**
 * @brief 织布机染料槽
 * 只接受DyeItem
 */
class LoomDyeSlot : public Slot {
public:
    using Slot::Slot;

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;
};

/**
 * @brief 织布机图案物品槽
 * 只接受BannerPatternItem
 */
class LoomPatternSlot : public Slot {
public:
    using Slot::Slot;

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;
};

/**
 * @brief 织布机输出槽
 * 禁止放入物品，取出时消耗输入
 */
class LoomResultSlot : public Slot {
public:
    LoomResultSlot(IInventory* inventory, i32 slotIndex, i32 x, i32 y, LoomContainer* container);

    [[nodiscard]] bool mayPlace(const ItemStack& stack) const override;

    /**
     * @brief 取走输出物品时消耗输入
     */
    ItemStack onTake(Player& player, ItemStack stack) override;

private:
    LoomContainer* m_container;
};

} // namespace container
} // namespace inventory
} // namespace entity
} // namespace mc
