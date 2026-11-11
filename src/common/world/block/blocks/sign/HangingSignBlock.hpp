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
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/blocks/sign/SignBlock.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/state/BlockState.hpp"
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
 */
class CeilingHangingSignBlock : public AbstractSignBlock {
public:
    CeilingHangingSignBlock(const BlockProperties& properties, const WoodType& woodType);
    ~CeilingHangingSignBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;
    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 支撑形状**直接返回 getShape**，绕过 hasCollision 判定
     *
     * 对齐原版 `WallHangingSignBlock` / `CeilingHangingSignBlock` 的覆写：
     *
     * ```java
     * protected VoxelShape getBlockSupportShape(BlockState state, BlockGetter level, BlockPos pos) {
     *     return this.getShape(state, level, pos, CollisionContext.empty());
     * }
     * ```
     *
     * 悬挂告示牌的 `hasCollision` 是 false（玩家可以穿过），若走基类默认实现
     * `getCollisionShape` 就会得到空形状，于是 `SupportType.CENTER` 在所有面上都判 false，
     * 告示牌无法承托任何附着物。原版显式覆写正是为了绕开这一点。
     * 由差分能力 face_sturdy 查出。
     */
    [[nodiscard]] const CollisionShape& getBlockSupportShape(const BlockState& state) const override
    {
        return getShape(state);
    }

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 悬挂告示牌交互失败时播放悬挂告示牌专属音效
     */
    [[nodiscard]] const ResourceLocation& getWaxedInteractFailSound() const override;

private:
    /**
     * @brief 按 `rotation`（0..15）索引的形状。
     *
     * 对齐原版 `CeilingHangingSignBlock`：
     *
     * ```java
     * SHAPE_DEFAULT = Block.column(10.0, 0.0, 16.0);          // box(3,0,3,13,16,13)
     * SHAPES        = rotateHorizontal(Block.column(14.0, 2.0, 0.0, 10.0))
     *                     .entrySet().stream()
     *                     .collect(toMap(RotationSegment::convertToSegment, Entry::getValue));
     * getShape(...) { return SHAPES.getOrDefault(state.getValue(ROTATION), SHAPE_DEFAULT); }
     * ```
     *
     * `rotateHorizontal` 只给出 8 个「旋转段」（每个 22.5°），映射到 `rotation` 值后
     * 只有 0/8 与 4/12 落在其中，其余 12 个 rotation 走 `SHAPE_DEFAULT`。
     *
     * 实测原版三种结果：
     *   rotation 0, 8   -> box(1,0,7,15,10,9)    长边沿 X，Y 只到 10
     *   rotation 4, 12  -> box(7,0,1,9,10,15)    长边沿 Z，Y 只到 10
     *   其余 12 个       -> box(3,0,3,13,16,13)  SHAPE_DEFAULT，Y 到 16
     */
    std::unordered_map<i32, CollisionShape> m_shapesByRotation;
    /// `SHAPE_DEFAULT`：rotation 不在 `m_shapesByRotation` 中时使用。
    CollisionShape m_defaultShape;
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
 */
class WallHangingSignBlock : public AbstractSignBlock {
public:
    WallHangingSignBlock(const BlockProperties& properties, const WoodType& woodType);
    ~WallHangingSignBlock() override = default;

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] const BlockState& rotate(const BlockState& state, Rotation rotation) const override;
    [[nodiscard]] const BlockState& mirror(const BlockState& state, Mirror mirror) const override;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    /**
     * @brief 支撑形状**直接返回 getShape**，绕过 hasCollision 判定
     *
     * 对齐原版 `WallHangingSignBlock` / `CeilingHangingSignBlock` 的覆写：
     *
     * ```java
     * protected VoxelShape getBlockSupportShape(BlockState state, BlockGetter level, BlockPos pos) {
     *     return this.getShape(state, level, pos, CollisionContext.empty());
     * }
     * ```
     *
     * 悬挂告示牌的 `hasCollision` 是 false（玩家可以穿过），若走基类默认实现
     * `getCollisionShape` 就会得到空形状，于是 `SupportType.CENTER` 在所有面上都判 false，
     * 告示牌无法承托任何附着物。原版显式覆写正是为了绕开这一点。
     * 由差分能力 face_sturdy 查出。
     */
    [[nodiscard]] const CollisionShape& getBlockSupportShape(const BlockState& state) const override
    {
        return getShape(state);
    }

    /**
     * @brief 碰撞形状只取顶部横板，不含中心立柱。
     *
     * 原版 `SHAPES_PLANK.get(FACING.getAxis())`——玩家能穿过立柱但撞得到横板。
     */
    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

protected:
    void fillStateContainer(StateContainer<Block, BlockState>& container) override;

    /**
     * @brief 悬挂告示牌交互失败时播放悬挂告示牌专属音效
     */
    [[nodiscard]] const ResourceLocation& getWaxedInteractFailSound() const override;

private:
    std::unordered_map<Direction, CollisionShape> m_shapesByDirection;

    /// 各朝向下的顶部横板形状（碰撞用，不含立柱）。
    std::unordered_map<Direction, CollisionShape> m_plankShapes;
    /// 寻路通行判定：原版 WallHangingSignBlock 显式返回 false。
    [[nodiscard]] bool isPathfindable(const BlockState& state, PathComputationType type) const override;
};

} // namespace blocks
} // namespace mc
