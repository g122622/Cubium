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

#include "common/core/Types.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace mc::util::text {

// ============================================================================
// UTF-8 编码 / 解码 / 迭代工具
// ============================================================================

/**
 * @brief 计算 Unicode 码点编码为 UTF-8 后的字节数
 *
 * @param codePoint Unicode 码点
 * @return UTF-8 字节数（1~4），非法码点返回 0
 */
[[nodiscard]] inline u8 utf8EncodeLength(u32 codePoint)
{
    if (codePoint < 0x80) {
        return 1;
    }
    if (codePoint < 0x800) {
        return 2;
    }
    if (codePoint < 0x10000) {
        return 3;
    }
    if (codePoint < 0x110000) {
        return 4;
    }
    return 0;
}

/**
 * @brief 将 Unicode 码点编码为 UTF-8 字节序列
 *
 * @param codePoint Unicode 码点
 * @return UTF-8 编码后的字符串；非法码点返回空串
 */
[[nodiscard]] inline std::string utf8Encode(u32 codePoint)
{
    std::string result;
    if (codePoint < 0x80) {
        result += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        result += static_cast<char>(0xC0 | (codePoint >> 6));
        result += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        result += static_cast<char>(0xE0 | (codePoint >> 12));
        result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x110000) {
        result += static_cast<char>(0xF0 | (codePoint >> 18));
        result += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
    return result;
}

/**
 * @brief 将 Unicode 码点编码为 UTF-8 并追加到字符串末尾
 *
 * @param output 目标字符串
 * @param codePoint Unicode 码点
 */
inline void utf8Append(std::string& output, u32 codePoint)
{
    if (codePoint < 0x80) {
        output += static_cast<char>(codePoint);
    } else if (codePoint < 0x800) {
        output += static_cast<char>(0xC0 | (codePoint >> 6));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
        output += static_cast<char>(0xE0 | (codePoint >> 12));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x110000) {
        output += static_cast<char>(0xF0 | (codePoint >> 18));
        output += static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F));
        output += static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F));
        output += static_cast<char>(0x80 | (codePoint & 0x3F));
    }
}

/**
 * @brief 判断字节是否为 UTF-8 续接字节（10xxxxxx）
 */
[[nodiscard]] inline bool utf8IsContinuationByte(u8 byte)
{
    return (byte & 0xC0u) == 0x80u;
}

/**
 * @brief 根据 UTF-8 首字节推断该码点的字节长度
 *
 * @param leadByte UTF-8 序列的首字节
 * @return 字节长度（1~4），无效首字节返回 1（按单字节处理）
 */
[[nodiscard]] inline u8 utf8LeadByteLength(u8 leadByte)
{
    if ((leadByte & 0x80u) == 0u) {
        return 1;
    }
    if ((leadByte & 0xE0u) == 0xC0u) {
        return 2;
    }
    if ((leadByte & 0xF0u) == 0xE0u) {
        return 3;
    }
    if ((leadByte & 0xF8u) == 0xF0u) {
        return 4;
    }
    return 1;
}

/**
 * @brief 从 UTF-8 字节序列中解码一个码点
 *
 * @param text UTF-8 字符串
 * @param index 起始字节索引，函数返回后指向下一个码点的起始位置
 * @return 解码得到的 Unicode 码点；遇到无效序列时返回 '?' 并跳过 1 字节
 */
[[nodiscard]] u32 utf8Decode(const std::string& text, size_t& index);

/**
 * @brief 获取指定字节位置处码点的字节长度（不跨越码点边界）
 *
 * 如果 index 指向续接字节，返回 1（按单字节错误处理）。
 *
 * @param text UTF-8 字符串
 * @param index 字节位置
 * @return 该码点的 UTF-8 字节长度
 */
[[nodiscard]] inline size_t utf8CodepointByteLength(const std::string& text, size_t index)
{
    if (index >= text.size()) {
        return 0;
    }
    const auto lead = static_cast<u8>(text[index]);
    const size_t expected = utf8LeadByteLength(lead);
    return std::min(index + expected, text.size()) - index;
}

/**
 * @brief 获取下一个码点的起始字节索引
 *
 * @param text UTF-8 字符串
 * @param index 当前码点的起始字节索引
 * @return 下一个码点的起始字节索引；若越界则返回 text.size()
 */
[[nodiscard]] inline size_t utf8NextCodepointIndex(const std::string& text, size_t index)
{
    if (index >= text.size()) {
        return text.size();
    }
    return index + utf8CodepointByteLength(text, index);
}

/**
 * @brief 获取前一个码点的起始字节索引
 *
 * @param text UTF-8 字符串
 * @param index 当前码点的起始字节索引（必须 > 0）
 * @return 前一个码点的起始字节索引；若越界则返回 0
 */
[[nodiscard]] inline size_t utf8PrevCodepointIndex(const std::string& text, size_t index)
{
    if (index == 0) {
        return 0;
    }
    size_t prev = index - 1;
    // 向前跳过续接字节，直到找到首字节
    while (prev > 0 && utf8IsContinuationByte(static_cast<u8>(text[prev]))) {
        --prev;
    }
    return prev;
}

/**
 * @brief 计算 UTF-8 字符串中的码点数量
 *
 * @param text UTF-8 字符串
 * @return 码点数量
 */
[[nodiscard]] inline size_t utf8CodepointCount(const std::string& text)
{
    size_t count = 0;
    size_t index = 0;
    while (index < text.size()) {
        index = utf8NextCodepointIndex(text, index);
        ++count;
    }
    return count;
}

/**
 * @brief 将码点索引转换为字节偏移量
 *
 * @param text UTF-8 字符串
 * @param codepointIndex 码点索引（从 0 开始）
 * @return 对应的字节偏移量；若越界则返回 text.size()
 */
[[nodiscard]] inline size_t utf8CodepointToByteOffset(const std::string& text, size_t codepointIndex)
{
    size_t byteOffset = 0;
    for (size_t i = 0; i < codepointIndex && byteOffset < text.size(); ++i) {
        byteOffset = utf8NextCodepointIndex(text, byteOffset);
    }
    return byteOffset;
}

/**
 * @brief 将字节偏移量转换为码点索引
 *
 * @param text UTF-8 字符串
 * @param byteOffset 字节偏移量
 * @return 对应的码点索引
 */
[[nodiscard]] inline size_t utf8ByteOffsetToCodepointIndex(const std::string& text, size_t byteOffset)
{
    size_t codepointIndex = 0;
    size_t index = 0;
    while (index < byteOffset && index < text.size()) {
        index = utf8NextCodepointIndex(text, index);
        ++codepointIndex;
    }
    return codepointIndex;
}

/**
 * @brief 将字节偏移量对齐到最近的码点边界
 *
 * 如果 byteOffset 正好在码点边界上，直接返回；
 * 否则向后对齐到下一个码点边界。
 *
 * @param text UTF-8 字符串
 * @param byteOffset 字节偏移量
 * @return 对齐后的字节偏移量
 */
[[nodiscard]] inline size_t utf8AlignToCodepointBoundary(const std::string& text, size_t byteOffset)
{
    while (byteOffset < text.size() && utf8IsContinuationByte(static_cast<u8>(text[byteOffset]))) {
        ++byteOffset;
    }
    return byteOffset;
}

/**
 * @brief 对 UTF-8 字符串中的每个码点执行回调
 *
 * @param text UTF-8 字符串
 * @param callback 回调函数，签名为 void(u32 codePoint, size_t byteOffset, size_t byteLength)
 */
template <typename Callback>
void utf8ForEachCodepoint(const std::string& text, Callback&& callback)
{
    size_t index = 0;
    while (index < text.size()) {
        const size_t start = index;
        const size_t nextIndex = utf8NextCodepointIndex(text, index);
        // 快速解码首字节
        const auto lead = static_cast<u8>(text[start]);
        u32 codePoint = 0;

        if ((lead & 0x80u) == 0u) {
            codePoint = lead;
        } else if ((lead & 0xE0u) == 0xC0u) {
            codePoint = static_cast<u32>(lead & 0x1Fu);
            if (nextIndex > start + 1) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 1]) & 0x3Fu);
            }
        } else if ((lead & 0xF0u) == 0xE0u) {
            codePoint = static_cast<u32>(lead & 0x0Fu);
            if (nextIndex > start + 1) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 1]) & 0x3Fu);
            }
            if (nextIndex > start + 2) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 2]) & 0x3Fu);
            }
        } else if ((lead & 0xF8u) == 0xF0u) {
            codePoint = static_cast<u32>(lead & 0x07u);
            if (nextIndex > start + 1) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 1]) & 0x3Fu);
            }
            if (nextIndex > start + 2) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 2]) & 0x3Fu);
            }
            if (nextIndex > start + 3) {
                codePoint = (codePoint << 6) | static_cast<u32>(static_cast<u8>(text[start + 3]) & 0x3Fu);
            }
        } else {
            codePoint = U'?';
        }

        callback(codePoint, start, nextIndex - start);
        index = nextIndex;
    }
}

} // namespace mc::util::text
