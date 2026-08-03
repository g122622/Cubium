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
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "entity/inventory/CraftingInventory.hpp"
#include "item/crafting/RecipeManager.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <array>
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;

/**
 * @brief 自动合成器方块实体
 *
 * 存储合成器9格物品和槽位锁定状态，负责合成tick倒计时。
 * 当红石信号触发合成时，CrafterBlock 调度4 tick延时后执行合成逻辑，
 * 合成成功后 CRAFTING 属性置 true，本实体维护6 tick倒计时动画。
 *
 * 参考: net.minecraft.world.level.block.entity.CrafterBlockEntity
 */
class CrafterBlockEntity : public ContainerBlockEntity {
public:
    static constexpr i32 CONTAINER_SIZE = 9;     ///< 3x3 合成网格
    static constexpr i32 SLOT_DISABLED = 1;      ///< 槽位禁用状态值
    static constexpr i32 SLOT_ENABLED = 0;       ///< 槽位启用状态值
    static constexpr i32 MAX_CRAFTING_TICKS = 6; ///< 合成动画持续tick数

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit CrafterBlockEntity(const BlockPos& pos);

    // ========== BlockEntity 接口 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;
    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override;

    // ========== 容器接口 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return CONTAINER_SIZE; }
    void clearContainer() override;

    /**
     * @brief 检查物品是否可以放入指定槽位
     *
     * 禁用的槽位不允许放入物品。
     */
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const;

    // ========== 合成器特有方法 ==========

    /**
     * @brief 设置槽位启用/禁用状态
     * @param slot 槽位索引 (0-8)
     * @param enabled true为启用，false为禁用
     */
    void setSlotState(i32 slot, bool enabled);

    /**
     * @brief 检查槽位是否被禁用
     * @param slot 槽位索引 (0-8)
     * @return 如果槽位被禁用返回true
     */
    [[nodiscard]] bool isSlotDisabled(i32 slot) const;

    /**
     * @brief 设置合成器触发状态
     * @param triggered 是否被红石触发
     */
    void setTriggered(bool triggered);

    /**
     * @brief 检查合成器是否被红石触发
     */
    [[nodiscard]] bool isTriggered() const { return m_triggered; }

    /**
     * @brief 设置合成动画剩余tick数
     * @param ticks 剩余tick数
     */
    void setCraftingTicksRemaining(i32 ticks) { m_craftingTicksRemaining = ticks; }

    /**
     * @brief 获取合成动画剩余tick数
     */
    [[nodiscard]] i32 getCraftingTicksRemaining() const { return m_craftingTicksRemaining; }

    /**
     * @brief 构建合成输入
     *
     * 从9格物品构建 CraftingInput，禁用槽位视为空。
     */
    [[nodiscard]] CraftingInventory asCraftInput() const;

    /**
     * @brief 获取红石比较器信号强度
     *
     * 每个非空或禁用的槽位贡献1点信号强度。
     */
    [[nodiscard]] i32 getRedstoneSignal() const;

    /**
     * @brief 设置自定义名称
     */
    void setCustomName(const std::string& name) override { m_customName = name; }

    /**
     * @brief 获取自定义名称
     */
    [[nodiscard]] std::string getCustomName() const override { return m_customName; }

private:
    /**
     * @brief 检查槽位是否可以被禁用
     *
     * 只有空槽位才能被禁用。
     */
    [[nodiscard]] bool slotCanBeDisabled(i32 slot) const;

    /**
     * @brief 背包变更回调
     *
     * 当背包内容变化时自动重新启用被放入物品的禁用槽位，
     * 与MC原版CrafterBlockEntity.setItem()行为一致。
     */
    void _onInventoryChanged();

    blockentity::SimpleInventory m_inventory;          ///< 9格物品存储
    std::array<i32, CONTAINER_SIZE> m_slotStates = {}; ///< 槽位状态 (0=启用, 1=禁用)
    i32 m_craftingTicksRemaining = 0;                  ///< 合成动画剩余tick
    bool m_triggered = false;                          ///< 红石触发状态
    std::string m_customName;                          ///< 自定义名称
};

} // namespace mc
