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

#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/world/block/Block.hpp"

namespace mc {

/**
 * @brief 方块物品类
 *
 * 继承自 Item 类，用于处理方块的放置。
 * 每个 BlockItem 与一个 Block 关联。
 *
 * 参考: net.minecraft.item.BlockItem
 */
class BlockItem : public Item {
public:
    /**
     * @brief 构造方块物品
     * @param block 关联的方块
     * @param properties 物品属性
     */
    BlockItem(const Block& block, ItemProperties properties);

    ~BlockItem() override = default;

    // ========== 基本属性 ==========

    /**
     * @brief 获取关联的方块
     */
    [[nodiscard]] const Block& block() const { return *m_block; }

    // ========== Item 接口实现 ==========

    /**
     * @brief 在方块上使用物品
     *
     * 当玩家右键点击方块时调用。
     * 参考 MC 1.16.5: BlockItem.onItemUse
     *
     * @param context 物品使用上下文
     * @return 动作结果类型
     */
    ActionResultType onItemUse(ItemUseContext& context) override;

    // ========== 放置逻辑 ==========

    /**
     * @brief 尝试放置方块
     *
     * 检查放置条件，获取放置状态，并执行放置。
     *
     * @param context 放置上下文
     * @return 动作结果类型
     */
    [[nodiscard]] ActionResultType tryPlace(BlockItemUseContext& context) const;

    /**
     * @brief 获取放置上下文
     *
     * 子类可重写以修改放置行为（如床、门等）。
     * 默认返回原始上下文。
     *
     * @param context 原始放置上下文
     * @return 修改后的放置上下文，如果无法放置返回 nullptr
     */
    [[nodiscard]] virtual BlockItemUseContext getBlockItemUseContext(BlockItemUseContext& context) const;

    /**
     * @brief 获取放置时的方块状态
     *
     * 子类可重写以支持有方向/状态的方块。
     * 默认实现返回方块的默认状态。
     *
     * @param context 放置上下文
     * @return 方块状态指针，如果不能放置返回 nullptr
     */
    [[nodiscard]] virtual const BlockState* getStateForPlacement(const BlockItemUseContext& context) const;

    /**
     * @brief 检查是否可以在此位置放置
     *
     * 检查方块状态是否可以放置在指定位置。
     *
     * @param context 放置上下文
     * @param state 要放置的方块状态
     * @return 是否可以放置
     */
    [[nodiscard]] bool canPlace(const BlockItemUseContext& context, const BlockState& state) const;

    /**
     * @brief 是否检查方块位置有效性
     *
     * 子类可重写以禁用位置检查（如末地传送门框架）。
     *
     * @return 默认返回 true
     */
    [[nodiscard]] virtual bool checkPosition() const { return true; }

protected:
    /**
     * @brief 执行方块放置
     *
     * 在世界中设置方块状态。
     * 子类可重写以添加额外效果（如播放声音）。
     *
     * @param context 放置上下文
     * @param state 要放置的方块状态
     * @return 是否放置成功
     */
    [[nodiscard]] virtual bool placeBlock(BlockItemUseContext& context, const BlockState* state) const;

    /**
     * @brief 方块放置后的处理
     *
     * 处理方块实体 NBT 数据。
     *
     * @param pos 方块位置
     * @param world 世界引用
     * @param player 玩家指针（可为nullptr）
     * @param stack 物品堆
     * @param state 方块状态
     * @return 是否成功设置了方块实体数据
     */
    [[nodiscard]] virtual bool onBlockPlaced(
        const BlockPos& pos, IWorld& world, Player* player, const ItemStack& stack, const BlockState& state) const;

    /**
     * @brief 检查放置位置是否有效
     *
     * 检查放置位置是否在方块碰撞范围内。
     *
     * @param context 放置上下文
     * @return 是否有效
     */
    [[nodiscard]] bool checkPositionValid(const BlockItemUseContext& context) const;

    /**
     * @brief 从物品 NBT 应用方块状态
     *
     * 处理物品堆中的 BlockStateTag，设置方块属性。
     *
     * @param pos 方块位置
     * @param world 世界引用
     * @param stack 物品堆
     * @param state 原始方块状态
     * @return 修改后的方块状态
     */
    [[nodiscard]] const BlockState* applyBlockStateFromNBT(
        const BlockPos& pos, IWorld& world, const ItemStack& stack, const BlockState& state) const;

private:
    /**
     * @brief 将物品堆中的 BlockEntityTag 数据应用到方块实体
     *
     * 参考 MC Java: BlockItem.updateCustomBlockEntityTag()
     * 从物品堆的 "BlockEntityTag" 子标签读取自定义数据，
     * 合并到指定位置的方块实体中。
     *
     * 流程：
     * 1. 检查物品堆是否有 BlockEntityTag
     * 2. 获取目标位置的方块实体
     * 3. 检查方块实体是否需要OP权限（onlyOpsCanSetNbt）
     * 4. 验证类型ID匹配
     * 5. 保存当前数据，合并BlockEntityTag，加载合并后数据
     * 6. 失败时回滚
     *
     * @param world 世界引用
     * @param player 放置方块的玩家（可为nullptr）
     * @param pos 方块位置
     * @param stack 物品堆
     * @return 是否成功设置了方块实体数据
     */
    bool setTileEntityNBT(IWorld& world, Player* player, const BlockPos& pos, const ItemStack& stack) const;

    const Block* m_block;
};

} // namespace mc
