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

#include "AzaleaBlock.hpp"

namespace mc {
namespace blocks {

AzaleaBlock::AzaleaBlock(const BlockProperties& properties)
    : Block(properties)
{
    // TODO: 正确的杜鹃花丛碰撞箱应该是组合形状
    // 目前使用完整方块作为占位符
    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16);
}

const CollisionShape& AzaleaBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

FloweringAzaleaBlock::FloweringAzaleaBlock(const BlockProperties& properties)
    : Block(properties)
{
    // TODO: 正确的开花杜鹃花丛碰撞箱应该是组合形状
    // 目前使用完整方块作为占位符
    m_shape = CollisionShape::fromPixelBox(0, 0, 0, 16, 16, 16);
}

const CollisionShape& FloweringAzaleaBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
