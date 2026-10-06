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
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 干燥恶魂方块
 *
 * 置于水中时可逐级提升湿润等级（每 5000 tick 一级，0→3），达到 3 级后孵化为幼年恶魂；
 * 离开水则等级逐级下降至 0。具有水平朝向属性，湿润等级达到 3 时可供玩家收集。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 水平朝向
 * - DRIED_GHAST_HYDRATION_LEVELS: 湿润等级 (0-3)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.world.level.block.DriedGhastBlock
 */
class DriedGhastBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造干燥恶魂方块
     * @param properties 方块属性
     */
    explicit DriedGhastBlock(const BlockProperties& properties);

    ~DriedGhastBlock() override = default;

    // ========== 状态访问 ==========

    /**
     * @brief 获取湿润等级
     * @param state 方块状态
     * @return 湿润等级 (0-3)
     */
    [[nodiscard]] i32 getHydrationLevel(const BlockState& state) const;

    // ========== 放置与更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    void onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 计划刻 / 随机刻 ==========

    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 客户端动画 ==========

    void animateTick(IBlockAnimateContext& context,
        const BlockPos& pos,
        const BlockState& state,
        math::IRandom& random) const override;

    // ========== 形状与渲染 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 旋转 / 镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    /**
     * @brief 干燥恶魂不可被路径寻找通过
     */
    [[nodiscard]] bool allowsMovement(const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== IWaterLoggable 接口实现 ==========

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 接收水（含水后调度水流体 tick 并播放入水音效）
     */
    bool receiveFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState) override;

private:
    /**
     * @brief 湿润等级是否已达孵化阈值
     */
    [[nodiscard]] bool _isReadyToSpawn(const BlockState& state) const;

    /**
     * @brief 含水状态下的计划刻：提升湿润等级或孵化为幼年恶魂
     */
    void _tickWaterlogged(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 孵化为幼年恶魂
     */
    void _spawnGhastling(IWorld& world, const BlockPos& pos, const BlockState& state);

    /// 碰撞形状（10x10 底面，高 10/16）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
