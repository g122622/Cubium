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

#include "TreeDecorator.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

/**
 * @brief 树干藤蔓装饰器（MC TrunkVineDecorator）
 *
 * 对每根原木，在 west/east/north/south 四个水平方向各以 2/3 概率（nextInt(3)>0）
 * 放置藤蔓：该方向邻居为空气时，放默认藤蔓并把对应朝向属性置 true
 * （west→EAST、east→WEST、north→SOUTH、south→NORTH，对齐 MC VineBlock 朝向语义：
 *藤蔓贴在该方向相反的面上）。
 */
class TrunkVineDecorator final : public TreeDecorator {
public:
    TrunkVineDecorator() = default;

    void place(const TreeDecoratorContext& context) const override;
};

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
