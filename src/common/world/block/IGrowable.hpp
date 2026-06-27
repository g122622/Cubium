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
#include "common/world/block/BlockPos.hpp"

namespace mc {

// Forward declarations
class IBlockReader;
class IWorld;
class BlockState;
namespace math {
class IRandom;
}

/**
 * @brief 可生长接口
 *
 * 实现此接口的方块可以被骨粉催熟。
 * 包括农作物、树苗、花朵、草等植物。
 *
 * 参考: net.minecraft.world.level.block.BonemealableBlock
 */
class IGrowable {
public:
    /**
     * @brief 骨粉类型枚举
     *
     * 决定骨粉粒子效果的分布方式：
     * - NEIGHBOR_SPREADER: 邻居传播型（草方块、菌岩、下界岩），
     *   粒子在方块上方水平扩散 3 倍数量
     * - GROWER: 自身成长型（默认，作物、树苗等），
     *   粒子在方块内部按形状高度生成
     *
     * 参考: net.minecraft.world.level.block.BonemealableBlock.Type
     */
    enum class BoneMealType {
        GROWER,           ///< 自身成长型（默认）
        NEIGHBOR_SPREADER ///< 邻居传播型
    };

    virtual ~IGrowable() = default;

    /**
     * @brief 检查是否可以生长
     *
     * @param world 世界读取器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param isClientSide 是否为客户端
     * @return 如果可以生长返回true
     */
    [[nodiscard]] virtual bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const = 0;

    /**
     * @brief 检查是否可以使用骨粉
     *
     * 对于某些方块（如草、花），骨粉有概率成功；
     * 对于农作物，骨粉必定有效（如果未成熟）。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 如果骨粉有效返回true
     */
    [[nodiscard]] virtual bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const = 0;

    /**
     * @brief 使用骨粉生长
     *
     * 当玩家对方块使用骨粉时调用。
     * 应该在服务端调用，因为会修改世界状态。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    virtual void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) = 0;

    /**
     * @brief 获取骨粉类型
     *
     * 决定骨粉粒子的分布方式。默认为 GROWER。
     * NEIGHBOR_SPREADER 类型的方块（草方块、菌岩、下界岩等）
     * 应重写此方法返回 NEIGHBOR_SPREADER。
     *
     * 参考: net.minecraft.world.level.block.BonemealableBlock.getType()
     *
     * @return 骨粉类型
     */
    [[nodiscard]] virtual BoneMealType getBoneMealType() const { return BoneMealType::GROWER; }

    /**
     * @brief 获取骨粉粒子生成位置
     *
     * NEIGHBOR_SPREADER 类型返回方块上方一格位置（pos.above()），
     * GROWER 类型返回方块自身位置。
     *
     * 参考: net.minecraft.world.level.block.BonemealableBlock.getParticlePos()
     *
     * @param pos 方块位置
     * @return 粒子生成位置
     */
    [[nodiscard]] virtual BlockPos getParticlePos(const BlockPos& pos) const
    {
        switch (getBoneMealType()) {
            case BoneMealType::NEIGHBOR_SPREADER:
                return BlockPos(pos.x, pos.y + 1, pos.z);
            case BoneMealType::GROWER:
            default:
                return pos;
        }
    }
};

} // namespace mc
