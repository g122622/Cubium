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

#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace function {

/**
 * @brief 字符串模板 - 解析 $(var) 宏变量语法
 *
 * 对应 MC 1.21.11 的 net.minecraft.commands.functions.StringTemplate。
 *
 * 将一行宏文本（去掉首字符 $ 后的内容）解析为：
 * - segments：被变量切开的纯文本片段（数量 = variables + 1）
 * - variables：变量名列表（不含 $()）
 *
 * 解析规则：
 * - 扫描 $ 字符，必须紧跟 ( 才视为变量起始
 * - 找匹配的 ) 提取变量名，变量名只允许 [A-Za-z0-9_]
 * - 至少包含一个变量，否则抛异常
 *
 * substitute(values) 用实参字符串列表填充模板，生成最终命令字符串。
 */
class StringTemplate {
public:
    /**
     * @brief 从字符串解析模板
     *
     * 解析 $(var) 语法，提取变量名和文本片段。
     *
     * @param input 宏文本（去掉首字符 $ 后的内容）
     * @return 解析后的 StringTemplate
     * @throws std::invalid_argument 如果变量名非法、变量未闭合或无变量
     */
    [[nodiscard]] static StringTemplate fromString(const std::string& input);

    /**
     * @brief 检查变量名是否合法（仅允许字母、数字、下划线）
     */
    [[nodiscard]] static bool isValidVariableName(const std::string& name) noexcept;

    StringTemplate() = default;

    /**
     * @brief 构造模板
     * @param segments 文本片段列表（数量 = variables + 1）
     * @param variables 变量名列表
     */
    StringTemplate(std::vector<std::string> segments, std::vector<std::string> variables)
        : m_segments(std::move(segments))
        , m_variables(std::move(variables))
    {}

    /** @brief 文本片段列表 */
    [[nodiscard]] const std::vector<std::string>& segments() const noexcept { return m_segments; }

    /** @brief 变量名列表 */
    [[nodiscard]] const std::vector<std::string>& variables() const noexcept { return m_variables; }

    /**
     * @brief 用实参值列表填充模板
     *
     * 实参顺序必须与 variables() 一致。每次追加都检查命令长度上限（2,000,000 字符）。
     *
     * @param values 实参字符串列表
     * @return 替换后的完整字符串
     * @throws std::runtime_error 如果替换后超过命令长度上限
     */
    [[nodiscard]] std::string substitute(const std::vector<std::string>& values) const;

private:
    std::vector<std::string> m_segments;
    std::vector<std::string> m_variables;
};

} // namespace function
} // namespace mc
