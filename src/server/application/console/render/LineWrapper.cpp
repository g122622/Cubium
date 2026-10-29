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

#include "LineWrapper.hpp"

#include <algorithm>

namespace mc::server::console {

namespace {

/// 解码一个码点及其字节长度（非法序列返回 1 字节、宽度 1）。
struct DecodedCodepoint {
    u32 codePoint = '?';
    usize byteLength = 1;
};

DecodedCodepoint _decodeCodepoint(std::string_view text, usize index)
{
    const auto lead = static_cast<u8>(text[index]);
    usize length = 1;
    if ((lead & 0x80u) == 0u) {
        length = 1;
    } else if ((lead & 0xE0u) == 0xC0u) {
        length = 2;
    } else if ((lead & 0xF0u) == 0xE0u) {
        length = 3;
    } else if ((lead & 0xF8u) == 0xF0u) {
        length = 4;
    }

    if (index + length > text.size()) {
        return DecodedCodepoint{'?', 1};
    }
    for (usize offset = 1; offset < length; ++offset) {
        if ((static_cast<u8>(text[index + offset]) & 0xC0u) != 0x80u) {
            return DecodedCodepoint{'?', 1};
        }
    }

    u32 codePoint = 0;
    switch (length) {
        case 1:
            codePoint = lead;
            break;
        case 2:
            codePoint = ((lead & 0x1Fu) << 6) | (static_cast<u8>(text[index + 1]) & 0x3Fu);
            break;
        case 3:
            codePoint = ((lead & 0x0Fu) << 12) | ((static_cast<u8>(text[index + 1]) & 0x3Fu) << 6) |
                (static_cast<u8>(text[index + 2]) & 0x3Fu);
            break;
        default:
            codePoint = ((lead & 0x07u) << 18) | ((static_cast<u8>(text[index + 1]) & 0x3Fu) << 12) |
                ((static_cast<u8>(text[index + 2]) & 0x3Fu) << 6) | (static_cast<u8>(text[index + 3]) & 0x3Fu);
            break;
    }
    return DecodedCodepoint{codePoint, length};
}

/**
 * @brief 码点在终端中的显示宽度
 *
 * 与 FTXUI 的 `string_width` 口径一致：组合字符与零宽字符计 0，东亚宽字符计 2，其余 1。
 * 见 src/ftxui/dom/string.cpp（FTXUI v6.1.9）中的区间表。
 */
usize _codepointWidth(u32 codePoint)
{
    // 组合附加符号、零宽字符与变体选择符：不占位。
    if ((codePoint >= 0x0300 && codePoint <= 0x036F) || (codePoint >= 0x200B && codePoint <= 0x200F) ||
        (codePoint >= 0xFE00 && codePoint <= 0xFE0F)) {
        return 0;
    }

    if ((codePoint >= 0x1100 && codePoint <= 0x115F) || codePoint == 0x2329 || codePoint == 0x232A ||
        (codePoint >= 0x2E80 && codePoint <= 0x303E) || (codePoint >= 0x3040 && codePoint <= 0xA4CF) ||
        (codePoint >= 0xAC00 && codePoint <= 0xD7A3) || (codePoint >= 0xF900 && codePoint <= 0xFAFF) ||
        (codePoint >= 0xFE10 && codePoint <= 0xFE19) || (codePoint >= 0xFE30 && codePoint <= 0xFE6F) ||
        (codePoint >= 0xFF00 && codePoint <= 0xFF60) || (codePoint >= 0xFFE0 && codePoint <= 0xFFE6) ||
        (codePoint >= 0x1F300 && codePoint <= 0x1F64F) || (codePoint >= 0x1F900 && codePoint <= 0x1F9FF) ||
        (codePoint >= 0x20000 && codePoint <= 0x2FFFD) || (codePoint >= 0x30000 && codePoint <= 0x3FFFD)) {
        return 2;
    }
    return 1;
}

/// 字符串中每个码点的 (起始字节, 字节长度, 显示宽度)。
struct CodepointSpan {
    usize start = 0;
    usize length = 0;
    usize width = 0;
};

std::vector<CodepointSpan> _splitCodepoints(std::string_view text)
{
    std::vector<CodepointSpan> spans;
    spans.reserve(text.size());
    usize index = 0;
    while (index < text.size()) {
        const DecodedCodepoint decoded = _decodeCodepoint(text, index);
        spans.push_back(CodepointSpan{index, decoded.byteLength, _codepointWidth(decoded.codePoint)});
        index += decoded.byteLength;
    }
    return spans;
}

} // namespace

usize displayWidth(std::string_view text)
{
    usize width = 0;
    usize index = 0;
    while (index < text.size()) {
        const DecodedCodepoint decoded = _decodeCodepoint(text, index);
        width += _codepointWidth(decoded.codePoint);
        index += decoded.byteLength;
    }
    return width;
}

std::string expandTabs(std::string_view text, usize tabWidth)
{
    const usize width = std::max<usize>(tabWidth, 1);
    std::string result;
    result.reserve(text.size());

    usize column = 0;
    usize index = 0;
    while (index < text.size()) {
        const char current = text[index];
        if (current == '\t') {
            // 与终端约定一致：对齐到下一个制表位。
            const usize advance = width - (column % width);
            result.append(advance, ' ');
            column += advance;
            ++index;
            continue;
        }
        const DecodedCodepoint decoded = _decodeCodepoint(text, index);
        result.append(text.substr(index, decoded.byteLength));
        column += _codepointWidth(decoded.codePoint);
        index += decoded.byteLength;
    }
    return result;
}

std::vector<std::string> wrapByDisplayWidth(std::string_view text, usize maxWidth)
{
    if (text.empty()) {
        return {std::string()};
    }
    if (maxWidth == 0) {
        return {std::string(text)};
    }

    const std::vector<CodepointSpan> spans = _splitCodepoints(text);
    std::vector<std::string> lines;

    const auto isSpaceAt = [&text, &spans](usize position) {
        return text[spans[position].start] == ' ' && spans[position].length == 1;
    };

    usize lineStart = 0;                      // 当前行起始码点下标
    usize lineWidth = 0;                      // 当前行已用宽度
    usize lastSpace = static_cast<usize>(-1); // 当前行内最后一个空格（可在此断行）

    usize index = 0;
    while (index < spans.size()) {
        const CodepointSpan& span = spans[index];

        // 单个字符本身就宽于整行（终端极窄时宽字符也会如此）：该字符独占一行。
        // 必须先于"整行溢出"判断处理，否则它在行首时会走空格断行分支，产生畸形空行。
        const bool oversizeChar = span.width > maxWidth;
        if (oversizeChar || (lineWidth + span.width > maxWidth && lineWidth > 0)) {
            // 断点：优先在最后一个空格后断行（空格留在上一行，拼回去与原文一致）；
            // 没有可断的空格时在当前位置硬切。lastSpace + 1 >= lineStart 恒成立
            // （lineStart 随断点前移），无需额外判断。
            usize breakIndex = lastSpace != static_cast<usize>(-1) ? lastSpace + 1 : index;
            if (oversizeChar) {
                breakIndex = index; // 超宽字符无论前面是否断过，都必须落到下一行
            }

            if (breakIndex > lineStart) {
                const usize byteBegin = spans[lineStart].start;
                lines.emplace_back(text.substr(byteBegin, spans[breakIndex].start - byteBegin));
                lineStart = breakIndex;
            }

            lineWidth = 0;
            lastSpace = static_cast<usize>(-1);
            if (oversizeChar) {
                // 该字符无论如何都放不进任何一行：独占输出，避免下一轮再触发断行。
                lines.emplace_back(text.substr(span.start, span.length));
                lineStart = index + 1;
                ++index;
                continue;
            }
            for (usize carried = lineStart; carried < index; ++carried) {
                lineWidth += spans[carried].width;
                if (isSpaceAt(carried)) {
                    lastSpace = carried;
                }
            }
            continue;
        }

        if (isSpaceAt(index)) {
            lastSpace = index;
        }
        lineWidth += span.width;
        ++index;
    }

    // 收尾：仅当还有未输出的码点时补一行。超宽字符分支会恰好把 lineStart 推到末尾，
    // 此时不能多产出一个空串（否则日志区会多出空行、复制结果带多余换行）。
    if (lineStart < spans.size()) {
        lines.emplace_back(text.substr(spans[lineStart].start));
    }
    return lines;
}

} // namespace mc::server::console
