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
#include "../../IGrowable.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/PlantType.hpp"

namespace mc {

class IWorld;
class IBlockReader;
class BlockItemUseContext;
namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 海草方块
 *
 * 水下植物，可以放置在水下地面上。
 * 可以通过骨粉催熟变成高海草。
 *
 * ## 特性
 * - 单格水下植物
 * - 需要固体支撑
 * - 必须放置在水源方块中（流体等级=8）
 * - 可用骨粉催熟变成高海草
 * - 实现 IGrowable 接口
 */
class SeagrassBlock : public Block, public IGrowable, public IPlantable {
public:
    explicit SeagrassBlock(const BlockProperties& properties);
    ~SeagrassBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    // ========== 形状 ==========

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] const CollisionShape& getCollisionShape(const BlockState& state) const override;

    [[nodiscard]] bool isOpaque(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return false;
    }

    // ========== IGrowable 接口 ==========

    /**
     * @brief 检查海草是否可以生长
     *
     * 海草可以生长的条件：
     * 1. 上方是水源方块
     *
     * @param world 世界读取器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param isClientSide 是否为客户端
     * @return 如果上方有水源则返回true
     */
    [[nodiscard]] bool canGrow(
        IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const override;

    /**
     * @brief 检查是否可以使用骨粉
     *
     * 海草使用骨粉有概率催熟成高海草。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return 总是返回true（骨粉总是有效）
     */
    [[nodiscard]] bool canUseBonemeal(
        IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const override;

    /**
     * @brief 使用骨粉生长
     *
     * 将海草变成高海草。
     *
     * @param world 世界
     * @param random 随机数生成器
     * @param pos 方块位置
     * @param state 当前方块状态
     */
    void grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) override;

    // ========== IPlantable 接口 ==========

    [[nodiscard]] PlantType getPlantType(IBlockReader& world, const BlockPos& pos) const override;
    [[nodiscard]] const BlockState& getPlant(IBlockReader& world, const BlockPos& pos) const override;

    // ========== 流体状态 ==========

    /**
     * @brief 获取流体状态
     *
     * 海草始终返回静止水的流体状态。
     *
     * @param state 方块状态
     * @return 水流体状态指针
     */
    [[nodiscard]] const fluid::FluidState* getFluidState(const BlockState& state) const override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
