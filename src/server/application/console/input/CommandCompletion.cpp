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

#include "CommandCompletion.hpp"

#include <algorithm>

namespace mc::server::console {

usize CompletionState::reset(std::string_view input, const mc::command::Suggestions& suggestions)
{
    m_candidates.clear();
    m_selected = 0;

    const auto& list = suggestions.getList();
    m_candidates.reserve(std::min(list.size(), MAX_CANDIDATES));
    for (const auto& suggestion : list) {
        if (m_candidates.size() >= MAX_CANDIDATES) {
            break;
        }
        // 同一候选文本只保留一次：建议来自多个 provider 合并时可能重复。
        const std::string full = suggestion.apply(input);
        const bool duplicated =
            std::any_of(m_candidates.begin(), m_candidates.end(), [&full](const CompletionCandidate& existing) {
                return existing.text == full;
            });
        if (duplicated) {
            continue;
        }

        m_candidates.push_back(CompletionCandidate{full, suggestion.getStart(), suggestion.getTooltip()});
    }

    return m_candidates.size();
}

void CompletionState::clear()
{
    m_candidates.clear();
    m_selected = 0;
}

void CompletionState::selectPrevious()
{
    if (m_candidates.empty()) {
        return;
    }
    const i32 count = static_cast<i32>(m_candidates.size());
    m_selected = (m_selected - 1 + count) % count;
}

void CompletionState::selectNext()
{
    if (m_candidates.empty()) {
        return;
    }
    const i32 count = static_cast<i32>(m_candidates.size());
    m_selected = (m_selected + 1) % count;
}

bool CompletionState::applySelected(std::string& input, i32& cursor) const
{
    if (m_candidates.empty()) {
        return false;
    }

    const i32 index = std::clamp(m_selected, 0, static_cast<i32>(m_candidates.size()) - 1);
    const CompletionCandidate& candidate = m_candidates[static_cast<usize>(index)];
    input = candidate.text;
    // 光标停在替换后候选的末尾：命令名补全后紧接着可以输入参数。
    cursor = static_cast<i32>(input.size());
    return true;
}

std::string normalizePastedText(std::string_view pasted)
{
    std::string result;
    result.reserve(pasted.size());

    for (const char raw : pasted) {
        const auto byte = static_cast<unsigned char>(raw);
        if (byte == '\r') {
            continue; // CRLF 中的 CR 丢弃
        }
        if (byte == '\n' || byte == '\t') {
            result.push_back(' ');
            continue;
        }
        if (byte < 0x20 || byte == 0x7F) {
            continue; // 其余控制字符不可见，直接丢弃
        }
        result.push_back(raw);
    }

    return result;
}

bool isCommandInput(std::string_view input)
{
    // 允许前导空白：用户可能在输入框里先敲了空格。
    usize index = input.find_first_not_of(" \t");
    if (index == std::string_view::npos) {
        return false;
    }
    return input[index] == '/';
}

std::string toSayCommand(std::string_view input)
{
    const usize begin = input.find_first_not_of(" \t");
    if (begin == std::string_view::npos) {
        return {};
    }
    std::string result = "say ";
    result.append(input.substr(begin));
    return result;
}

} // namespace mc::server::console
