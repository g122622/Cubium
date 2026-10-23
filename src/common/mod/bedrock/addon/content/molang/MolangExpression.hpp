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

#include "common/core/Result.hpp"
#include "common/mod/bedrock/addon/content/molang/base/MolangTypes.hpp"
#include <memory>
#include <string_view>

namespace mc::mod::bedrock::addon {
namespace detail {
struct MolangProgram;
}

/** @brief 编译后可跨实体复用的 Molang 表达式门面。 */
class MolangExpression {
public:
    /** @brief 校验并编译表达式；未知查询、函数或语法明确返回错误。 */
    [[nodiscard]] static Result<MolangExpression> compile(std::string_view source);
    /** @brief 使用本次调用提供的属性和随机数入口求值。 */
    [[nodiscard]] Result<MolangValue> evaluate(MolangEvaluationContext& context) const;

    /** @brief 默认值等受限求值位置可据此禁止实体属性查询。 */
    [[nodiscard]] bool usesPropertyQueries() const;

private:
    /** @brief 仅由编译入口接收完整有效的程序。 */
    explicit MolangExpression(std::shared_ptr<const detail::MolangProgram> program);
    std::shared_ptr<const detail::MolangProgram> m_program;
};

} // namespace mc::mod::bedrock::addon
