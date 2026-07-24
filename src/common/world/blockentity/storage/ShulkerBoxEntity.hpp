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
#include "world/blockentity/core/LootableContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 潜影盒方块实体
 *
 * 潜影盒是一种特殊容器，特点：
 * - 27格物品存储
 * - 被破坏时保留物品（不掉落）
 * - 可以被锁定（需要正确名称的物品打开）
 * - 打开时有动画效果
 * - 实现 ISidedInventory（漏斗可以从任意方向访问所有槽位）
 * - 支持战利品表填充（继承自 LootableContainerBlockEntity）
 */
class ShulkerBoxEntity : public LootableContainerBlockEntity, public ISidedInventory {
public:
    /// 潜影盒容量（27格）
    static constexpr i32 SHULKER_BOX_SIZE = 27;

    /// 潜影盒打开状态枚举
    enum class AnimationStatus : u8 {
        Closed = 0,  ///< 关闭状态
        Opening = 1, ///< 打开中
        Opened = 2,  ///< 已打开
        Closing = 3  ///< 关闭中
    };

    // ========== 构造函数 ==========

    using ContainerBlockEntity::closeContainer;
    using ContainerBlockEntity::openContainer;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ShulkerBoxEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~ShulkerBoxEntity() noexcept override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return SHULKER_BOX_SIZE; }

    // ========== IInventory 委托方法 ==========

    // 注：isEmpty 必须重写（IInventory 纯虚方法），委托给基类以触发战利品表填充。
    // clearContainer 继承自 LootableContainerBlockEntity，会自动触发 _unpackLootTable。
    // 以下 IInventory 方法需要在 m_inventory 操作前调用 _unpackLootTable(nullptr)，
    // 确保战利品表在首次容器内容访问时被正确填充。
    // 参考: net.minecraft.RandomizableContainerBlockEntity 中的 getItem/setItem/removeItem/removeItemNoUpdate

    // isEmpty 委托给基类（会触发 _unpackLootTable）
    [[nodiscard]] bool isEmpty() const override { return LootableContainerBlockEntity::isEmpty(); }

    [[nodiscard]] i32 getMaxStackSize() const override { return m_inventory.getMaxStackSize(); }
    [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override
    {
        return m_inventory.canPlaceItem(slot, stack);
    }
    void setChanged() override { LootableContainerBlockEntity::setChanged(); }

    // ========== IInventory 接口实现（带战利品表延迟填充）==========

    [[nodiscard]] ItemStack getItem(i32 slot) const override
    {
        _unpackLootTable(nullptr);
        return m_inventory.getItem(slot);
    }

    void setItem(i32 slot, const ItemStack& stack) override
    {
        _unpackLootTable(nullptr);
        m_inventory.setItem(slot, stack);
    }

    ItemStack removeItem(i32 slot, i32 count) override
    {
        _unpackLootTable(nullptr);
        return m_inventory.removeItem(slot, count);
    }

    ItemStack removeItemNoUpdate(i32 slot) override
    {
        _unpackLootTable(nullptr);
        return m_inventory.removeItemNoUpdate(slot);
    }

    void clear() override
    {
        _unpackLootTable(nullptr);
        m_inventory.clear();
    }

    // ========== ISidedInventory 接口实现 ==========

    /**
     * @brief 获取指定面可以访问的槽位
     *
     * 潜影盒可以从任意方向访问所有槽位。
     *
     * @param side 访问方向
     * @return 所有槽位索引数组（0-26）
     */
    [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override;

    /**
     * @brief 检查是否可以从指定方向向指定槽位插入物品
     *
     * 注意：潜影盒不能插入另一个潜影盒（防止递归）。
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
     * 潜影盒可以从任意方向提取任意物品。
     *
     * @param slot 槽位索引
     * @param stack 要提取的物品
     * @param direction 提取方向
     * @return 总是返回 true
     */
    [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;

    // ========== 潜影盒特有接口 ==========

    /**
     * @brief 获取动画状态
     * @return 动画状态
     */
    [[nodiscard]] AnimationStatus getAnimationStatus() const { return m_animationStatus; }

    /**
     * @brief 获取打开进度（0.0 - 1.0）
     * @return 打开进度
     */
    [[nodiscard]] f32 getProgress(f32 partialTick) const;

    /**
     * @brief 玩家打开潜影盒
     * @param player 玩家（可为nullptr）
     */
    void openContainer(Player* player) override;

    /**
     * @brief 玩家关闭潜影盒
     * @param player 玩家（可为nullptr）
     */
    void closeContainer(Player* player) override;

    /**
     * @brief 检查玩家是否可以打开
     * @param world 世界引用
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool canOpen(IWorld& world) const;

    // 引入基类的 canOpen 重载，避免被本类 canOpen(IWorld&) 隐藏
    using LootableContainerBlockEntity::canOpen;

    // ========== 方块事件 ==========

    /**
     * @brief 处理方块事件（客户端动画同步）
     * @param id 事件ID（1 = 开合动画）
     * @param type 事件参数（>0 表示打开，0 表示关闭）
     * @return 如果事件被处理返回true
     */
    [[nodiscard]] bool triggerEvent(i32 id, i32 type) override;

    /**
     * @brief 计算红石比较器信号
     * @param world 世界引用
     * @return 信号强度 (0-15)
     */
    [[nodiscard]] i32 getComparatorSignal(IWorld& world) const;

    // ========== 战利品表接口 ==========

    // 注：hasLootTable(), getLootTable(), getLootTableSeed(), setLootTable(), needsLootFill()
    // fillWithLoot() 继承自 LootableContainerBlockEntity，无需重写

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    /**
     * @brief 从 NBT 加载（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则加载容器物品列表。
     * LootTable 与 Items 互斥，与 MC Java RandomizableContainer 一致。
     */
    bool loadFromNBT(const nbt::CompoundTag& tag) override;

    /**
     * @brief 保存到 NBT（结构模板 / 客户端同步）
     *
     * 调用基类处理战利品表引用后，若无未解包的战利品表则保存容器物品列表。
     */
    void saveToNBT(nbt::CompoundTag& tag) const override;

    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

protected:
    [[nodiscard]] std::string getDefaultName() const override { return "container.shulkerBox"; }

private:
    /**
     * @brief 更新动画状态
     * @param partialTick 部分tick时间
     */
    void _updateAnimation(f32 partialTick);

    /**
     * @brief 检查是否可以打开（内部使用）
     * @param world 世界引用
     * @return 如果可以打开返回true
     */
    [[nodiscard]] bool _checkCanOpen(IWorld& world) const;

    /**
     * @brief 推动碰撞的实体
     *
     * 当潜影盒打开/关闭时，推动附近实体。
     *
     * @param world 世界引用
     * @param facing 潜影盒朝向（缓存）
     */
    void _moveCollidedEntities(IWorld& world, Direction facing);

    /**
     * @brief 缓存潜影盒朝向
     * @param world 世界引用
     */
    void _cacheFacing(IWorld& world);

    SimpleInventory m_inventory;                                 ///< 27格物品存储
    AnimationStatus m_animationStatus = AnimationStatus::Closed; ///< 动画状态
    f32 m_progress = 0.0f;                                       ///< 打开进度 (0.0 - 1.0)
    f32 m_prevProgress = 0.0f;                                   ///< 上一帧打开进度
    i32 m_openCount = 0;                                         ///< 打开计数
    Direction m_cachedFacing = Direction::None;                  ///< 缓存的朝向（避免每帧查询）
};

} // namespace blockentity
} // namespace mc
