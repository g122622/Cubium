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
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/base/family/WoodType.hpp"
#include "common/world/block/blocks/sign/AbstractSignBlock.hpp"
#include "common/world/block/core/Block.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
class BlockPos;
class BlockEntity;
class Player;

namespace blocks {

/**
 * @brief 站立告示牌
 *
 * 放置在地面上的告示牌，有16个旋转方向（每22.5度）。
 *
 * 状态属性：
 * - ROTATION: 0-15，表示16个旋转方向
 * - WATERLOGGED: 是否含水
 *
 */
class StandingSignBlock : public AbstractSignBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     * @param woodType 木材类型
     */
    StandingSignBlock(const BlockProperties& properties, const WoodType& woodType);

    /**
     * @brief 析构函数
     */
    ~StandingSignBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 旋转和镜像 ==========

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;

    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    /// 站立告示牌碰撞形状（告示牌杆）
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
