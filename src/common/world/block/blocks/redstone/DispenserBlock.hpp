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

#include "../../../../util/Direction.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../redstone/RedstonePower.hpp"
#include "../../Block.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/Material.hpp"
#include <memory>

namespace mc {

// 前向声明
class IInventory;

namespace blocks {

/**
 * @brief 发射器方块
 *
 * 发射器可以发射物品，对特定物品有特殊行为。
 *
 * ## 特性
 * - 9格存储空间
 * - 红石激活时随机发射物品
 * - 特殊行为：箭矢、火焰弹、TNT等
 * - 方向性：可向6个方向发射
 *
 * ## 容易踩的坑
 * - 发射器朝向和发射方向的关系
 * - 需要与 DispenserBlockEntity 配合
 * - 特殊物品行为处理
 *
 * 参考: net.minecraft.block.DispenserBlock
 */
class DispenserBlock : public Block {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit DispenserBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const noexcept override
    {
        MC_UNUSED(state);
        return false;
    }

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return Material::PushReaction::Normal;
    }

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 发射器特有方法 ==========

    /**
     * @brief 检查发射器是否被触发
     *
     * @param state 方块状态
     * @return true 如果被触发
     */
    [[nodiscard]] static bool isTriggered(const BlockState& state);

    /**
     * @brief 设置触发状态
     *
     * @param state 方块状态
     * @param triggered 是否触发
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withTriggered(BlockState state, bool triggered);

    /**
     * @brief 获取发射器朝向
     *
     * @param state 方块状态
     * @return Direction 发射方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 发射物品
     *
     * 子类可以重写此方法实现不同的发射行为。
     * 例如，投掷器重写此方法使用简单的投掷行为。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    virtual void dispense(IWorld& world, const BlockPos& pos, const BlockState& state);

protected:
    /**
     * @brief 尝试从发射器位置发射物品
     *
     * 子类可以重写此方法实现不同的发射逻辑。
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param state 当前方块状态
     * @return true 如果成功发射
     */
    virtual bool tryDispense(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 播放发射音效
     *
     * 子类可以重写此方法播放不同的音效。
     *
     * @param world 世界引用
     * @param pos 方块位置
     */
    virtual void playDispenseSound(IWorld& world, const BlockPos& pos);

    /**
     * @brief 默认发射行为
     *
     * 处理将物品放入容器或生成物品实体。
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param facing 发射方向
     * @param targetPos 目标位置
     * @param stack 要发射的物品堆
     * @return 发射后剩余的物品堆
     */
    static ItemStack defaultDispense(
        IWorld& world, const BlockPos& pos, Direction facing, const BlockPos& targetPos, ItemStack stack);

    /**
     * @brief 生成物品实体
     *
     * @param world 世界引用
     * @param pos 发射器位置
     * @param facing 发射方向
     * @param stack 物品堆
     */
    static void spawnItemEntity(IWorld& world, const BlockPos& pos, Direction facing, const ItemStack& stack);
};

} // namespace blocks
} // namespace mc
