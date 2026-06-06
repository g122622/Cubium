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
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE ON AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "../../Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 孢子花方块
 *
 * 一种悬挂在天花板上的装饰性植物，会向下滴落孢子粒子。
 * 只能放置在天花板下方（上方必须有坚固面的方块）。
 * 客户端会生成两种粒子效果：
 * - falling_spore_blossom: 从花正下方掉落的粒子
 * - spore_blossom_air: 在花周围21x10x21区域内漂浮的粒子
 *
 * 参考: net.minecraft.block.SporeBlossomBlock
 */
class SporeBlossomBlock : public Block {
public:
    explicit SporeBlossomBlock(const BlockProperties& properties);

    ~SporeBlossomBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

    [[nodiscard]] bool useShapeForLightOcclusion(const BlockState& state) const override
    {
        MC_UNUSED(state);
        return true;
    }

    /**
     * @brief 检查是否可以放置在指定位置
     *
     * 孢子花只能放置在上方有坚固面的方块下方
     */
    [[nodiscard]] bool isValidPosition(
        const BlockState& state, IBlockReader& world, const BlockPos& pos) const override;

    /**
     * @brief 客户端方块动画 tick
     *
     * 在客户端每 tick 调用，用于生成孢子花粒子效果。
     * 生成两种粒子：
     * - falling_spore_blossom: 从花正下方掉落的绿色孢子粒子
     * - spore_blossom_air: 在花周围漂浮的环境粒子
     *
     * 注意：此方法需要由客户端 animateTick 系统调用，
     * 当前该系统尚未实现，因此粒子暂不会自动生成。
     *
     * @param world 世界实例
     * @param pos 方块位置
     * @param state 方块状态
     * @param random 随机数生成器
     */
    void animateTick(IWorld& world, const BlockPos& pos, const BlockState& state, math::IRandom& random);

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
