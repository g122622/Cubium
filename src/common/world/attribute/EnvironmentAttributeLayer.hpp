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

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 环境属性层基类
 *
 * 类型擦除基类，用于在 Timeline 中存储异构的属性层。
 * MC 使用 sealed interface，C++ 通过虚函数实现等价语义。
 */
class EnvironmentAttributeLayerBase {
public:
    virtual ~EnvironmentAttributeLayerBase() = default;
};

/**
 * @brief 环境属性层接口
 *
 * MC 1.21.11 sealed interface EnvironmentAttributeLayer<Value>。
 * 定义属性层如何被应用，有三个子接口：Positional / TimeBased / Constant。
 */
template <typename Value>
class EnvironmentAttributeLayer {
public:
    virtual ~EnvironmentAttributeLayer() = default;

    /**
     * @brief 时间相关层
     *
     * 根据时间（dayTime）应用属性值变化。
     */
    class TimeBased {
    public:
        virtual ~TimeBased() = default;
        virtual Value applyTimeBased(Value value, i64 tick) = 0;
    };

    /**
     * @brief 常量层
     *
     * 不随时间变化，直接应用常量值。
     */
    class Constant {
    public:
        virtual ~Constant() = default;
        virtual Value applyConstant(Value value) = 0;
    };

    // TODO: Positional 子接口（空间插值）暂未实现，按需补充
};

} // namespace attribute
} // namespace world
} // namespace mc
