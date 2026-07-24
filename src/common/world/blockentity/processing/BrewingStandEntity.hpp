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

#include "entity/inventory/ISidedInventory.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 酿造台方块实体
 *
 * 酿造台用于酿造药水，拥有：
 * - 3个药水瓶槽位（槽位 0-2）
 * - 1个材料槽位（槽位 3）
 * - 1个燃料槽位（槽位 4，烈焰粉）
 *
 * 实现 ISidedInventory 接口：
 * - 上方：材料槽（槽位 3）
 * - 下方：药水瓶槽 + 材料槽（槽位 0, 1, 2, 3）
 * - 侧面：药水瓶槽 + 燃料槽（槽位 0, 1, 2, 4）
 *
 * 参考: net.minecraft.tileentity.BrewingStandTileEntity
 */
class BrewingStandEntity : public ContainerBlockEntity, public ISidedInventory {
public:
    /// 药水瓶槽位数量
    static constexpr i32 BOTTLE_SLOTS = 3;
    /// 材料槽位索引
    static constexpr i32 INGREDIENT_SLOT = 3;
    /// 燃料槽位索引
    static constexpr i32 FUEL_SLOT = 4;
    /// 总槽位数量
    static constexpr i32 TOTAL_SLOTS = 5;
    /// 烈焰粉燃烧时间（每次酿造消耗）
    static constexpr i32 FUEL_PER_BREW = 20;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BrewingStandEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BrewingStandEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return TOTAL_SLOTS; }

    // ========== IInventory 委托方法 ==========

    [[nodiscard]] bool isEmpty() const override { return m_inventory.isEmpty(); }
    [[nodiscard]] i32 getMaxStackSize() const override { return m_inventory.getMaxStackSize(); }
    [[nodiscard]] ItemStack getItem(i32 slot) const override { return m_inventory.getItem(slot); }
    void setItem(i32 slot, const ItemStack& stack) override { m_inventory.setItem(slot, stack); }
    ItemStack removeItem(i32 slot, i32 count) override { return m_inventory.removeItem(slot, count); }
    ItemStack removeItemNoUpdate(i32 slot) override { return m_inventory.removeItemNoUpdate(slot); }
    void clear() override { m_inventory.clear(); }
    void setChanged() override { ContainerBlockEntity::setChanged(); }
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;

    // ========== ISidedInventory 接口实现 ==========

    /**
     * @brief 获取指定面可以访问的槽位
     *
     * 酿造台槽位访问规则：
     * - 上方 (Direction::Up)：材料槽（槽位 3）
     * - 下方 (Direction::Down)：药水瓶槽 + 材料槽（槽位 0, 1, 2, 3）
     * - 侧面：药水瓶槽 + 燃料槽（槽位 0, 1, 2, 4）
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
     * 特殊规则：材料槽（槽位 3）只能提取玻璃瓶。
     *
     * @param slot 槽位索引
     * @param stack 要提取的物品
     * @param direction 提取方向
     * @return 如果可以提取返回 true
     */
    [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;

    /**
     * @brief 检查槽位是否可以从指定方向访问
     * @param slot 槽位索引
     * @param direction 方向
     * @return 如果槽位可访问返回 true
     */
    [[nodiscard]] bool isSlotAccessibleForDirection(i32 slot, Direction direction) const;

    // ========== 酿造逻辑 ==========

    /**
     * @brief 获取燃料等级
     * @return 燃料等级 (0-20)
     */
    [[nodiscard]] i32 getFuelLevel() const { return m_fuel; }

    /**
     * @brief 设置燃料等级
     * @param fuel 燃料等级
     */
    void setFuelLevel(i32 fuel);

    /**
     * @brief 获取酿造时间
     * @return 酿造时间 (0-400)
     */
    [[nodiscard]] i32 getBrewTime() const { return m_brewTime; }

    /**
     * @brief 检查是否正在酿造
     * @return 如果正在酿造返回true
     */
    [[nodiscard]] bool isBrewing() const { return m_brewTime > 0; }

    /**
     * @brief 检查是否有燃料
     * @return 如果有燃料返回true
     */
    [[nodiscard]] bool hasFuel() const { return m_fuel > 0; }

    /**
     * @brief 检查槽位是否有瓶子
     * @param slot 槽位索引 (0-2)
     * @return 如果有瓶子返回true
     */
    [[nodiscard]] bool hasBottle(i32 slot) const;

    /**
     * @brief 获取红石比较器信号强度
     *
     * 计算方式：
     * 1. 计算每个槽位的填充率 = 物品数量 / min(容器堆叠上限, 物品最大堆叠数)
     * 2. 平均填充率 = 所有槽位填充率之和 / 总槽位数
     * 3. 信号强度 = floor(平均填充率 * 14) + (有非空槽位 ? 1 : 0)
     *
     * @return 红石信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== 自定义名称 ==========

    /**
     * @brief 获取自定义名称
     * @return 自定义名称（空字符串表示无自定义名称）
     */
    [[nodiscard]] std::string getCustomName() const override { return m_customName; }

    /**
     * @brief 设置自定义名称
     * @param name 自定义名称（空字符串清除自定义名称）
     */
    void setCustomName(const std::string& name) override
    {
        if (m_customName != name) {
            m_customName = name;
            setChanged();
        }
    }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 检查是否可以酿造
     * @return 如果可以酿造返回true
     */
    [[nodiscard]] bool _canBrew() const;

    /**
     * @brief 执行酿造
     * @param world 世界引用
     */
    void _doBrew(IWorld& world);

    /**
     * @brief 消耗燃料
     */
    void _consumeFuel();

    /**
     * @brief 更新方块状态
     * @param world 世界引用
     */
    void _updateBlockState(IWorld& world);

    SimpleInventory m_inventory; ///< 物品存储
    i32 m_brewTime = 0;          ///< 酿造时间 (0-400)
    i32 m_fuel = 0;              ///< 燃料等级 (0-20)
    bool m_lastBrewing = false;  ///< 上一帧是否在酿造
    ItemStack m_ingredientCache; ///< 材料缓存（用于检测材料变化）
    std::string m_customName;    ///< 自定义名称（铁砧重命名后由放置物品传递）
};

} // namespace blockentity
} // namespace mc
