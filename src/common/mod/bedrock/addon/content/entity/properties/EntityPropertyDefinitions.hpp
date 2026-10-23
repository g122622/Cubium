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
#include "common/mod/bedrock/addon/content/molang/MolangExpression.hpp"
#include <map>
#include <nlohmann/json.hpp>

namespace mc::mod::bedrock::addon {
/** @brief 属性声明及逐实例计算的默认值表达式。 */
class EntityPropertyDefinitions {
public:
    /** @brief 编译默认值表达式并校验属性声明，不在加载阶段固化随机结果。 */
    [[nodiscard]] static Result<EntityPropertyDefinitions> compile(const nlohmann::json& definitions);
    /** @brief 返回用于静态事件验证的声明，表达式默认值使用合法类型占位。 */
    [[nodiscard]] const nlohmann::json& literalSchema() const;
    /** @brief 使用实例随机源计算实际默认值，成功后返回完整且已验证的声明。 */
    [[nodiscard]] Result<nlohmann::json> instantiate(mc::math::IRandom& random) const;

private:
    nlohmann::json m_literalSchema;
    std::map<std::string, MolangExpression> m_defaults;
};
} // namespace mc::mod::bedrock::addon
