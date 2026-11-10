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
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace mc::math {
class IRandom;
}

namespace mc::mod::bedrock::addon {

/** @brief Molang 数值使用单精度，字符串供属性比较和枚举结果使用。 */
using MolangValue = std::variant<f32, std::string>;

/**
 * @brief 每次求值的宿主访问入口，随机数与查询读取均由调用方明确提供。
 *
 * 具名查询与动态属性分成两个入口：前者按完整查询名（如 `query.is_baby`）取值，
 * 后者按属性名（如 `test:count`）取值，两者键空间不同，不能合并成一个回调。
 */
struct MolangEvaluationContext {
    mc::math::IRandom& random;
    /// 具名零参查询的解析入口；返回 std::nullopt 表示该查询尚未实现（求值按 0 处理）。
    std::function<std::optional<MolangValue>(std::string_view)> query;
    /// 动态属性读取入口，参数是 `query.property('ns:name')` 里的属性名本身。
    std::function<std::optional<MolangValue>(std::string_view)> property;
};

} // namespace mc::mod::bedrock::addon
