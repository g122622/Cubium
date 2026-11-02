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

#include "common/world/block/blocks/SimpleBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 遮光玻璃（对齐 vanilla `TintedGlassBlock`）
 *
 * 原版继承 `TransparentBlock`，但把光照行为整体改写成了「完全不透光」：
 * - `propagatesSkylightDown -> false`
 * - `getLightBlock -> 15`
 *
 * 也就是说，它虽然 noOcclusion（不参与面遮挡、不产生 AO 阴影），却会在光照传播时
 * 把整格光线全部吃掉——和普通玻璃（lightBlock=0）完全相反。
 *
 * 注册属性本身表达不了这一点：`BlockProperties::opacity()` 的 15 与默认哨兵值相同，
 * 写入后仍会落入「不透明则 15，否则按默认公式」的分支，得不到 15。因此需要重写。
 */
class TintedGlassBlock : public SimpleBlock {
public:
    explicit TintedGlassBlock(BlockProperties properties);

    ~TintedGlassBlock() override = default;

    [[nodiscard]] i32 getOpacity(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;

    [[nodiscard]] bool propagatesSkylightDown(
        const BlockState& state, IWorld* world = nullptr, const BlockPos* pos = nullptr) const override;
};

} // namespace blocks
} // namespace mc
