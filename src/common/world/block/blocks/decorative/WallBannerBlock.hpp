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
#include "common/util/color/DyeColor.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/blocks/decorative/AbstractBannerBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;

namespace blocks {

/**
 * @brief 墙壁旗帜
 *
 * 附着在墙面上的旗帜，有4个水平朝向。
 *
 * 状态属性：
 * - HORIZONTAL_FACING: 北、南、东、西四个方向
 *
 */
class WallBannerBlock : public AbstractBannerBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param color 旗帜底色
     */
    WallBannerBlock(const BlockProperties& properties, DyeColor color);

    ~WallBannerBlock() override = default;

    // ========== 放置逻辑 ==========

    /**
     * @brief 根据玩家视线方向选择墙面朝向
     * @return 带HORIZONTAL_FACING属性的方块状态，无法放置时返回默认状态
     */
    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    /**
     * @brief 检查背面是否有实心方块支撑
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 邻居更新 ==========

    /**
     * @brief 支撑方块被移除时旗帜变为空气
     */
    [[nodiscard]] BlockState updatePostPlacement(const BlockState& state,
        Direction facing,
        const BlockState& facingState,
        IWorld& world,
        const BlockPos& currentPos,
        const BlockPos& facingPos) override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 各方向的碰撞形状
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;
};

} // namespace blocks
} // namespace mc
