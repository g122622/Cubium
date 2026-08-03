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
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include <array>
#include <optional>

namespace mc {

class IWorld;
class IBlockReader;
class WorldGenRegion;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 紫颂花方块
 *
 * 紫颂植物的顶部，可以生长。紫颂花会随着生长阶段改变外观，
 * 当达到最大年龄时会死亡。
 *
 * 状态属性：
 * - AGE_0_5: 生长阶段 (0-5)
 */
class ChorusFlowerBlock : public Block {
public:
    explicit ChorusFlowerBlock(const BlockProperties& properties);
    ~ChorusFlowerBlock() noexcept override = default;

    // ========== 状态属性 ==========

    [[nodiscard]] i32 getAge(const BlockState& state) const;
    [[nodiscard]] BlockState withAge(i32 age) const;
    [[nodiscard]] i32 getMaxAge() const { return 5; }

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 生长逻辑 ==========

    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== 世界生成 ==========

    /**
     * @brief 在指定位置生成一棵完整的紫颂树
     *
     * 在指定位置放置紫颂植物茎干，然后递归生长紫颂树。
     *
     * @param world 世界区域（用于方块查询和设置）
     * @param pos 起始位置（紫颂植物茎干底部）
     * @param random 随机数生成器
     * @param maxHorizontalDistance 最大水平扩展距离（原版为8）
     */
    static void generatePlant(
        WorldGenRegion& world, const BlockPos& pos, math::Random& random, i32 maxHorizontalDistance);

private:
    std::array<CollisionShape, 6> m_shapesByAge;

    /**
     * @brief 递归生长紫颂树
     *
     * 从指定位置向上生长茎干，然后在水平方向分枝。
     *
     * @param world 世界区域
     * @param pos 当前递归起点位置
     * @param random 随机数生成器
     * @param origin 紫颂树原点位置（用于限制水平扩展范围）
     * @param maxHorizontalDistance 最大水平扩展距离
     * @param depth 当前递归深度（0-4）
     */
    static void growTreeRecursive(WorldGenRegion& world,
        const BlockPos& pos,
        math::Random& random,
        const BlockPos& origin,
        i32 maxHorizontalDistance,
        i32 depth);

    /**
     * @brief 检查指定位置的所有邻居（除了排除方向）是否都为空气
     *
     * 检查指定位置的四个水平方向邻居（排除指定方向）是否都为空气。
     * @param world 世界区域
     * @param pos 要检查的位置
     * @param excludeDir 排除的方向（传空表示不排除）
     * @return true 如果所有未排除的水平邻居都为空气
     */
    static bool allNeighborsEmpty(
        WorldGenRegion& world, const BlockPos& pos, const std::optional<Direction>& excludeDir);
};

} // namespace blocks
} // namespace mc
