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
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/feature/state/BlockStateProvider.hpp"
#include <memory>

namespace mc::world::gen::feature::state {

/**
 * @brief 旋转方块提供者
 *
 * 取方块的默认状态并随机选取一个轴（X/Y/Z）设置 AXIS 属性；
 * 若该方块无 AXIS 属性则原样返回默认状态。解析时仅保留 Block，丢弃 Properties。
 */
class RotatedBlockStateProvider : public BlockStateProvider {
public:
    explicit RotatedBlockStateProvider(const Block* block);

    [[nodiscard]] const BlockState* getState(
        const IWorld& world, math::IRandom& random, i32 x, i32 y, i32 z) const override;

    [[nodiscard]] std::unique_ptr<BlockStateProvider> clone() const override;

private:
    const Block* m_block;
};

} // namespace mc::world::gen::feature::state
