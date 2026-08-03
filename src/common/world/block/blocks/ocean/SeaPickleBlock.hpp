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
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include <array>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 海泡菜方块
 *
 * 可放置在水下的发光方块，可以堆叠最多4个。
 * 放置在水中的海泡菜会发光。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * ## 状态属性
 * - PICKLES_1_4: 海泡菜数量 (1-4)
 * - WATERLOGGED: 是否含水
 *
 * ## 发光机制
 * - 在水中时发光
 * - 亮度随数量增加：1个=6, 2个=9, 3个=12, 4个=15
 * - 离开水不发光
 *
 * 参考: net.minecraft.block.SeaPickleBlock
 */
class SeaPickleBlock : public Block, public IWaterLoggable {
public:
    explicit SeaPickleBlock(const BlockProperties& properties);
    ~SeaPickleBlock() override = default;

    // ========== 状态属性 ==========

    /**
     * @brief 获取海泡菜数量
     * @param state 方块状态
     * @return i32 数量（1-4）
     */
    [[nodiscard]] i32 getPickles(const BlockState& state) const;

    /**
     * @brief 设置海泡菜数量
     * @param count 数量（1-4）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] BlockState withPickles(i32 count) const;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 光照 ==========

    /**
     * @brief 获取发光等级
     *
     * 只有在水中时才发光，亮度随数量增加：
     * - 1个: 6
     * - 2个: 9
     * - 3个: 12
     * - 4个: 15
     *
     * @param state 方块状态
     * @return u8 发光等级（0-15）
     */
    [[nodiscard]] u8 getLightLevel(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== IWaterLoggable 接口实现 ==========

    /**
     * @brief 获取流体状态
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
    /// 各数量的形状（索引0=1个，索引1=2个，...）
    std::array<CollisionShape, 4> m_shapesByCount;
};

} // namespace blocks
} // namespace mc
