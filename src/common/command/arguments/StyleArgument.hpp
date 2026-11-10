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

#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/core/Types.hpp"
#include "common/util/nbt/NbtJsonUtils.hpp"
#include "common/util/text/StyleNbt.hpp"
#include "common/util/text/TextStyle.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc::command {

/**
 * @brief `style` 参数（原版 {@code StyleArgument}，命令树里叫 `style`）
 *
 * 语法是 **SNBT 复合标签**：`{}`、`{color:"red"}`、`{bold:true,italic:1b}`、`{color:"#FF5555"}`。
 * 原版用 `SnbtGrammar` 解析成 NBT 再经 `Style.Serializer.CODEC` 解码，这里走同一条链：
 * `parseMojangson` → `text::styleFromNbt`。
 *
 * 值类型用 `shared_ptr<Style>` 而不是 `Style`：`Style` 可默认构造，而本项目的参数节点协议把
 * 「默认值」当作「这一层没有设置命令」，`style {}` 的空样式恰好是默认值，会被误判成没有命令。
 * 包一层指针后 `kDefaultConstructible` 为 false，解析出的任何样式都算真实参数。
 */
using StylePtr = std::shared_ptr<text::Style>;

class StyleArgumentType : public ArgumentType<StylePtr> {
public:
    [[nodiscard]] StylePtr parse(StringReader& reader) override
    {
        const i32 start = reader.getCursor();
        const std::string text = _readCompoundText(reader);
        if (text.empty()) {
            throw CommandException(
                CommandErrorType::Unknown, "Expected style at position " + std::to_string(start), start);
        }

        // SNBT → NBT 复合标签 → Style。两条失败路径都必须报错而不是返回空样式：
        // 静默回退会让写错的命令看起来成功、实际把格式清成了「无样式」。
        auto tagResult = nbt::parseMojangson(text);
        if (tagResult.failed()) {
            throw CommandException(CommandErrorType::Unknown, "Invalid style (not valid SNBT): " + text, start);
        }
        auto tag = std::move(tagResult).value();
        auto style = text::styleFromNbt(*tag);
        if (style.failed()) {
            throw CommandException(
                CommandErrorType::Unknown, "Invalid style " + text + ": " + style.error().message(), start);
        }

        return std::make_shared<text::Style>(std::move(style.value()));
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "style";
    }

    /// 原版 StyleArgument 注册为 SingletonArgumentInfo（网络载荷为空），故元数据无属性。
    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"{bold:true}", "{color:\"red\"}", "{}"};
    }

    static std::shared_ptr<StyleArgumentType> style()
    {
        return std::make_shared<StyleArgumentType>();
    }

private:
    /**
     * @brief 读取一段完整的 SNBT 复合标签文本
     *
     * 命令参数按空白分隔，但 SNBT 允许在花括号内出现空白（`{bold: true}`）与含空白的引号字符串，
     * 因此必须按**括号配平**扫描：花括号/方括号进栈、引号内的字符不参与配平、引号内支持
     * 反斜杠转义。扫描到配平结束（或输入结束）为止，交给 SNBT 解析器做完整校验。
     */
    [[nodiscard]] static std::string _readCompoundText(StringReader& reader)
    {
        const i32 start = reader.getCursor();
        std::string text;
        i32 depth = 0;
        char quote = '\0';

        while (reader.canRead()) {
            const char c = reader.peek();

            if (quote != '\0') {
                // 引号内：只关心转义与收尾引号，其他字符（含空白、括号）原样收下
                text.push_back(c);
                reader.skip();
                if (c == '\\' && reader.canRead()) {
                    text.push_back(reader.peek());
                    reader.skip();
                    continue;
                }
                if (c == quote) {
                    quote = '\0';
                }
                continue;
            }

            if (c == '"' || c == '\'') {
                quote = c;
                text.push_back(c);
                reader.skip();
                continue;
            }

            if (c == '{' || c == '[') {
                ++depth;
            } else if (c == '}' || c == ']') {
                --depth;
            } else if (depth == 0 && reader.isWhitespace(c)) {
                // 顶层空白即参数结束
                break;
            }

            text.push_back(c);
            reader.skip();

            if (depth == 0 && (c == '}' || c == ']')) {
                // 最外层复合标签/列表已闭合：参数到此为止
                break;
            }
        }

        if (depth != 0 || quote != '\0') {
            // 未配平：把游标退回起点，让上层报出准确的错误位置
            reader.setCursor(start);
            return {};
        }
        return text;
    }
};

} // namespace mc::command
