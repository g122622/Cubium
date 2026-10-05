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

#include "common/util/Base64.hpp"

#include <array>
#include <cctype>

namespace mc::util {

std::string base64Encode(std::span<const u8> data)
{
    std::string encoded;
    if (data.empty()) {
        return encoded;
    }
    // 每 3 字节输入产生 4 字符输出，向上取整。
    encoded.reserve(((data.size() + 2) / 3) * 4);

    const usize size = data.size();
    usize i = 0;
    for (; i + 3 <= size; i += 3) {
        const u32 chunk =
            (static_cast<u32>(data[i]) << 16) | (static_cast<u32>(data[i + 1]) << 8) | static_cast<u32>(data[i + 2]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 18) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 12) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 6) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[chunk & 0x3F]);
    }

    // 尾部剩余 1 或 2 字节：补 '=' 至 4 字符边界。
    const usize remaining = size - i;
    if (remaining == 1) {
        const u32 chunk = static_cast<u32>(data[i]) << 16;
        encoded.push_back(BASE64_ALPHABET[(chunk >> 18) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 12) & 0x3F]);
        encoded.push_back('=');
        encoded.push_back('=');
    } else if (remaining == 2) {
        const u32 chunk = (static_cast<u32>(data[i]) << 16) | (static_cast<u32>(data[i + 1]) << 8);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 18) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 12) & 0x3F]);
        encoded.push_back(BASE64_ALPHABET[(chunk >> 6) & 0x3F]);
        encoded.push_back('=');
    }
    return encoded;
}

std::vector<u8> base64Decode(std::string_view encoded)
{
    std::vector<u8> decoded;
    if (encoded.empty()) {
        return decoded;
    }

    // 忽略 ASCII 空白（与皮肤签名解码的既有宽容语义一致）。
    std::string clean;
    clean.reserve(encoded.size());
    for (char c : encoded) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            clean.push_back(c);
        }
    }
    if (clean.empty() || clean.size() % 4 != 0) {
        return decoded;
    }

    usize padding = 0;
    if (clean.back() == '=') {
        ++padding;
        if (clean.size() >= 2 && clean[clean.size() - 2] == '=') {
            ++padding;
        }
    }
    decoded.reserve((clean.size() / 4) * 3 - padding);

    u32 buffer = 0;
    i32 bits = 0;
    for (char c : clean) {
        if (c == '=') {
            buffer <<= 6;
            bits += 6;
            continue;
        }
        const usize pos = BASE64_ALPHABET.find(c);
        if (pos == std::string_view::npos) {
            // 非法字符：整体解码失败（避免把损坏数据部分解码成非空字节）。
            decoded.clear();
            return decoded;
        }
        buffer = (buffer << 6) | static_cast<u32>(pos);
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            decoded.push_back(static_cast<u8>((buffer >> bits) & 0xFF));
        }
    }
    return decoded;
}

} // namespace mc::util
