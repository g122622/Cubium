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
 * @brief 下界岩方块
 *
 * 下界岩是下界的基础岩石。对下界岩使用骨粉时，若其上方透光且周围 3×3×3
 * 范围内存在菌岩（绯红菌岩或诡异菌岩），则下界岩会转化为对应类型的菌岩：
 *   - 周围仅有绯红菌岩 → 转化为绯红菌岩
 *   - 周围仅有诡异菌岩 → 转化为诡异菌岩
 *   - 周围两种菌岩都有 → 随机转化为其中一种
 *
 * 骨粉类型为 NEIGHBOR_SPREADER（邻居传播型），骨粉作用于下界岩本身。
 *
 * Ref: net.minecraft.world.level.block.NetherrackBlock（1.21.11）
 *      isValidBonemealTarget: 上方传播天空光 + 周围 3×3×3 有菌岩
 *      performBonemeal: 根据周围菌岩类型转化下界岩
 *
 * MC ID: minecraft:netherrack
 */
class NetherrackBlock : public Block, public IGrowable {
public:
    explicit NetherrackBlock(BlockProperties properties);

    // ========== IGrowable 接口实现 ==========

    /**
     * @brief 检查下界岩是否可以使用骨粉
     *
     * 条件（对齐 MC NetherrackBlock.isValidBonemealTarget）：
     *   1. 上方方块传播天空光（即上方不阻挡天空光，如空气）
     *   2. 周围 3×3×3 范围内存在菌岩（crimson_nylium / warped_nylium）
     *
     * @param world 世界读取器
     * @param pos 下界岩位置
     * @param state 当前方块状态
     * @param isClientSide 是否为客户端
     * @return 如果可以使用骨粉返回 true
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 下界岩骨粉总是有效（只要 canGrow 返回 true）
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 遍历下界岩周围 3×3×3 范围，统计绯红菌岩和诡异菌岩的存在情况，
     * 然后根据统计结果将下界岩转化为对应菌岩：
     *   - 两种菌岩都有 → 随机选一种
     *   - 仅有诡异菌岩 → 转化为诡异菌岩
     *   - 仅有绯红菌岩 → 转化为绯红菌岩
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 下界岩位置
     * @param state 当前方块状态
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    /**
     * @brief 下界岩为邻居传播型骨粉类型
     *
     * 粒子在方块上方水平扩散，数量为传入值的3倍。
     */
    [[nodiscard]] BoneMealType getBoneMealType() const override { return BoneMealType::NEIGHBOR_SPREADER; }
};

} // namespace blocks
} // namespace mc
