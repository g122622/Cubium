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
#include "common/util/property/Properties.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/IGrowable.hpp"

namespace mc {

// 前向声明
class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 绯红/诡异菌岩方块
 *
 * 下界的可蔓延岩类方块，在光照过高时会退化为下界岩。
 * 骨粉可以在其上方生成下界植物（菌类、菌索等）。
 *
 * MC ID: minecraft:crimson_nylium, minecraft:warped_nylium
 */
class NyliumBlock : public Block, public IGrowable {
public:
    explicit NyliumBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 在光照过亮时退化为下界岩。
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override { return true; }

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查菌岩上方是否可以使用骨粉
     *
     * 菌岩上方需要有空气才能使用骨粉。
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 菌岩骨粉总是有效
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 在菌岩上方生成下界植物，使用与 MC 原版 NetherForestVegetationFeature 一致的散布算法。
     *
     * 绯红菌岩：
     *   - 执行 CRIMSON_FOREST_VEGETATION_BONEMEAL（9次散布，加权选择）：
     *     绯红菌索 87/99, 绯红菌 11/99, 诡异菌 1/99
     *
     * 诡异菌岩：
     *   - 执行 WARPED_FOREST_VEGETATION_BONEMEAL（9次散布，加权选择）：
     *     诡异菌索 85/100, 诡异菌 13/100, 绯红菌索 1/100, 绯红菌 1/100
     *   - 执行 NETHER_SPROUTS_BONEMEAL（9次散布，固定下界苗）
     *   - 1/8 概率执行 TWISTING_VINES_BONEMEAL（9次散布，1~4格高缠怨藤柱）
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 菌岩为邻居传播型骨粉类型
     *
     * 粒子在方块上方水平扩散，数量为传入值的3倍。
     */
    [[nodiscard]] BoneMealType getBoneMealType() const override { return BoneMealType::NEIGHBOR_SPREADER; }

private:
    /**
     * @brief 下界植被类型，用于加权随机选择
     */
    enum class NetherVegetationType : u8 {
        Crimson, ///< 绯红菌岩植被
        Warped   ///< 诡异菌岩植被
    };

    /**
     * @brief 散布放置下界植被（菌索、菌类）
     *
     * 对应 MC NetherForestVegetationFeature。
     * 在 origin 附近 3×1×3 范围内进行 9 次散布尝试，
     * 每次尝试根据植被类型加权随机选择放置的方块。
     *
     * @param world 世界引用
     * @param random 随机数生成器
     * @param origin 散布原点（菌岩上方一格）
     * @param type 植被类型（绯红/诡异）
     */
    static void _placeNetherVegetation(
        IWorld& world, math::IRandom& random, const BlockPos& origin, NetherVegetationType type);

    /**
     * @brief 散布放置下界苗
     *
     * 对应 MC NETHER_SPROUTS_BONEMEAL。
     * 在 origin 附近 3×1×3 范围内进行 9 次散布尝试，固定放置下界苗。
     *
     * @param world 世界引用
     * @param random 随机数生成器
     * @param origin 散布原点（菌岩上方一格）
     */
    static void _placeNetherSprouts(IWorld& world, math::IRandom& random, const BlockPos& origin);

    /**
     * @brief 散布放置缠怨藤
     *
     * 对应 MC TWISTING_VINES_BONEMEAL (TwistingVinesFeature)。
     * 在 origin 附近 3×1×3 范围内进行 9 次散布尝试，
     * 每次尝试在合适位置放置 1~4 格高的缠怨藤柱。
     * 地面方块需为下界岩、诡异菌岩或诡异疣块。
     *
     * @param world 世界引用
     * @param random 随机数生成器
     * @param origin 散布原点（菌岩上方一格）
     */
    static void _placeTwistingVines(IWorld& world, math::IRandom& random, const BlockPos& origin);

    /**
     * @brief 检查位置是否足够暗
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 方块状态
     * @return true 如果足够暗（不会退化）
     */
    [[nodiscard]] static bool _isDarkEnough(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
