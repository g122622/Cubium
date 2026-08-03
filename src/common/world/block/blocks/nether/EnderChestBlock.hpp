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
#include "common/item/core/BlockActionResult.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <memory>

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class Player;
class BlockState;
class BlockPos;
class BlockEntity;
class BlockRaycastResult;

namespace fluid {
class FluidState;
}

namespace blocks {

/**
 * @brief 末影箱方块
 *
 * 末影箱是一种特殊的存储方块，所有末影箱共享同一物品栏（每个玩家独立）。
 * 物品存储在玩家数据中，而非方块实体中。方块实体仅用于开盖动画和音效。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 朝向 (NORTH, SOUTH, EAST, WEST)
 * - WATERLOGGED: 是否含水
 */
class EnderChestBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit EnderChestBlock(const BlockProperties& properties);
    ~EnderChestBlock() noexcept override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    // ========== 方块实体 ==========

    [[nodiscard]] bool hasBlockEntity() const noexcept override { return true; }

    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

    // ========== 交互 ==========

    /**
     * @brief 玩家右键点击方块时调用
     *
     * 检查方块上方是否有红石导体阻挡，然后打开末影箱容器界面。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

    // ========== 状态更新 ==========

    /**
     * @brief 处理相邻方块更新（含水方块调度流体tick）
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction direction,
        const BlockState& neighborState,
        IWorld& world,
        const BlockPos& pos,
        const BlockPos& neighborPos) override;

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

protected:
    /// 末影箱形状（14x14底部，非完整方块）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
