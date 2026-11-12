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

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/blocks/dirt/SnowyDirtBlock.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/core/Block.hpp"

#include <functional>

namespace mc {

class IWorld;
class BlockPos;
class BlockState;

namespace math {
class IRandom;
}

namespace blocks {

/**
 * @brief 可蔓延的雪覆盖泥土方块基类
 *
 * 这是草方块和菌丝的基类，在 SnowyDirtBlock（持有 SNOWY 属性、放置/邻居更新同步雪状态）
 * 之上额外提供蔓延和退化机制：
 * - 当光照不足时退化成泥土
 * - 当光照足够时向周围泥土蔓延
 */
class SpreadableSnowyDirtBlock : public SnowyDirtBlock {
public:
    explicit SpreadableSnowyDirtBlock(BlockProperties properties);

    /**
     * @brief 随机刻
     *
     * 处理蔓延和退化逻辑：
     * - 光照不足时退化成泥土
     * - 光照充足时向周围泥土蔓延
     */
    void randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) override;

    /**
     * @brief 是否响应随机刻
     */
    [[nodiscard]] bool ticksRandomly() const noexcept override
    {
        return true;
    }

protected:
    /**
     * @brief 检查是否为雪覆盖条件
     *
     * 检查上方是否有雪（仅1层）或者光照条件是否满足。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件
     */
    [[nodiscard]] static bool isSnowyConditions(IWorld& world, const BlockPos& pos, const BlockState& state);

    /**
     * @brief 检查是否为雪覆盖且非水下条件
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return true 如果满足蔓延条件且不在水下
     */
    [[nodiscard]] static bool isSnowyAndNotUnderwater(IWorld& world, const BlockPos& pos, const BlockState& state);
};

} // namespace blocks
} // namespace mc
