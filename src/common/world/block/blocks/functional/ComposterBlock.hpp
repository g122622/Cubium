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

#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../entity/inventory/ISidedInventoryProvider.hpp"
#include "../../../../item/core/ActionResult.hpp"
#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../Material.hpp"
#include "entity/inventory/ISidedInventory.hpp"
#include <array>
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Entity;
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 堆肥桶方块
 *
 * 用于将植物材料转化为骨粉的功能方块。
 * 具有8个填充等级（0-7），满级后可产出骨粉。
 * 实现 ISidedInventoryProvider 接口以支持漏斗自动化交互：
 * - 漏斗可从上方输入可堆肥物品（等级 0-6）
 * - 漏斗可从下方提取骨粉（等级 8）
 * - 等级 7 时不允许任何交互（等待 20 tick 转变）
 *
 * 状态属性：
 * - LEVEL_0_8: 填充等级 (0-8，8表示完成)
 */
class ComposterBlock : public Block, public ISidedInventoryProvider {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ComposterBlock(const BlockProperties& properties);
    ~ComposterBlock() noexcept override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== Tick ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return false; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 渲染属性 ==========

    [[nodiscard]] bool hasComparatorInputOverride(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] i32 getComparatorInputOverride(
        const BlockState& state, IWorld& world, const BlockPos& pos) const override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键交互
     *
     * - 如果等级为8，取出骨粉
     * - 如果玩家手持可堆肥物品，尝试堆肥
     * - 否则不执行任何操作
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== ISidedInventoryProvider 接口 ==========

    /**
     * @brief 根据方块状态创建侧面背包
     *
     * 根据堆肥桶等级返回不同的容器：
     * - 等级 0-6: InputContainer（仅允许从上方输入可堆肥物品）
     * - 等级 7: EmptyContainer（不允许任何交互，等待 20 tick 转变）
     * - 等级 8: OutputContainer（仅允许从下方提取骨粉）
     *
     * 参考: MC Java ComposterBlock.getContainer()
     */
    [[nodiscard]] std::unique_ptr<ISidedInventory> createInventory(
        const BlockState& state, IWorld& world, const BlockPos& pos) override;

    // ========== 工具方法 ==========

    /**
     * @brief 获取填充等级
     */
    [[nodiscard]] static i32 getLevel(const BlockState& state) { return state.get(BlockStateProperties::LEVEL_0_8()); }

    /**
     * @brief 尝试堆肥
     * @param state 当前方块状态
     * @param world 世界引用
     * @param pos 方块位置
     * @param block 方块引用（用于调度tick）
     * @param itemId 物品ID
     * @return 新的方块状态（可能改变等级）
     */
    static BlockState attemptCompost(
        const BlockState& state, IWorld& world, const BlockPos& pos, const Block& block, u32 itemId);

    /**
     * @brief 清空堆肥桶
     */
    static BlockState empty(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 检查物品是否可堆肥
     */
    [[nodiscard]] static bool isCompostable(u32 itemId);

    /**
     * @brief 获取物品的堆肥概率
     * @return 0.0-1.0之间的概率，0.0表示不可堆肥
     */
    [[nodiscard]] static f32 getCompostChance(u32 itemId);

private:
    /**
     * @brief 堆肥桶空容器（等级 7 时使用）
     *
     * 0 槽位容器，不允许任何方向的插入或提取。
     * 等级 7 是过渡状态，等待 20 tick 后转变为等级 8。
     *
     * 参考: MC Java ComposterBlock.EmptyContainer
     */
    class EmptyContainer : public ISidedInventory {
    public:
        [[nodiscard]] i32 getContainerSize() const noexcept override { return 0; }
        [[nodiscard]] bool isEmpty() const noexcept override { return true; }
        [[nodiscard]] ItemStack getItem(i32 slot) const override;
        void setItem(i32 slot, const ItemStack& stack) override;
        ItemStack removeItem(i32 slot, i32 count) override;
        ItemStack removeItemNoUpdate(i32 slot) override;
        void clear() override {}
        void setChanged() override {}
        [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override;
        [[nodiscard]] bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const override;
        [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;
    };

    /**
     * @brief 堆肥桶输入容器（等级 0-6 时使用）
     *
     * 1 槽位容器，仅允许从上方（Direction::Up）输入可堆肥物品。
     * 当物品被放入槽位时，自动调用 ComposterBlock::attemptCompost() 处理堆肥，
     * 并将 changed 标记设为 true 防止重复处理。
     *
     * 参考: MC Java ComposterBlock.InputContainer
     */
    class InputContainer : public ISidedInventory {
    public:
        InputContainer(const BlockState& state, IWorld& world, const BlockPos& pos);

        [[nodiscard]] i32 getContainerSize() const noexcept override { return 1; }
        [[nodiscard]] bool isEmpty() const noexcept override;
        [[nodiscard]] i32 getMaxStackSize() const noexcept override { return 1; }
        [[nodiscard]] ItemStack getItem(i32 slot) const override;
        void setItem(i32 slot, const ItemStack& stack) override;
        ItemStack removeItem(i32 slot, i32 count) override;
        ItemStack removeItemNoUpdate(i32 slot) override;
        void clear() override;
        void setChanged() override;
        [[nodiscard]] bool canPlaceItem(i32 slot, const ItemStack& stack) const override;
        [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override;
        [[nodiscard]] bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const override;
        [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;

    private:
        const BlockState& m_state;
        IWorld& m_world;
        BlockPos m_pos;
        bool m_changed = false;
        ItemStack m_item;
    };

    /**
     * @brief 堆肥桶输出容器（等级 8 时使用）
     *
     * 1 槽位容器，包含 1 个骨粉，仅允许从下方（Direction::Down）提取。
     * 当骨粉被提取时，自动调用 ComposterBlock::empty() 重置堆肥桶为等级 0，
     * 并将 changed 标记设为 true 防止重复提取。
     *
     * 参考: MC Java ComposterBlock.OutputContainer
     */
    class OutputContainer : public ISidedInventory {
    public:
        OutputContainer(const BlockState& state, IWorld& world, const BlockPos& pos);

        [[nodiscard]] i32 getContainerSize() const noexcept override { return 1; }
        [[nodiscard]] bool isEmpty() const noexcept override;
        [[nodiscard]] i32 getMaxStackSize() const noexcept override { return 1; }
        [[nodiscard]] ItemStack getItem(i32 slot) const override;
        void setItem(i32 slot, const ItemStack& stack) override;
        ItemStack removeItem(i32 slot, i32 count) override;
        ItemStack removeItemNoUpdate(i32 slot) override;
        void clear() override;
        void setChanged() override;
        [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override;
        [[nodiscard]] bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const override;
        [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override;

    private:
        const BlockState& m_state;
        IWorld& m_world;
        BlockPos m_pos;
        bool m_changed = false;
        ItemStack m_item;
    };

protected:
    /// 各等级的渲染形状缓存（等级0-8，等级8与等级7相同）
    /// 形状为完整方块减去内部12像素宽柱体后的外壁：底板 + 四面墙壁
    /// MC Java: Shapes.join(Shapes.block(), Block.column(12, clamp(1+level*2, 2, 16), 16), ONLY_FIRST)
    std::array<CollisionShape, 9> m_shapesByLevel;
};

} // namespace blocks
} // namespace mc
