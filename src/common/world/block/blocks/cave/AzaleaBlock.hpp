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
 * @brief 杜鹃花丛方块
 *
 * 生长在苔藓上的装饰性植物，可以种植在泥土和苔藓上。
 * 使用骨粉可以使其生长为杜鹃树。
 *
 * 参考: net.minecraft.block.AzaleaBlock
 */
class AzaleaBlock : public Block {
public:
    explicit AzaleaBlock(const BlockProperties& properties);

    ~AzaleaBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    CollisionShape m_shape;
};

/**
 * @brief 开花的杜鹃花丛方块
 *
 * 杜鹃花丛的开花变体，会开出粉红色的花朵。
 * 与普通杜鹃花丛功能相同，但具有不同的外观。
 *
 * 参考: net.minecraft.block.FloweringAzaleaBlock
 */
class FloweringAzaleaBlock : public Block {
public:
    explicit FloweringAzaleaBlock(const BlockProperties& properties);

    ~FloweringAzaleaBlock() override = default;

    [[nodiscard]] const CollisionShape& getShape(const BlockState& state) const override;

private:
    CollisionShape m_shape;
};

} // namespace blocks
} // namespace mc
