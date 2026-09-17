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

#include "common/world/attribute/EnvironmentAttribute.hpp"
#include "common/world/attribute/LerpFunction.hpp"

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 属性修改器
 *
 * MC 1.21.11 interface AttributeModifier<Subject, Argument>。
 * 定义如何将关键帧采样得到的 argument 应用到基础值 subject 上。
 */
template <typename Subject, typename Argument>
class AttributeModifier {
public:
    virtual ~AttributeModifier() = default;

    virtual Subject apply(Subject subject, Argument argument) = 0;

    /**
     * @brief 覆写修改器
     *
     * 直接返回 argument（忽略 subject）。
     * MC 中 OverrideModifier.apply(value, value1) 返回 value1。
     */
    class Override {
    public:
        Subject apply(Subject, Argument argument) { return argument; }
    };
};

} // namespace attribute
} // namespace world
} // namespace mc
