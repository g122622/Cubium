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

#include "Utf8.hpp"
#include "common/core/Types.hpp"
#include <cstddef>
#include <string>

namespace mc::util::text {

u32 utf8Decode(const std::string& text, size_t& index)
{
    if (index >= text.size()) {
        return U'\0';
    }

    const auto lead = static_cast<u8>(text[index]);
    size_t length = 1;

    if ((lead & 0x80u) == 0u) {
        // 单字节序列 (0xxxxxxx)
        u32 codePoint = lead;
        ++index;
        return codePoint;
    }

    if ((lead & 0xE0u) == 0xC0u) {
        // 双字节序列 (110xxxxx 10xxxxxx)
        length = 2;
        if (index + 1 >= text.size() || !utf8IsContinuationByte(static_cast<u8>(text[index + 1]))) {
            ++index;
            return U'?';
        }
        u32 codePoint = static_cast<u32>(lead & 0x1Fu);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 1]) & 0x3Fu);
        index += length;
        // 检查代理范围和非最短编码
        if (codePoint < 0x80) {
            return U'?';
        }
        return codePoint;
    }

    if ((lead & 0xF0u) == 0xE0u) {
        // 三字节序列 (1110xxxx 10xxxxxx 10xxxxxx)
        length = 3;
        if (index + 2 >= text.size() || !utf8IsContinuationByte(static_cast<u8>(text[index + 1])) ||
            !utf8IsContinuationByte(static_cast<u8>(text[index + 2]))) {
            ++index;
            return U'?';
        }
        u32 codePoint = static_cast<u32>(lead & 0x0Fu);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 1]) & 0x3Fu);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 2]) & 0x3Fu);
        index += length;
        // 检查代理范围和非最短编码
        if (codePoint < 0x800 || (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
            return U'?';
        }
        return codePoint;
    }

    if ((lead & 0xF8u) == 0xF0u) {
        // 四字节序列 (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        length = 4;
        if (index + 3 >= text.size() || !utf8IsContinuationByte(static_cast<u8>(text[index + 1])) ||
            !utf8IsContinuationByte(static_cast<u8>(text[index + 2])) ||
            !utf8IsContinuationByte(static_cast<u8>(text[index + 3]))) {
            ++index;
            return U'?';
        }
        u32 codePoint = static_cast<u32>(lead & 0x07u);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 1]) & 0x3Fu);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 2]) & 0x3Fu);
        codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[index + 3]) & 0x3Fu);
        index += length;
        // 检查有效范围和非最短编码
        if (codePoint < 0x10000 || codePoint >= 0x110000) {
            return U'?';
        }
        return codePoint;
    }

    // 无效的首字节
    ++index;
    return U'?';
}

} // namespace mc::util::text
