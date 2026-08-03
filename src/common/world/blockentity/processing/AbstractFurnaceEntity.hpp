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
#include "common/item/core/Item.hpp"
#include "common/item/crafting/IRecipe.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/inventory/ISidedInventory.hpp"
#include "item/crafting/SmeltingRecipe.hpp"
#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/processing/FurnaceInventory.hpp"
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// Forward declaration
class Player;

namespace blockentity {

/**
 * @brief 熔炉方块实体基类
 *
 * 提供熔炉、高炉、烟熏炉的通用功能：
 * - 燃烧管理（燃烧时间、燃料消耗）
 * - 熔炼进度（熔炼时间、配方匹配）
 * - 红石比较器信号
 * - 锁定功能
 * - ISidedInventory 接口（漏斗交互）
 *
 * 子类:
 * - FurnaceEntity（普通熔炉，200tick熔炼）
 * - BlastFurnaceEntity（高炉，100tick熔炼，仅矿石/金属）
 * - SmokerEntity（烟熏炉，100tick熔炼，仅食物）
 *
 * 槽位访问规则：
 * - 上方：输入槽（槽位 0）
 * - 下方：输出槽（槽位 2）、燃料槽（槽位 1）
 * - 侧面：燃料槽（槽位 1）
 */
class AbstractFurnaceEntity : public LockableBlockEntity, public ISidedInventory {
public:
    // ========== 常量 ==========

    /// 输入槽索引
    static constexpr i32 SLOT_INPUT = 0;
    /// 燃料槽索引
    static constexpr i32 SLOT_FUEL = 1;
    /// 输出槽索引
    static constexpr i32 SLOT_OUTPUT = 2;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    AbstractFurnaceEntity(BlockEntityType type, const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~AbstractFurnaceEntity() noexcept override = default;

    // ========== BlockEntity 接口 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== IInventory 接口（委托给 FurnaceInventory） ==========

    [[nodiscard]] bool isEmpty() const override { return m_inventory.isEmpty(); }
    [[nodiscard]] i32 getMaxStackSize() const override { return m_inventory.getMaxStackSize(); }
    [[nodiscard]] ItemStack getItem(i32 slot) const override { return m_inventory.getItem(slot); }
    void setItem(i32 slot, const ItemStack& stack) override { m_inventory.setItem(slot, stack); }
    ItemStack removeItem(i32 slot, i32 count) override { return m_inventory.removeItem(slot, count); }
    ItemStack removeItemNoUpdate(i32 slot) override { return m_inventory.removeItemNoUpdate(slot); }
    void clear() override { m_inventory.clear(); }
    void setChanged() override { LockableBlockEntity::setChanged(); }
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override
    {
        return m_inventory.canPlaceItem(slot, stack);
    }

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return 3; }

    // ========== ISidedInventory 接口 ==========

    /**
     * @brief 获取指定面可以访问的槽位
     *
     * 熔炉槽位访问规则：
     * - 上方 (Direction::Up)：输入槽（槽位 0）
     * - 下方 (Direction::Down)：输出槽（槽位 2）、燃料槽（槽位 1）
     * - 侧面：燃料槽（槽位 1）
     *
     * @param side 访问方向
     * @return 可访问的槽位索引数组
     */
    [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override;

    /**
     * @brief 检查是否可以从指定方向向指定槽位插入物品
     *
     * @param slot 槽位索引
     * @param stack 要插入的物品
     * @param direction 插入方向
     * @return 如果可以插入返回 true
     */
    [[nodiscard]] bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const override;

    /**
     * @brief 检查是否可以从指定方向从指定槽位提取物品
     *
     * @param slot 槽位索引
     * @param stack 要提取的物品
     * @param direction 提取方向
     * @return 如果可以提取返回 true
     */
    [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;

private:
    /**
     * @brief 检查槽位是否可以从指定方向访问
     * @param slot 槽位索引
     * @param direction 方向
     * @return 如果槽位可访问返回 true
     */
    [[nodiscard]] bool _isSlotAccessibleForDirection(i32 slot, Direction direction) const;

public:
    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    // ========== 熔炉状态 ==========

    /**
     * @brief 获取累积的熔炼经验
     * @return 累积的经验值
     */
    [[nodiscard]] f32 getStoredExperience() const { return m_storedExperience; }

    /**
     * @brief 设置累积的熔炼经验
     * @param xp 经验值
     */
    void setStoredExperience(f32 xp) { m_storedExperience = xp; }

    /**
     * @brief 提取并清除累积的熔炼经验
     * @return 累积的经验值
     */
    f32 extractStoredExperience()
    {
        f32 xp = m_storedExperience;
        m_storedExperience = 0.0f;
        return xp;
    }

    /**
     * @brief 检查是否正在燃烧
     * @return 如果燃烧中返回true
     */
    [[nodiscard]] bool isBurning() const { return m_burnTime > 0; }

    /**
     * @brief 获取当前燃烧时间
     * @return 剩余燃烧时间（tick）
     */
    [[nodiscard]] i32 getBurnTime() const { return m_burnTime; }

    /**
     * @brief 获取当前燃料的总燃烧时间
     * @return 总燃烧时间（tick）
     */
    [[nodiscard]] i32 getBurnTimeTotal() const { return m_burnTimeTotal; }

    /**
     * @brief 获取熔炼进度
     * @return 当前熔炼时间（tick）
     */
    [[nodiscard]] i32 getCookTime() const { return m_cookTime; }

    /**
     * @brief 获取总熔炼时间
     * @return 总熔炼时间（tick）
     */
    [[nodiscard]] i32 getCookTimeTotal() const { return m_cookTimeTotal; }

    /**
     * @brief 获取红石比较器信号
     * @return 信号强度（0-15）
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== 熔炉特定方法 ==========

    /**
     * @brief 检查物品是否为燃料
     * @param stack 物品堆
     * @return 如果是燃料返回true
     */
    [[nodiscard]] static bool isFuel(const ItemStack& stack);

    /**
     * @brief 获取物品的燃烧时间（静态方法，基础值）
     * @param stack 物品堆
     * @return 燃烧时间（tick），如果不是燃料返回0
     */
    [[nodiscard]] static i32 getBurnTime(const ItemStack& stack);

    /**
     * @brief 获取燃料燃烧时间（实例方法，考虑倍率）
     *
     * 子类可重写此方法调整燃烧速度。
     * 高炉和烟熏炉重写此方法返回一半的时间。
     *
     * @param stack 物品堆
     * @return 燃烧时间（tick），如果不是燃料返回0
     */
    [[nodiscard]] virtual i32 getBurnTimeForFuel(const ItemStack& stack) const { return getBurnTime(stack); }

    /**
     * @brief 获取背包
     */
    [[nodiscard]] FurnaceInventory& getFurnaceInventory() { return m_inventory; }
    [[nodiscard]] const FurnaceInventory& getFurnaceInventory() const { return m_inventory; }

protected:
    /**
     * @brief 获取默认显示名称（子类重写）
     */
    [[nodiscard]] std::string getDefaultName() const override = 0;

    /**
     * @brief 获取默认熔炼时间（子类重写）
     * @return 默认熔炼时间（tick）
     *
     * - 普通熔炉：200 tick
     * - 高炉/烟熏炉：100 tick
     */
    [[nodiscard]] virtual i32 getDefaultCookTime() const { return 200; }

    /**
     * @brief 获取当前配方的熔炼时间
     * @param world 世界
     * @return 熔炼时间（tick），如果没有配方返回默认值
     */
    [[nodiscard]] virtual i32 getCookTime(IWorld& world) const;

    /**
     * @brief 检查是否可以熔炼
     * @param world 世界
     * @return 如果可以熔炼返回true
     */
    [[nodiscard]] virtual bool canSmelt(IWorld& world) const;

    /**
     * @brief 执行熔炼
     * @param world 世界
     */
    void smelt(IWorld& world);

    /**
     * @brief 消耗燃料
     * @return 如果成功消耗返回true
     */
    bool burnFuel();

    /**
     * @brief 更新燃烧状态（更新方块状态）
     * @param world 世界
     */
    void updateBurnState(IWorld& world);

    /**
     * @brief 获取火苗噼啪声（子类重写）
     * @return 音效事件
     */
    [[nodiscard]] virtual const ResourceLocation& getFireCrackleSound() const = 0;

    /**
     * @brief 获取熔炼配方类型（子类重写）
     */
    [[nodiscard]] virtual crafting::RecipeType getRecipeType() const = 0;

    /**
     * @brief 获取匹配的熔炼配方
     * @param world 世界
     * @return 配方指针，如果没有返回nullptr
     */
    [[nodiscard]] const crafting::SmeltingRecipe* getRecipe(IWorld& world) const;

    /**
     * @brief 检查是否可以熔炼（使用缓存的配方）
     * @param recipe 配方指针（可为nullptr）
     * @return 如果可以熔炼返回true
     */
    [[nodiscard]] bool canSmeltWithRecipe(const crafting::SmeltingRecipe* recipe) const;

    /**
     * @brief 获取配方的熔炼时间
     * @param recipe 配方指针（可为nullptr）
     * @return 熔炼时间（tick），如果没有配方返回默认值
     */
    [[nodiscard]] i32 getCookTimeFromRecipe(const crafting::SmeltingRecipe* recipe) const;

    /**
     * @brief 执行熔炼（使用缓存的配方）
     * @param recipe 配方指针（可为nullptr）
     */
    void smeltWithRecipe(const crafting::SmeltingRecipe* recipe);

private:
    FurnaceInventory m_inventory;                           ///< 熔炉背包
    i32 m_burnTime = 0;                                     ///< 当前燃烧时间
    i32 m_burnTimeTotal = 0;                                ///< 当前燃料的总燃烧时间
    i32 m_cookTime = 0;                                     ///< 当前熔炼时间
    i32 m_cookTimeTotal = 200;                              ///< 总熔炼时间
    f32 m_storedExperience = 0.0f;                          ///< 累积的熔炼经验（玩家取出物品时发放）
    const crafting::SmeltingRecipe* m_lastRecipe = nullptr; ///< 上次使用的配方
};

} // namespace blockentity
} // namespace mc
