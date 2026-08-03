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

#include "TranslationTextComponent.hpp"
#include "common/resource/LanguageManager.hpp"
#include "common/util/text/TextStyle.hpp"
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace mc {
namespace text {

// 静态成员初始化
resource::LanguageManager* TranslationTextComponent::s_languageManager = nullptr;

std::string TranslationTextComponent::getUnformattedText() const
{
    // 获取翻译后的文本
    std::string result = getTranslatedText();

    // 添加子组件
    for (const auto& sibling : m_siblings) {
        result += sibling->getUnformattedText();
    }

    return result;
}

std::string TranslationTextComponent::getFormattedText() const
{
    // 获取样式代码
    std::string result = getStyleCodes(m_style);

    // 添加翻译后的文本
    result += getTranslatedText();

    // 添加子组件
    for (const auto& sibling : m_siblings) {
        result += sibling->getFormattedText();
    }

    return result;
}

std::string TranslationTextComponent::getTranslatedText() const
{
    std::string translatedText;

    // 尝试从语言管理器获取翻译
    LanguageManager* manager = s_languageManager != nullptr ? s_languageManager : &LanguageManager::instance();

    // 收集参数文本
    std::vector<std::string> paramTexts;
    paramTexts.reserve(m_params.size());
    for (const auto& param : m_params) {
        paramTexts.push_back(param->getUnformattedText());
    }

    // 获取翻译并替换参数
    if (m_params.empty()) {
        translatedText = manager->get(m_key);
    } else {
        translatedText = manager->get(m_key, paramTexts);
    }

    return translatedText;
}

std::string TranslationTextComponent::replacePlaceholdersLocal(
    const std::string& text, const std::vector<std::string>& params)
{
    if (params.empty() || text.empty()) {
        return text;
    }

    std::string result;
    result.reserve(text.size() * 2);

    size_t i = 0;
    size_t sequentialIndex = 0;

    while (i < text.size()) {
        // 检查 %%
        if (i + 1 < text.size() && text[i] == '%' && text[i + 1] == '%') {
            result += '%';
            i += 2;
            continue;
        }

        // 检查占位符
        if (text[i] == '%' && i + 1 < text.size()) {
            // 检查是否是位置参数格式 %N$s
            if (i + 2 < text.size() && std::isdigit(static_cast<unsigned char>(text[i + 1]))) {
                // 尝试解析位置参数
                size_t digitStart = i + 1;
                size_t digitEnd = digitStart;

                while (digitEnd < text.size() && std::isdigit(static_cast<unsigned char>(text[digitEnd]))) {
                    ++digitEnd;
                }

                // 检查是否是 $s 格式
                if (digitEnd + 1 < text.size() && text[digitEnd] == '$' && text[digitEnd + 1] == 's') {
                    // 解析位置索引
                    int position = 0;
                    try {
                        position = std::stoi(text.substr(digitStart, digitEnd - digitStart));
                    }
                    catch (...) {
                        // 解析失败，保留原样
                        result += text.substr(i, digitEnd + 2 - i);
                        i = digitEnd + 2;
                        continue;
                    }

                    // 替换参数（位置从1开始）
                    if (position >= 1 && position <= static_cast<int>(params.size())) {
                        result += params[position - 1];
                    } else {
                        // 参数不存在，保留原占位符
                        result += text.substr(i, digitEnd + 2 - i);
                    }

                    i = digitEnd + 2;
                    continue;
                }
            }

            // 检查是否是顺序参数 %s
            if (i + 1 < text.size() && text[i + 1] == 's') {
                if (sequentialIndex < params.size()) {
                    result += params[sequentialIndex];
                    ++sequentialIndex;
                } else {
                    // 参数不存在，保留占位符
                    result += "%s";
                }
                i += 2;
                continue;
            }

            // 检查 %d 或 %f（Minecraft 加载时转换为 %s，但这里也处理）
            if (i + 1 < text.size() && (text[i + 1] == 'd' || text[i + 1] == 'f')) {
                if (sequentialIndex < params.size()) {
                    result += params[sequentialIndex];
                    ++sequentialIndex;
                } else {
                    result += text.substr(i, 2);
                }
                i += 2;
                continue;
            }
        }

        // 普通字符
        result += text[i];
        ++i;
    }

    return result;
}

} // namespace text
} // namespace mc
