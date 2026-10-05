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

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mc::util {

/// 标准 Base64 字母表（RFC 4648 §4，索引 62/63 为 '+' 与 '/'）。
inline constexpr std::string_view BASE64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * @brief 标准 Base64 编码（RFC 4648 §4）
 *
 * 输出带 '=' 填充、无换行，语义等价 Java `Base64.getEncoder().encodeToString(bytes)`。
 * 服务端图标（server-icon.png → data URL）与皮肤元数据序列化均使用此编码。
 *
 * @param data 待编码的字节序列
 * @return Base64 字符串；空输入返回空串
 */
[[nodiscard]] std::string base64Encode(std::span<const u8> data);

/**
 * @brief 标准 Base64 解码（RFC 4648 §4）
 *
 * 解码前忽略输入中的 ASCII 空白字符（空格/制表/CR/LF），与皮肤签名解码的既有宽容语义一致；
 * 遇非法字符或长度非法时返回空 vector（整体失败，而非部分解码）。
 *
 * @param encoded Base64 字符串
 * @return 解码后的字节；输入非法或为空时返回空 vector
 */
[[nodiscard]] std::vector<u8> base64Decode(std::string_view encoded);

} // namespace mc::util
