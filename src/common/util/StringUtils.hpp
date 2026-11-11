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
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>

namespace mc::util {

/**
 * @brief 将字符串转换为小写（ASCII）
 *
 * 仅处理 ASCII 字符，非 ASCII 字符保持不变。
 * 用于不区分大小写的字符串比较和搜索。
 *
 * @param text 输入字符串
 * @return 转换后的小写字符串
 */
[[nodiscard]] inline std::string toLowerAscii(std::string_view text)
{
    std::string lowered;
    lowered.reserve(text.size());
    for (const char character : text) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return lowered;
}

/**
 * @brief 检查字符串是否只包含数字
 *
 * @param text 输入字符串
 * @param allowSign 是否允许前导符号（+/-）
 * @return 是否只包含数字
 */
[[nodiscard]] inline bool isNumeric(std::string_view text, bool allowSign = true)
{
    if (text.empty()) {
        return false;
    }

    size_t start = 0;
    if (allowSign && (text[0] == '+' || text[0] == '-')) {
        start = 1;
        if (text.size() == 1) {
            return false; // 只有符号
        }
    }

    for (size_t i = start; i < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// StringUtil：计时、截断、聊天过滤
// ============================================================================

/**
 * @brief 把 tick 数格式化成 `mm:ss` 或 `hh:mm:ss`，对应原版 `StringUtil.formatTickDuration`
 *
 * 先按每秒 tick 数换算成秒（向下取整），再拆时分秒。超过一小时才带上小时段，
 * 且**各段都是两位零填充**（小时超过 99 时自然变长）。
 */
[[nodiscard]] inline std::string formatTickDuration(i32 ticks, f32 ticksPerSecond)
{
    MC_ASSERT_RELEASE_MSG(ticksPerSecond > 0.0f, "formatTickDuration requires a positive tick rate");
    i32 totalSeconds = static_cast<i32>(std::floor(static_cast<f32>(ticks) / ticksPerSecond));
    const i32 seconds = totalSeconds % 60;
    totalSeconds /= 60;
    const i32 minutes = totalSeconds % 60;
    const i32 hours = totalSeconds / 60;

    char buffer[32];
    if (hours > 0) {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", hours, minutes, seconds);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    }
    return buffer;
}

/**
 * @brief 超长时截断并补省略号，对应原版 `StringUtil.truncateStringIfNecessary`
 *
 * 长度不超过上限时原样返回。需要省略号时**在 `maxLength` 内留出 3 个字符给 `"..."`**，
 * 也就是取前 `maxLength - 3` 个字符再接省略号；`maxLength <= 3` 时放不下省略号，
 * 直接硬截到 `maxLength`。
 *
 * 注意长度以 `std::string::size` 计，即按**字节**而非码点——原版按 UTF-16 码元，
 * 对纯 ASCII 两者一致；非 ASCII 文本的边界行为会不同。
 */
[[nodiscard]] inline std::string truncateStringIfNecessary(const std::string& text, i32 maxLength, bool addEllipsis)
{
    if (static_cast<i32>(text.size()) <= maxLength) {
        return text;
    }
    if (addEllipsis && maxLength > 3) {
        return text.substr(0, static_cast<size_t>(maxLength - 3)) + "...";
    }
    return text.substr(0, static_cast<size_t>(std::max(maxLength, 0)));
}

/**
 * @brief 判定非空字符串的每个字符都满足给定谓词
 *
 * 供「只允许某类字符」的校验使用（如属性名 `[a-z0-9_]+`、记分板名 `[A-Za-z0-9_-]+`）。
 * 相比 `std::regex_match` 不构造正则、不抛 `std::regex_error`，也不依赖 locale。
 *
 * @param text 待检查字符串
 * @param predicate 接收 `char` 返回 `bool` 的谓词
 * @return 字符串非空且所有字符都满足谓词时返回 true
 */
template <typename Predicate>
[[nodiscard]] inline bool allOfCharacters(std::string_view text, Predicate predicate) noexcept
{
    if (text.empty()) {
        return false;
    }
    for (const char character : text) {
        if (!predicate(character)) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 检查字符串是否为点分十进制 IPv4 地址（如 `"192.168.0.1"`）
 *
 * 要求恰好 4 段、每段 1~3 位十进制数字且数值不超过 255。允许前导零（与旧正则行为一致）。
 * 不做 DNS/端口解析，也不接受 IPv6。
 *
 * @param text 待检查字符串
 * @return 合法 IPv4 返回 true
 */
[[nodiscard]] inline bool isValidIpv4(std::string_view text) noexcept
{
    if (text.empty()) {
        return false;
    }

    i32 partCount = 0;
    size_t index = 0;
    while (index < text.size()) {
        i32 partValue = 0;
        i32 digitCount = 0;
        while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
            partValue = partValue * 10 + (text[index] - '0');
            ++digitCount;
            ++index;
            if (digitCount > 3 || partValue > 255) {
                return false;
            }
        }
        if (digitCount == 0) {
            return false;
        }
        ++partCount;
        if (index == text.size()) {
            break;
        }
        if (text[index] != '.' || partCount >= 4) {
            return false;
        }
        ++index;
    }
    return partCount == 4;
}

/**
 * @brief 该字符能否出现在聊天文本中，对应原版 `StringUtil.isAllowedChatCharacter`
 *
 * 规则：**不是 `§`(167)、且不小于 32、且不是 127**。也就是排除控制字符、
 * 排除 DEL、以及排除颜色控制符 `§`。
 */
[[nodiscard]] inline constexpr bool isAllowedChatCharacter(char value) noexcept
{
    const auto code = static_cast<unsigned char>(value);
    return code != 167 && code >= 32 && code != 127;
}

/**
 * @brief 玩家名是否合法，对应原版 `StringUtil.isValidPlayerName`
 *
 * 两条规则：**长度不超过 16**（以字节计），且**不含码值 <= 32 或 >= 127 的字符**。
 *
 * 注意第二个条件等价于「全部字符都落在 `[33, 126]`」——比「只允许字母数字下划线」
 * 宽松得多，空格与控制字符被排除但各种标点都允许。
 */
[[nodiscard]] inline bool isValidPlayerName(std::string_view name) noexcept
{
    if (name.size() > 16) {
        return false;
    }
    for (const char value : name) {
        const auto code = static_cast<unsigned char>(value);
        if (code <= 32 || code >= 127) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 过滤聊天文本，对应原版 `StringUtil.filterText(String, boolean)`
 *
 * 逐字符保留合法聊天字符；`keepNewlines` 为真时额外保留 `\n`。
 *
 * 注意 `\n` 本身**不是**合法聊天字符（码值 10 < 32），因此只有显式打开 `keepNewlines`
 * 才会留下换行。
 */
[[nodiscard]] inline std::string filterText(std::string_view text, bool keepNewlines)
{
    std::string filtered;
    filtered.reserve(text.size());
    for (const char value : text) {
        if (isAllowedChatCharacter(value)) {
            filtered.push_back(value);
        } else if (keepNewlines && value == '\n') {
            filtered.push_back(value);
        }
    }
    return filtered;
}

/**
 * @brief 过滤聊天文本（不保留换行），对应原版 `StringUtil.filterText(String)`
 */
[[nodiscard]] inline std::string filterText(std::string_view text)
{
    return filterText(text, false);
}

/**
 * @brief 去掉 `§` 颜色码，对应原版 `StringUtil.stripColor`
 *
 * 原版用正则 `(?i)\u00A7[0-9A-FK-OR]`——`§` 后跟**一个**在 `0-9A-FK-OR` 里的字符
 * （大小写均可）即整段删除。这里逐字符扫描实现同样语义。
 *
 * 注意 `§` 后若不是上述字符，该 `§` **原样保留**，不做吞掉处理。
 *
 * `§`（U+00A7）在 UTF-8 下是**两字节** `0xC2 0xA7`。本函数两种写法都认：
 * 单字节 `0xA7`（Latin-1 风格的字符串）与 `0xC2 0xA7`（正常 UTF-8 文本）。
 * 文档里常见的是后者，忽略它会导致颜色码去不掉。
 */
[[nodiscard]] inline std::string stripColor(std::string_view text)
{
    const auto isFormatCode = [](unsigned char value) {
        return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f') || (value >= 'A' && value <= 'F') ||
            value == 'k' || value == 'l' || value == 'm' || value == 'n' || value == 'o' || value == 'r' ||
            value == 'K' || value == 'L' || value == 'M' || value == 'N' || value == 'O' || value == 'R';
    };

    std::string stripped;
    stripped.reserve(text.size());
    size_t index = 0;
    while (index < text.size()) {
        const auto code = static_cast<unsigned char>(text[index]);

        // UTF-8 的 `§`：0xC2 0xA7，其后还需一个格式码
        if (code == 0xC2 && index + 2 < text.size() && static_cast<unsigned char>(text[index + 1]) == 0xA7 &&
            isFormatCode(static_cast<unsigned char>(text[index + 2]))) {
            index += 3;
            continue;
        }

        // Latin-1 单字节的 `§`
        if (code == 0xA7 && index + 1 < text.size() && isFormatCode(static_cast<unsigned char>(text[index + 1]))) {
            index += 2;
            continue;
        }

        stripped.push_back(text[index]);
        ++index;
    }
    return stripped;
}

/**
 * @brief 统计文本行数，对应原版 `StringUtil.lineCount`
 *
 * 原版正则 `\\r\\n|\\v`：先吃掉 `\\r\\n` 作为**一个**换行，否则任何垂直空白算一个。
 * Java 的 `\\v` 是垂直空白类，含 `\\n`、`\\x0B`、`\\f`、`\\r` 以及 U+0085/U+2028/U+2029。
 *
 * 结果 = 换行次数 + 1（空串特判为 0）。这里只处理 ASCII 与 UTF-8 编码的
 * U+0085/U+2028/U+2029，够覆盖原版行为。
 */
[[nodiscard]] inline i32 lineCount(std::string_view text)
{
    if (text.empty()) {
        return 0;
    }
    i32 lines = 1;
    for (size_t index = 0; index < text.size(); ++index) {
        const auto code = static_cast<unsigned char>(text[index]);
        if (code == '\r') {
            // \r\n 算一处换行；单独 \r 也算一处
            if (index + 1 < text.size() && text[index + 1] == '\n') {
                ++index;
            }
            ++lines;
        } else if (code == '\n' || code == 0x0B || code == 0x0C) {
            ++lines;
        } else if (code == 0xC2 && index + 1 < text.size() && static_cast<unsigned char>(text[index + 1]) == 0x85) {
            ++index;
            ++lines;
        } else if (code == 0xE2 && index + 2 < text.size() && static_cast<unsigned char>(text[index + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[index + 2]) == 0xA8 ||
                static_cast<unsigned char>(text[index + 2]) == 0xA9)) {
            index += 2;
            ++lines;
        }
    }
    return lines;
}

/**
 * @brief 是否为空白字符，对应原版 `StringUtil.isWhitespace`
 *
 * 原版是 `Character.isWhitespace(c) || Character.isSpaceChar(c)` 的并集。
 * 两者单独看都有盲区，并起来才完整：
 *
 * - `isWhitespace` 排除不间断空格（U+00A0、U+2007、U+202F）却含 `\t`/`\n`；
 * - `isSpaceChar` 恰相反，含不间断空格却排除控制字符。
 *
 * 这里按码点判断，覆盖 ASCII 控制空白、U+00A0、U+1680、U+2000..U+200A、U+2028/2029、
 * U+202F、U+205F、U+3000。
 */
[[nodiscard]] inline constexpr bool isWhitespace(u32 codepoint) noexcept
{
    if (codepoint >= 0x09 && codepoint <= 0x0D) {
        return true;
    }
    if (codepoint == 0x20 || codepoint == 0x85 || codepoint == 0xA0 || codepoint == 0x1680) {
        return true;
    }
    if (codepoint >= 0x2000 && codepoint <= 0x200A) {
        return true;
    }
    if (codepoint == 0x2028 || codepoint == 0x2029 || codepoint == 0x202F || codepoint == 0x205F ||
        codepoint == 0x3000) {
        return true;
    }
    return codepoint == 0x1C || codepoint == 0x1D || codepoint == 0x1E || codepoint == 0x1F;
}

} // namespace mc::util
