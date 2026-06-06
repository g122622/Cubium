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

#include "../../Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 孢子花方块
 *
 * 一种悬挂在天花板上的装饰性植物，会向下滴落孢子粒子。
 * 只能放置在天花板下方。
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

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
