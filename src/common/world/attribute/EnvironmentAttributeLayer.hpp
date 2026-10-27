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
#include "common/util/math/Vector3.hpp"

#include <functional>
#include <utility>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 环境属性层基类
 *
 * 类型擦除基类，用于在 Timeline 等容器中存储异构的属性层。
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
 * 定义属性层如何被应用，有三个子接口：Constant / TimeBased / Positional。
 *
 * 三个子接口是同一模板内的嵌套类，因此各自的方法签名随 Value 一并实例化：
 * 采样器只需继承 `<Value>::TimeBased` 即可获得按属性值类型擦除的调用点。
 */
template <typename Value>
class EnvironmentAttributeLayer {
public:
    virtual ~EnvironmentAttributeLayer() = default;

    /**
     * @brief 常量层
     *
     * 不随时间/位置变化，直接应用常量参数。
     */
    class Constant {
    public:
        virtual ~Constant() = default;
        virtual Value applyConstant(Value value) const = 0;
    };

    /**
     * @brief 时间相关层
     *
     * 根据时间（dayTime/tick 序号）应用属性值变化。
     */
    class TimeBased {
    public:
        virtual ~TimeBased() = default;
        virtual Value applyTimeBased(Value value, i64 tick) const = 0;

        /**
         * @brief 清空层自身的 tick 缓存
         *
         * 系统每次失效 tick 缓存（每 tick 一次）时调用，保证下一次求值按新时刻重新采样。
         */
        virtual void invalidateTickCache() const = 0;
    };

    /**
     * @brief 位置相关层
     *
     * 根据查询位置应用属性值变化。pos 为世界坐标（方块坐标的浮点表示）。
     */
    class Positional {
    public:
        virtual ~Positional() = default;
        virtual Value applyPositional(Value value, const Vector3& pos) const = 0;
    };
};

/**
 * @brief 位置相关层：按位置查询并应用属性修改器
 *
 * 以函数对象承载具体逻辑，避免为每种查询来源单独定义子类。
 * 函数签名为 Value(const Value& subject, const Vector3& pos)。
 */
template <typename Value>
class FunctionalPositionalLayer final : public EnvironmentAttributeLayer<Value>::Positional {
public:
    using ApplyFn = std::function<Value(const Value&, const Vector3&)>;

    explicit FunctionalPositionalLayer(ApplyFn fn)
        : m_fn(std::move(fn))
    {}

    Value applyPositional(Value value, const Vector3& pos) const override
    {
        return m_fn(value, pos);
    }

private:
    ApplyFn m_fn;
};

/**
 * @brief 常量层：按属性修改器应用固定参数
 *
 * 与函数式位置层同理，供维度常量属性表使用。
 */
template <typename Value>
class FunctionalConstantLayer final : public EnvironmentAttributeLayer<Value>::Constant {
public:
    using ApplyFn = std::function<Value(const Value&)>;

    explicit FunctionalConstantLayer(ApplyFn fn)
        : m_fn(std::move(fn))
    {}

    Value applyConstant(Value value) const override
    {
        return m_fn(value);
    }

private:
    ApplyFn m_fn;
};

} // namespace attribute
} // namespace world
} // namespace mc
