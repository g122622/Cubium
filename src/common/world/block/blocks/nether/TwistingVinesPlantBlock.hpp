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

#include "common/world/block/blocks/growing_plant/GrowingPlantBodyBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 扭曲藤植株（茎部）
 *
 * 对齐 vanilla `TwistingVinesPlantBlock`（1.21.11）：继承 `GrowingPlantBodyBlock`，
 * 向上生长、无 AGE 属性、不独立生长。形状是 `Block.column(8, 0, 16)` 细柱，
 * 不是整格——因此天空光可穿过（lightBlock=0）。
 *
 * 早期实现直接用 `SimpleBlock` 注册，形状退回整格立方体，导致该方块被误判为
 * 完全挡光（lightBlock=1），且失去「头部断裂时茎段变回头部」的行为。
 */
class TwistingVinesPlantBlock : public GrowingPlantBodyBlock {
public:
    explicit TwistingVinesPlantBlock(const BlockProperties& properties);
    ~TwistingVinesPlantBlock() override = default;

    [[nodiscard]] const Block* getHeadBlock() const override;
    [[nodiscard]] const Block* getBodyBlock() const override;
};

} // namespace blocks
} // namespace mc
