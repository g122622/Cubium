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
 * @brief 梯子方块
 *
 * 梯子是一种可以攀爬的方块：
 * - 只能附在固体方块的侧面
 * - 实现 IWaterLoggable 接口支持含水功能
 * - 可以攀爬
 * - 没有碰撞箱
 */
class LadderBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit LadderBlock(const BlockProperties& properties);

    // ========== 状态创建 ==========

    /**
     * @brief 获取放置状态
     */
    BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 严格放置判定（对齐 vanilla LadderBlock.getStateForPlacement 返回 null 的两处放弃条件）
     *
     * 1. 未替换点击方块（贴在方块侧面）时，若点击位置内侧方块是朝向点击面的同种梯子，则放弃；
     * 2. getStateForPlacement 选出的候选状态必须能存活（canSurvive），否则放弃。
     *
     * 基类 BlockItem::canPlace 以 canSurvive 校验 `state.owner()`，而在"全部候选都不能存活"时
     * getStateForPlacement 只能回退默认朝向（其 owner 仍是本方块），故基类校验无法表达"放弃放置"。
     * 本方法在 BlockItem 计算放置状态之前给出精确判定。
     *
     * @param context 放置上下文
     * @return 可以放置返回 true
     */
    [[nodiscard]] bool canPlaceAt(const BlockItemUseContext& context) const override;

    // ========== 放置检测 ==========

    /**
     * @brief 检查是否可以放置（等价原版 canSurvive）
     *
     * 梯子必须附着在朝向反方向方块的相应面上，且该面为实体面
     * （原版 canAttachTo → BlockState.isFaceSturdy）。
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 邻居更新
     */
    BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转 ==========

    /**
     * @brief 旋转方块
     */
    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    /**
     * @brief 镜像方块
     */
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    /**
     * @brief 获取形状
     */
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 获取碰撞形状（梯子没有碰撞）
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 检查方块是否可攀爬
     *
     * 梯子始终可攀爬。
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

protected:
    /// 各方向的形状
    CollisionShape m_shapes[6];
};

} // namespace blocks
} // namespace mc
