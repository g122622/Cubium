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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/capability/IWaterLoggable.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {

class IWorld;
class BlockPos;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 树叶方块
 *
 * 树叶方块会检测与原木的距离，距离超过6格时会腐烂。
 * 玩家放置的树叶（PERSISTENT=true）不会腐烂。
 *
 * 状态属性:
 * - DISTANCE (1-7): 距离最近原木的距离，7表示超过6格
 * - PERSISTENT: 是否持久（玩家放置的树叶）
 * - WATERLOGGED: 是否含水
 *
 */
class LeavesBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LeavesBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~LeavesBlock() override = default;

    /**
     * @brief 树叶不提供任何支撑面
     *
     * 对齐 MC 1.21.11 的 `LeavesBlock#getBlockSupportShape`：
     *
     * ```java
     * @Override
     * protected VoxelShape getBlockSupportShape(BlockState state, BlockGetter level, BlockPos pos) {
     *     return Shapes.empty();
     * }
     * ```
     *
     * 树叶的碰撞形状是完整方块，但**支撑形状恒为空**——因此火把、藤蔓、梯子等
     * 附着型方块无法贴在树叶上。若缺少这个覆写，`isFaceSturdy` 会按碰撞形状判定，
     * 导致所有「能否附着在树叶上」的判断都错成 true。
     */
    [[nodiscard]] const CollisionShape& getBlockSupportShape(const BlockState& state) const override;

    /**
     * @brief 获取放置状态
     *
     * 玩家放置的树叶会被标记为持久（PERSISTENT=true）
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新逻辑 ==========

    /**
     * @brief 邻居更新时调度距离更新
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 方块Tick - 更新距离
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 只有「距离 7 且非持久」的树叶才响应随机刻
     *
     * 树叶是否腐烂由 DISTANCE 决定：distance=7 表示已经连不上树干。
     * 其余状态（distance < 7 或 persistent=true）不需要随机刻 ——
     * 它们要么还在树上，要么是玩家放置的永久叶。
     *
     * 不覆写这条会让**所有**树叶每刻都被随机刻调度，白白消耗随机刻预算，
     * 并让本该静止的树叶反复参与腐烂判定。
     */
    [[nodiscard]] bool isRandomlyTicking(const BlockState& state) const noexcept override;

    /**
     * @brief 是否需要随机Tick
     *
     * 只有距离为7且非持久的树叶才需要随机Tick（用于腐烂）
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override;

    /**
     * @brief 随机Tick - 腐烂逻辑
     *
     * 距离为7且非持久的树叶会腐烂并掉落
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 渲染属性 ==========

    /**
     * @brief 获取光照透明度
     *
     * 树叶透明度为1，允许部分光线通过
     */
    [[nodiscard]] i32 getOpacity(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return 1;
    }

    /**
     * @brief 不重写 `propagatesSkylightDown`，走基类默认推导
     *
     * 原版 `LeavesBlock` 没有重写它，注册属性里也没有 `propagatesSkylightDown()`；默认公式
     * 「遮挡形状非完整立方体 且 无流体」对本方块为 false——树叶的遮挡形状是完整立方体
     * （原版 `noOcclusion()` 只关掉 canOcclude，不影响形状）。因此天空光穿过树叶会衰减
     * 1 级（lightBlock=1），而不是无衰减直通。
     *
     * 早期实现这里返回 true，导致树冠下的地面比原版亮一级。
     */

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 含水接口 ==========

    /**
     * @brief 获取方块对应的流体状态
     *
     * 含水时返回水的流体状态，否则返回空。
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

private:
    /**
     * @brief 更新树叶到原木的距离
     * @param state 当前状态
     * @param world 世界
     * @param pos 树叶位置
     * @return 更新后的状态
     */
    static BlockState _updateDistance(const BlockState& state, IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取邻居方块的距离值
     * @param neighborState 邻居状态
     * @return 距离值（原木=0，树叶=其DISTANCE值，其他=7）
     */
    static i32 _getDistance(const BlockState& neighborState);
};

} // namespace blocks
} // namespace mc
