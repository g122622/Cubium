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

#include "SignBlock.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include <unordered_map>

namespace mc {
namespace blocks {

/**
 * @brief 天花板悬挂告示牌
 *
 * 从天花板悬挂的告示牌，有16个旋转方向。
 * 1.20 Trails & Tales 新增。
 *
 * 状态属性：
 * - ROTATION: 0-15，表示16个旋转方向
 * - ATTACHED: 是否连接到链条/其他挂牌
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.CeilingHangingSignBlock
 */
class CeilingHangingSignBlock : public AbstractSignBlock {
public:
    CeilingHangingSignBlock(const BlockProperties& properties, WoodType woodType);
    ~CeilingHangingSignBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 悬挂告示牌交互失败时播放悬挂告示牌专属音效
     */
    [[nodiscard]] const ResourceLocation& getWaxedInteractFailSound() const override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 墙面悬挂告示牌
 *
 * 附着在墙面上的悬挂告示牌，有4个水平朝向。
 * 1.20 Trails & Tales 新增。
 *
 * 状态属性：
 * - FACING: 北、南、东、西四个方向
 * - WATERLOGGED: 是否含水
 *
 * 参考: net.minecraft.block.WallHangingSignBlock
 */
class WallHangingSignBlock : public AbstractSignBlock {
public:
    WallHangingSignBlock(const BlockProperties& properties, WoodType woodType);
    ~WallHangingSignBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 悬挂告示牌交互失败时播放悬挂告示牌专属音效
     */
    [[nodiscard]] const ResourceLocation& getWaxedInteractFailSound() const override;

private:
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;
};

} // namespace blocks
} // namespace mc
