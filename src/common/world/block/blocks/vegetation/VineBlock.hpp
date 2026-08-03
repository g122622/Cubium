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
#include "../../Material.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 藤蔓方块
 *
 * 可以附着在墙面上的攀爬植物。
 * 玩家可以攀爬藤蔓。
 *
 * 状态属性：
 * - UP: 是否向上延伸
 * - NORTH/SOUTH/EAST/WEST: 各方向是否附着
 */
class VineBlock : public Block {
public:
    explicit VineBlock(const BlockProperties& properties);
    ~VineBlock() override = default;

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

    // ========== 旋转 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    // ========== 攀爬 ==========

    /**
     * @brief 检查方块是否可攀爬
     *
     * 藤蔓始终可攀爬。
     *
     * @return 始终返回 true
     */
    [[nodiscard]] bool isLadder(const BlockState& state,
        IWorld* world = nullptr,
        const BlockPos* pos = nullptr,
        const Entity* entity = nullptr) const override
    {
        MC_UNUSED(world);
        MC_UNUSED(pos);
        MC_UNUSED(entity);
        MC_UNUSED(state);
        return true;
    }

    // ========== 渲染属性 ==========

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

private:
    /**
     * @brief 检查是否可以附着到指定方向的方块
     */
    [[nodiscard]] bool _canAttachTo(IBlockReader& world, const BlockPos& pos, Direction direction) const;

    /**
     * @brief 获取藤蔓连接数
     */
    [[nodiscard]] i32 _getConnectionCount(const BlockState& state) const;

    /**
     * @brief 检查周围藤蔓密度是否允许蔓延
     *
     * 在 9x3x9 范围内藤蔓数量不能超过 5 个。
     *
     * @param world 世界读取器
     * @param pos 当前位置
     * @return 如果有空间蔓延返回 true
     */
    [[nodiscard]] bool _hasRoomToSpread(IBlockReader& world, const BlockPos& pos) const;

    /**
     * @brief 检查状态是否有水平连接
     */
    [[nodiscard]] bool _hasHorizontalConnection(const BlockState& state) const;

    /**
     * @brief 随机复制水平连接
     *
     * 从源状态随机复制水平连接到目标状态。
     */
    [[nodiscard]] BlockState _copyRandomHorizontalConnections(
        const BlockState& source, const BlockState& target, math::IRandom& random) const;

    /**
     * @brief 获取方向对应的属性
     */
    [[nodiscard]] const BooleanProperty* _getPropertyFor(Direction direction) const;

    /// 各方向的形状
    CollisionShape m_northShape;
    CollisionShape m_southShape;
    CollisionShape m_eastShape;
    CollisionShape m_westShape;
};

} // namespace blocks
} // namespace mc
