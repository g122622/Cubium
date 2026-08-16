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

#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <cstddef>

namespace mc {

class IWorld;
class BlockItemUseContext;

namespace world::gen {
class WorldGenRegion;
}

namespace blocks {

/**
 * @brief 紫颂植物方块
 *
 * 末地的植物，可以生长紫颂果。
 *
 * 状态属性：
 * - NORTH/SOUTH/EAST/WEST/DOWN/UP: 各方向连接
 *
 * 形状系统：预计算 64 种组合（2^6），使用位掩码索引
 */
class ChorusPlantBlock : public Block {
public:
    explicit ChorusPlantBlock(const BlockProperties& properties);
    ~ChorusPlantBlock() override = default;

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

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    /**
     * @brief 紫颂植物不透传天空光（对齐 vanilla PipeBlock#propagatesSkylightDown=false）
     *
     * vanilla PipeBlock（紫颂植物基类）显式 override 返回 false。紫颂植物 getShape 虽是
     * SimpleBox（非完整立方体），默认公式会得 true（错误），故 override 对齐 vanilla：
     * 不透传天空光，opacity=1（衰减 1 级）。
     */
    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override
    {
        MC_UNUSED(state);
        MC_UNUSED(world);
        MC_UNUSED(pos);
        return false;
    }

    /**
     * @brief 根据连接状态计算形状索引
     *
     * 使用位掩码：Down=1, Up=2, North=4, South=8, West=16, East=32
     *
     * @param state 方块状态
     * @return 形状索引 (0-63)
     */
    [[nodiscard]] static size_t getShapeIndex(const BlockState& state);

    /**
     * @brief 根据周围方块自动计算连接状态并返回对应的方块状态
     *
     * 用于世界生成（ChorusPlantFeature）和紫颂树生长（ChorusFlowerBlock），
     * 在放置紫颂植物方块时自动检测周围可以连接的方块。
     *
     * @param world 世界引用（用于方块查询）
     * @param pos 当前方块位置
     * @param defaultState 紫颂植物的默认方块状态
     * @return 带有正确连接属性的方块状态
     */
    [[nodiscard]] static BlockState getStateWithConnections(
        IWorld& world, const BlockPos& pos, const BlockState& defaultState);

    /**
     * @brief 检查是否连接到指定方向
     *
     * 连接规则：
     * - 所有方向：连接到紫颂植物和紫颂花
     * - 仅下方：额外连接到末地石
     *
     * @param world 方块读取器
     * @param pos 当前方块位置
     * @param direction 检查方向
     * @return true 如果应该连接
     */
    [[nodiscard]] bool _canConnect(IBlockReader& world, const BlockPos& pos, Direction direction) const;

private:
    CollisionShape m_centerShape;            ///< 中心柱形状
    CollisionShape m_armShapes[6];           ///< 6 个方向的臂形状
    std::array<CollisionShape, 64> m_shapes; ///< 预计算的 64 种组合形状
};

} // namespace blocks
} // namespace mc
