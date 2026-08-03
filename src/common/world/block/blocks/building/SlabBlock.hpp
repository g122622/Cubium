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
#include "common/util/Direction.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/IBucketPickupHandler.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 台阶方块
 *
 * 支持单层和双层状态，双层时变成完整方块。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 注意：双层台阶不能含水，只有单层台阶可以含水。
 *
 * 状态属性：
 * - TYPE: 台阶类型 (BOTTOM, TOP, DOUBLE)
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.SlabBlock
 */
class SlabBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit SlabBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~SlabBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 检查方块是否可被替换
     *
     * 单层台阶可被同类型台阶替换以形成双层台阶。
     * 双层台阶不可被替换。
     *
     * 参考: net.minecraft.block.SlabBlock#isReplaceable
     */
    [[nodiscard]] bool isReplaceable(const BlockState& state, const BlockItemUseContext& context) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 单层台阶需要精确的形状遮挡检测，双层台阶不需要。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        // 单层台阶需要形状遮挡检测
        return state.get(BlockStateProperties::SLAB_TYPE()) != BlockStateProperties::SlabType::Double;
    }

    // ========== 其他 ==========

    /**
     * @brief 检查是否为双层
     * @param state 方块状态
     * @return 如果是双层返回true
     */
    [[nodiscard]] static bool isDouble(const BlockState& state);

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
     *
     * 如果方块含水且不是双层，返回水的流体状态。
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

    /**
     * @brief 检查方块是否含水
     *
     * 双层台阶永远不含水。
     */
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        // 双层台阶不能含水
        if (state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double) {
            return false;
        }
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    /**
     * @brief 检查是否可以容纳流体
     *
     * 双层台阶不能含水，委托给基类实现。
     */
    [[nodiscard]] bool canContainFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid) const override
    {
        // 双层台阶不能含水
        if (state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double) {
            return false;
        }
        return IWaterLoggable::canContainFluid(world, pos, state, fluid);
    }

    /**
     * @brief 接收流体
     *
     * 双层台阶不能接收流体，委托给基类实现。
     */
    bool receiveFluid(
        IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::FluidState& fluidState) override
    {
        // 双层台阶不能含水
        if (state.get(BlockStateProperties::SLAB_TYPE()) == BlockStateProperties::SlabType::Double) {
            return false;
        }
        return IWaterLoggable::receiveFluid(world, pos, state, fluidState);
    }

private:
    /// 下半台阶形状
    CollisionShape m_bottomShape;

    /// 上半台阶形状
    CollisionShape m_topShape;

    /// 完整方块形状
    CollisionShape m_fullCubeShape;
};

} // namespace blocks
} // namespace mc
