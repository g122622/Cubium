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
#include "common/world/block/IWaterLoggable.hpp"
#include "common/world/block/Material.hpp"
#include <array>
#include <cstddef>

namespace mc {

class IWorld;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 栅栏方块
 *
 * 支持与相邻栅栏/墙连接。
 * 实现 IWaterLoggable 接口支持含水功能。
 *
 * 状态属性：
 * - NORTH/WEST/EAST/SOUTH: 各方向是否连接
 * - WATERLOGGED: 是否含水
 */
class FenceBlock : public Block, public IWaterLoggable {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit FenceBlock(const BlockProperties& properties);

    /**
     * @brief 析构函数
     */
    ~FenceBlock() override = default;

    // ========== 放置和更新 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getOcclusionShape(const BlockState& state) const override;

    /**
     * @brief 是否使用形状进行光照遮挡检测
     *
     * 栅栏有复杂的形状，需要精确的形状遮挡检测。
     */
    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
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
    [[nodiscard]] bool isWaterlogged(const BlockState& state) const override
    {
        return state.get(BlockStateProperties::WATERLOGGED());
    }

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

private:
    /**
     * @brief 检查是否可以连接到邻居
     * @param state 邻居状态
     * @param isNeighborSolid 邻居是否为固体
     * @param direction 从栅栏指向邻居的方向
     * @return 如果可以连接返回true
     */
    [[nodiscard]] bool _canConnect(const BlockState& state, bool isNeighborSolid, Direction direction) const;

    /**
     * @brief 获取形状索引
     * @param north 北面是否连接
     * @param east 东面是否连接
     * @param south 南面是否连接
     * @param west 西面是否连接
     * @return 形状索引 (0-15)
     */
    [[nodiscard]] static size_t _getShapeIndex(bool north, bool east, bool south, bool west);

    /// 预计算的形状缓存（16种组合）
    std::array<CollisionShape, 16> m_shapes;
};

} // namespace blocks
} // namespace mc
