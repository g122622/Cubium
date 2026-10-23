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
#include <string>
#include <string_view>
#include <vector>

namespace mc::mod::bedrock::addon::detail {

/** @brief 不可变表达式节点的类别。 */
enum class MolangNodeKind { Number, String, Unary, Binary, Conditional, Call };

/** @brief 以索引连接节点，避免深层 unique_ptr 析构链。 */
struct MolangNode {
    MolangNodeKind kind;
    f32 number;
    std::string text;
    std::vector<std::size_t> inputs;
};

/** @brief 经语法及函数签名校验的表达式程序。 */
struct MolangProgram {
    std::vector<MolangNode> nodes;
    std::size_t root;
};

/** @brief 将 Molang 表达式解析为有界的语法树。 */
Result<MolangProgram> parseMolang(std::string_view source);
/** @brief 使用显式宿主上下文计算表达式程序。 */
Result<MolangValue> evaluateMolang(const MolangProgram& program, MolangEvaluationContext& context);

} // namespace mc::mod::bedrock::addon::detail
