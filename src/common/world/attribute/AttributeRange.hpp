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

#include <algorithm>
#include <functional>

namespace mc {
namespace world {
namespace attribute {

/**
 * @brief 属性值范围
 *
 * 用于验证和规范化环境属性的值。
 */
template <typename Value>
class AttributeRange {
public:
    AttributeRange() = default;

    AttributeRange(std::function<bool(Value)> validateFn,
        std::function<Value(Value)> sanitizeFn)
        : m_validate(std::move(validateFn))
        , m_sanitize(std::move(sanitizeFn))
    {}

    bool validate(Value value) const { return m_validate(value); }
    Value sanitize(Value value) const { return m_sanitize(value); }

    static AttributeRange<Value> any()
    {
        return AttributeRange<Value>(
            [](Value) { return true; },
            [](Value v) { return v; });
    }

    static AttributeRange<float> ofFloat(float min, float max)
    {
        return AttributeRange<float>(
            [min, max](float v) { return v >= min && v <= max; },
            [min, max](float v) { return std::clamp(v, min, max); });
    }

private:
    std::function<bool(Value)> m_validate;
    std::function<Value(Value)> m_sanitize;
};

} // namespace attribute
} // namespace world
} // namespace mc
