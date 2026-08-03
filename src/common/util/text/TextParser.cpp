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

#include "TextParser.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"
#include <cctype>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::text {

std::unique_ptr<ITextComponent> TextParser::parse(std::string_view text)
{
    ParseState state;

    size_t i = 0;
    while (i < text.length()) {
        char c = text[i];

        // 检查 § 代码
        if (c == '\xC2' && i + 1 < text.length() && text[i + 1] == '\xA7') {
            // UTF-8 编码的 § (C2 A7)
            i += 2;
            if (i < text.length()) {
                handleFormattingCode(state, text[i]);
            }
            ++i;
            continue;
        }

        // 检查直接的 § 字符（某些情况下使用）
        if (c == '\xA7' && i + 1 < text.length()) {
            ++i;
            handleFormattingCode(state, text[i]);
            ++i;
            continue;
        }

        // 普通字符
        state.currentText += c;
        ++i;
    }

    // 刷新剩余文本
    state.flushText();

    // 优化：如果根组件为空且只有一个子组件，返回子组件
    const auto& siblings = state.root->getSiblings();
    if (state.root->getText().empty() && siblings.size() == 1) {
        // 将唯一子组件作为根返回
        auto& child = const_cast<std::unique_ptr<ITextComponent>&>(siblings[0]);
        return std::move(const_cast<std::vector<std::unique_ptr<ITextComponent>>&>(siblings)[0]);
    }

    // 返回根组件
    return std::move(state.root);
}

std::string TextParser::toLegacyFormat(const ITextComponent& component)
{
    std::string result;

    // 添加当前组件的样式代码
    Style currentStyle = component.getStyle();
    result += getStyleCodes(currentStyle);

    // 添加当前组件的文本
    const StringTextComponent* stringComp = dynamic_cast<const StringTextComponent*>(&component);
    if (stringComp) {
        result += stringComp->getText();
    }

    // 递归处理子组件
    for (const auto& sibling : component.getSiblings()) {
        result += toLegacyFormat(*sibling);
    }

    return result;
}

bool TextParser::handleFormattingCode(ParseState& state, char code)
{
    // 转换为小写
    char lowerCode = static_cast<char>(std::tolower(static_cast<unsigned char>(code)));

    TextFormatting formatting = fromCode(lowerCode);

    if (formatting == TextFormatting::None) {
        // 无效代码，当作普通字符处理
        state.currentText += "§";
        state.currentText += code;
        return false;
    }

    // 重置代码
    if (formatting == TextFormatting::Reset) {
        state.flushText();
        state.currentStyle = Style();
        return true;
    }

    // 刷新当前文本
    state.flushText();

    // 更新样式
    if (isColor(formatting)) {
        // 颜色代码重置所有样式
        state.currentStyle.setColor(formatting);
        state.currentStyle.setBold(false);
        state.currentStyle.setItalic(false);
        state.currentStyle.setUnderlined(false);
        state.currentStyle.setStrikethrough(false);
        state.currentStyle.setObfuscated(false);
    } else {
        // 样式代码
        switch (formatting) {
            case TextFormatting::Bold:
                state.currentStyle.setBold(true);
                break;
            case TextFormatting::Italic:
                state.currentStyle.setItalic(true);
                break;
            case TextFormatting::Underline:
                state.currentStyle.setUnderlined(true);
                break;
            case TextFormatting::Strikethrough:
                state.currentStyle.setStrikethrough(true);
                break;
            case TextFormatting::Obfuscated:
                state.currentStyle.setObfuscated(true);
                break;
            default:
                break;
        }
    }

    return true;
}

} // namespace mc::text
