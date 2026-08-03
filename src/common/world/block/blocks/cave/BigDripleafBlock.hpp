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
 * copies of substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IGrowable.hpp"
#include "../../IWaterLoggable.hpp"
#include "common/core/Types.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc {

// 前向声明
class Entity;
class BlockRaycastResult;
namespace blocks {

/**
 * @brief 大滴叶方块
 *
 * 大型叶片方块，可以被玩家或实体踩倾斜，支持含水。
 * 倾斜状态: NONE→UNSTABLE(10tick)→PARTIAL(10tick)→FULL(100tick)→NONE
 * 完全倾斜时无碰撞箱，玩家会掉落。
 * 红石信号可立即重置倾斜状态为NONE。
 *
 * 参考: net.minecraft.block.BigDripleafBlock
 */
class BigDripleafBlock : public Block, public IWaterLoggable, public IGrowable {
public:
    explicit BigDripleafBlock(const BlockProperties& properties);

    ~BigDripleafBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 处理倾斜进度和实体检测
     *
     * UNSTABLE→PARTIAL: 10tick后
     * PARTIAL→FULL: 10tick后
     * FULL→NONE: 100tick后自动重置
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 红石信号响应 - 重置倾斜状态
     */
    void neighborChanged(
        IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving) override;

    /**
     * @brief 投掷物击中时直接设为FULL倾斜
     */
    void onProjectileHit(
        IWorld& world, const BlockState& state, const BlockRaycastResult& hitResult, Entity& projectile) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== IGrowable 接口 ==========

    /**
     * @brief 大滴叶不支持骨粉（小型垂滴叶才支持）
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        MC_UNUSED(isClientSide);
        return false;
    }

    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(random);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return false;
    }

    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override
    {
        MC_UNUSED(world);
        MC_UNUSED(random);
        MC_UNUSED(pos);
        MC_UNUSED(state);
    }

    /**
     * @brief 当实体站在叶片上时触发倾斜
     *
     * 检测到实体站在上面时，将TILT设为UNSTABLE并调度tick
     */
    void onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

private:
    CollisionShape m_fullShape;

    /// 实体检测最小Y偏移（像素 11/16 = 0.6875）
    static constexpr f32 ENTITY_DETECTION_MIN_Y = 0.6875f;

    /**
     * @brief 获取倾斜状态的tick延迟
     */
    static i32 _getTiltDelay(BlockStateProperties::Tilt tilt);

    /**
     * @brief 调度下一次倾斜tick
     */
    void _scheduleTiltTick(IWorld& world, const BlockPos& pos, BlockStateProperties::Tilt tilt) const;

    /**
     * @brief 设置倾斜状态并调度下一次tick
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param tilt 目标倾斜状态
     * @param playSound 是否播放倾斜音效
     */
    void _setTiltAndScheduleTick(
        IWorld& world, const BlockPos& pos, BlockState& state, BlockStateProperties::Tilt tilt, bool playSound);

    /**
     * @brief 设置倾斜状态
     *
     * @param world 世界
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param tilt 目标倾斜状态
     */
    static void _setTilt(IWorld& world, const BlockPos& pos, BlockState& state, BlockStateProperties::Tilt tilt);

    /**
     * @brief 重置倾斜状态为NONE，如果之前不是NONE则播放音效
     */
    void _resetTilt(IWorld& world, const BlockPos& pos, BlockState& state);

    /**
     * @brief 检查实体是否能触发倾斜
     *
     * 实体必须站在地面上，且Y坐标高于方块顶部 11/16（0.6875）
     */
    [[nodiscard]] static bool _canEntityTilt(const BlockPos& pos, const Entity& entity);
};

} // namespace blocks
} // namespace mc
