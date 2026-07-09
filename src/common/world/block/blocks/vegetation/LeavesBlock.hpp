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

#include "../../../../physics/collision/CollisionShape.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../Block.hpp"
#include "../../IWaterLoggable.hpp"
#include "../../Material.hpp"

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
 * 参考: net.minecraft.block.LeavesBlock
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
     * @brief 是否传播天空光
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 是否不透明
     */
    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 获取碰撞形状（树叶无碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

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
