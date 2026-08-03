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
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 脚手架方块
 *
 * 脚手架是一种特殊的可攀爬方块：
 * - 可以堆叠放置
 * - 可以攀爬
 * - 玩家可以在上面行走
 * - 距离底部过远会掉落
 * - 实现 IWaterLoggable 接口支持含水功能
 */
class ScaffoldingBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit ScaffoldingBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 更新 ==========

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    /**
     * @brief 方块被添加到世界时的处理
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 执行方块计划刻
     */
    void tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const noexcept override;

    /**
     * @brief 获取碰撞形状
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const noexcept override;

    // ========== 攀爬 ==========

    /**
     * @brief 检查方块是否可攀爬
     *
     * 脚手架始终可攀爬。
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool isLadder(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const noexcept override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
        MC_UNUSED(state);
        return true;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const noexcept override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 静态方法 ==========

    /**
     * @brief 计算脚手架距离支撑点的距离
     *
     * @param world 世界引用
     * @param pos 脚手架位置
     * @return 距离值 (0-7)，0 表示直接支撑，7 表示过远需要掉落
     */
    static i32 calculateDistance(IWorld& world, const BlockPos& pos);

protected:
    /// 底部形状（站立平台）
    CollisionShape m_baseShape;
    /// 顶部横杆形状
    CollisionShape m_topShape;
    /// 完整形状（含支撑柱）
    CollisionShape m_fullShape;
    /// 无碰撞形状（当脚手架距离=0且有底部支撑时）
    CollisionShape m_emptyShape;

    /**
     * @brief 检查是否应该显示底部支撑柱
     *
     * @param world 世界引用
     * @param pos 脚手架位置
     * @param distance 距离值
     * @return 如果应该显示底部返回 true
     */
    [[nodiscard]] static bool shouldShowBottom(IWorld& world, const BlockPos& pos, i32 distance) noexcept;

    /**
     * @brief 检查方块是否为脚手架
     */
    [[nodiscard]] static bool isScaffolding(const BlockState* state) noexcept;
};

} // namespace blocks
} // namespace mc
