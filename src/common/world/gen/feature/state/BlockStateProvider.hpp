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
#include "common/util/math/random/IRandom.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>
#include <vector>

namespace mc::world::gen::feature::state {

/**
 * @brief 方块状态提供者基类
 *
 * 用于提供方块状态，可以是固定的或基于权重的。
 */
class BlockStateProvider {
public:
    virtual ~BlockStateProvider() = default;

    /**
     * @brief 获取方块状态
     * @param random 随机数生成器
     * @param x X 坐标
     * @param y Y 坐标
     * @param z Z 坐标
     * @return 方块状态
     */
    [[nodiscard]] virtual const BlockState* getState(math::IRandom& random, i32 x, i32 y, i32 z) const = 0;
};

/**
 * @brief 固定方块状态提供者
 *
 * 始终返回同一个方块状态。
 */
class SimpleBlockStateProvider : public BlockStateProvider {
public:
    explicit SimpleBlockStateProvider(const BlockState* state);

    [[nodiscard]] const BlockState* getState(math::IRandom& random, i32 x, i32 y, i32 z) const override;

private:
    const BlockState* m_state;
};

} // namespace mc::world::gen::feature::state
