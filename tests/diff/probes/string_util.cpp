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

/**
 * @brief 差分能力：`StringUtil` 的字符串工具（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.util.StringUtil`：
 *
 * ```java
 * private static final Pattern STRIP_COLOR_PATTERN = Pattern.compile("(?i)\u00A7[0-9A-FK-OR]");
 * private static final Pattern LINE_PATTERN = Pattern.compile("\r\n|\v");
 *
 * public static String formatTickDuration(int ticks, float ticksPerSecond) {
 *     int i = Mth.floor(ticks / ticksPerSecond);   // 注意：int / float 是浮点除法
 *     int j = i / 60;  i %= 60;
 *     int k = j / 60;  j %= 60;
 *     return k > 0 ? String.format(Locale.ROOT, "%02d:%02d:%02d", k, j, i)
 *                  : String.format(Locale.ROOT, "%02d:%02d", j, i);
 * }
 *
 * public static String stripColor(String s) { return STRIP_COLOR_PATTERN.matcher(s).replaceAll(""); }
 *
 * public static String truncateStringIfNecessary(String s, int maxLength, boolean addEllipsis) {
 *     if (s.length() <= maxLength) return s;
 *     return addEllipsis && maxLength > 3
 *         ? s.substring(0, maxLength - 3) + "..."
 *         : s.substring(0, maxLength);
 * }
 *
 * public static int lineCount(String s) { /* 空串得 0，否则 1 + LINE_PATTERN 匹配次数 *\/ ; }
 *
 * public static String trimChatMessage(String s) { return truncateStringIfNecessary(s, 256, false); }
 *
 * public static boolean isAllowedChatCharacter(int c) { return c != 167 && c >= 32 && c != 127; }
 *
 * public static boolean isValidPlayerName(String s) {
 *     return s.length() > 16 ? false : s.chars().filter(c -> c <= 32 || c >= 127).findAny().isEmpty();
 * }
 *
 * public static String filterText(String s) { return filterText(s, false); }
 * public static String filterText(String s, boolean keepNewlines) {
 *     StringBuilder sb = new StringBuilder();
 *     for (char c : s.toCharArray()) {
 *         if (isAllowedChatCharacter(c)) sb.append(c);
 *         else if (keepNewlines && c == '\n') sb.append(c);
 *     }
 *     return sb.toString();
 * }
 *
 * public static boolean isWhitespace(int c) {
 *     return Character.isWhitespace(c) || Character.isSpaceChar(c);
 * }
 *
 * public static boolean isBlank(@Nullable String s) {
 *     return s != null && !s.isEmpty() ? s.chars().allMatch(StringUtil::isWhitespace) : true;
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`truncateStringIfNecessary` 的三个分支容易漏掉 `maxLength > 3` 这个条件。**
 *
 *    ```java
 *    return addEllipsis && maxLength > 3
 *        ? s.substring(0, maxLength - 3) + "..."
 *        : s.substring(0, maxLength);
 *    ```
 *
 *    - `addEllipsis = true, maxLength = 3`：**不加省略号**，直接截 3 个字符（因为 `3 - 3 = 0`
 *      加 `"..."` 会超长）
 *    - `addEllipsis = true, maxLength = 4`：截 1 个字符 + `"..."`，结果长度正好 4
 *    - `addEllipsis = false`：无论 `maxLength` 多少都直接截断
 *
 *    **`maxLength <= 0` 时 `substring(0, maxLength)` 会抛异常**——原版没有保护。
 *
 * 2. **`lineCount` 的空串特判与环境无关。**
 *
 *    ```java
 *    if (s.isEmpty()) return 0;
 *    ```
 *
 *    否则 `""` 会被算成 1 行。而 `"\n"`（单个换行）得 **1**（不是 2），因为 `LINE_PATTERN`
 *    只匹配 `\r\n` 和 `\v`，**不匹配单独的 `\n`**。
 *
 * 3. **`stripColor` 的正则包含 `(?i)` 不区分大小写标志，但 `\u00A7` 后面只认 `[0-9A-FK-OR]`。**
 *
 *    `\u00A7` 是 `§`（分节符）。`(?i)` 让 `a-f` 与 `A-F` 都能匹配，`k-o` 与 `K-O` 同理。
 *
 *    **`\u00A7z` 不会被剥离**——`z` 不在字母表里。
 *
 * 4. **`formatTickDuration` 的 `ticks / ticksPerSecond` 是 `int / float`，结果是 `float`。**
 *
 *    不是整数除法！`Mth.floor(float)` 再向负无穷取整。
 *
 *    - `formatTickDuration(119, 20.0F)` → `119/20 = 5.95` → `floor = 5` → `00:05`
 *    - `formatTickDuration(3600, 20.0F)` → `180` → `3` 分 `0` 秒 → `03:00`
 *
 *    `ticksPerSecond <= 0` 时会产生 `Infinity`/`NaN`，原版不保护。
 *
 * 单元格式：
 * - `duration|<ticks>|<ticksPerSecond>` —— `formatTickDuration`
 * - `strip|<text>` —— `stripColor`
 * - `trunc|<text>|<maxLength>|<addEllipsis>` —— `truncateStringIfNecessary`
 * - `lines|<text>` —— `lineCount`
 * - `trimChat|<text>` —— `trimChatMessage`
 * - `allowedChat|<codepoint>` —— `isAllowedChatCharacter`
 * - `playerName|<text>` —— `isValidPlayerName`
 * - `filter|<text>|<keepNewlines>` —— `filterText(String, boolean)`
 * - `whitespace|<codepoint>` —— `isWhitespace`
 * - `blank|<text>` —— `isBlank`
 *
 * 文本参数中的 `§` 直接以 UTF-8 字符给出；换行用字面量 `\n`（两字符转义）表示，
 * 由探针与 oracle 各自解码。**输出文本按 JSON 字符串转义**，两侧一致。
 */
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 把单元里的 `\n` / `\r` / `\v` / `\t` / `\uXXXX` 转义解码为真实字符。
std::string decodeEscapes(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 1 < text.size()) {
            switch (text[i + 1]) {
                case 'n':
                    out.push_back('\n');
                    ++i;
                    continue;
                case 'r':
                    out.push_back('\r');
                    ++i;
                    continue;
                case 'v':
                    out.push_back('\v');
                    ++i;
                    continue;
                case 't':
                    out.push_back('\t');
                    ++i;
                    continue;
                case 'u':
                case 'U':
                    if (i + 6 <= text.size()) {
                        const u32 codepoint = static_cast<u32>(std::stoul(text.substr(i + 2, 4), nullptr, 16));
                        if (codepoint < 0x80) {
                            out.push_back(static_cast<char>(codepoint));
                        } else if (codepoint < 0x800) {
                            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        } else {
                            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        }
                        i += 5;
                        continue;
                    }
                    break;
                default:
                    break;
            }
        }
        out.push_back(text[i]);
    }
    return out;
}

/// 原版 `Mth.floor(float)`。
i32 mthFloor(f32 value)
{
    const i32 truncated = static_cast<i32>(value);
    return value < static_cast<f32>(truncated) ? truncated - 1 : truncated;
}

/// 原版 `StringUtil.formatTickDuration`。
std::string formatTickDuration(i32 ticks, f32 ticksPerSecond)
{
    i32 i = mthFloor(static_cast<f32>(ticks) / ticksPerSecond);
    i32 j = i / 60;
    i %= 60;
    const i32 k = j / 60;
    j %= 60;

    char buffer[64];
    if (k > 0) {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", k, j, i);
    } else {
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", j, i);
    }
    return buffer;
}

/// 判断 `§` 后的字符是否属于 `[0-9A-FK-OR]`（不区分大小写）。
bool isColorCode(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return true;
    }
    const char upper = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - 'a' + 'A') : ch;
    switch (upper) {
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'R':
            return true;
        default:
            return false;
    }
}

/// 原版 `StringUtil.stripColor`：移除所有 `§` + 颜色码。
std::string stripColor(const std::string& text)
{
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        // `§` 在 UTF-8 中是 0xC2 0xA7。
        if (i + 2 < text.size() && static_cast<unsigned char>(text[i]) == 0xC2 &&
            static_cast<unsigned char>(text[i + 1]) == 0xA7 && isColorCode(text[i + 2])) {
            i += 2;
            continue;
        }
        out.push_back(text[i]);
    }
    return out;
}

/// 原版 `StringUtil.truncateStringIfNecessary`。
std::string truncateStringIfNecessary(const std::string& text, i32 maxLength, bool addEllipsis)
{
    if (static_cast<i32>(text.size()) <= maxLength) {
        return text;
    }
    if (addEllipsis && maxLength > 3) {
        return text.substr(0, static_cast<std::size_t>(maxLength - 3)) + "...";
    }
    return text.substr(0, static_cast<std::size_t>(maxLength));
}

/// 原版 `StringUtil.lineCount`：`LINE_PATTERN = "\r\n|\v"`，Java 正则的 `\v` 是垂直空白类。
i32 lineCount(const std::string& text)
{
    if (text.empty()) {
        return 0;
    }
    i32 count = 1;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
            ++count;
            ++i;
            continue;
        }
        // Java 正则的 \v 是「垂直空白字符类」，匹配 \n、垂直制表、\f、\r、\u0085、\u2028、\u2029。
        if (text[i] == '\n' || text[i] == '\v' || text[i] == '\f' || text[i] == '\r') {
            ++count;
            continue;
        }
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        if (lead == 0xC2 && i + 1 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0x85) {
            ++count;
            ++i;
            continue;
        }
        if (lead == 0xE2 && i + 2 < text.size() && static_cast<unsigned char>(text[i + 1]) == 0x80 &&
            (static_cast<unsigned char>(text[i + 2]) == 0xA8 || static_cast<unsigned char>(text[i + 2]) == 0xA9)) {
            ++count;
            i += 2;
        }
    }
    return count;
}

/// 原版 `StringUtil.isAllowedChatCharacter`。
bool isAllowedChatCharacter(i32 codepoint)
{
    return codepoint != 167 && codepoint >= 32 && codepoint != 127;
}

/// 把 UTF-8 字节串解码为码点序列（用于按 Java `int` 语义判断）。
std::vector<u32> decodeCodepoints(const std::string& text)
{
    std::vector<u32> codepoints;
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        if (lead < 0x80) {
            codepoints.push_back(lead);
            i += 1;
        } else if ((lead >> 5) == 0x6 && i + 1 < text.size()) {
            codepoints.push_back(((lead & 0x1FU) << 6) | (static_cast<unsigned char>(text[i + 1]) & 0x3FU));
            i += 2;
        } else if ((lead >> 4) == 0xE && i + 2 < text.size()) {
            codepoints.push_back(((lead & 0x0FU) << 12) | ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 6) |
                (static_cast<unsigned char>(text[i + 2]) & 0x3FU));
            i += 3;
        } else {
            codepoints.push_back(((lead & 0x07U) << 18) | ((static_cast<unsigned char>(text[i + 1]) & 0x3FU) << 12) |
                ((static_cast<unsigned char>(text[i + 2]) & 0x3FU) << 6) |
                (static_cast<unsigned char>(text[i + 3]) & 0x3FU));
            i += 4;
        }
    }
    return codepoints;
}

/// 原版 `StringUtil.isValidPlayerName`：`chars()` 迭代的是 UTF-16 码元。
bool isValidPlayerName(const std::string& text)
{
    // 长度按 UTF-16 码元数计：BMP 内 1 个，辅助平面 2 个（代理对）。
    i32 length = 0;
    for (u32 codepoint : decodeCodepoints(text)) {
        length += codepoint > 0xFFFF ? 2 : 1;
    }
    if (length > 16) {
        return false;
    }
    // `chars()` 逐码元过滤：辅助平面字符会产生两个代理码元（均 >= 127），因此非法。
    for (u32 codepoint : decodeCodepoints(text)) {
        if (codepoint > 0xFFFF) {
            return false;
        }
        if (codepoint <= 32 || codepoint >= 127) {
            return false;
        }
    }
    return true;
}

/// 原版 `StringUtil.filterText(String, boolean)`：逐码点过滤，非 ASCII 码点（>= 128）合法。
std::string filterText(const std::string& text, bool keepNewlines)
{
    std::string out;
    std::size_t offset = 0;
    for (u32 codepoint : decodeCodepoints(text)) {
        std::size_t width = 1;
        if (codepoint >= 0x10000) {
            width = 4;
        } else if (codepoint >= 0x800) {
            width = 3;
        } else if (codepoint >= 0x80) {
            width = 2;
        }
        if (isAllowedChatCharacter(static_cast<i32>(codepoint))) {
            out.append(text, offset, width);
        } else if (keepNewlines && codepoint == '\n') {
            out.push_back('\n');
        }
        offset += width;
    }
    return out;
}

/// 原版 `StringUtil.isWhitespace(int)`。
bool isWhitespace(i32 codepoint)
{
    switch (codepoint) {
        case 0x09: // \t
        case 0x0A: // \n
        case 0x0B: // \v
        case 0x0C: // \f
        case 0x0D: // \r
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
        case 0x20: // space
        case 0xA0: // NBSP (isSpaceChar)
        case 0x1680:
        case 0x2000:
        case 0x2001:
        case 0x2002:
        case 0x2003:
        case 0x2004:
        case 0x2005:
        case 0x2006:
        case 0x2007:
        case 0x2008:
        case 0x2009:
        case 0x200A:
        case 0x2028:
        case 0x2029:
        case 0x202F:
        case 0x205F:
        case 0x3000:
            return true;
        default:
            return false;
    }
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // duration：常规
        "duration|0|20",
        "duration|119|20",
        "duration|1200|20",
        "duration|3600|20",
        "duration|72000|20",
        // strip：颜色码
        "strip|\\u00A7ahello",
        "strip|\\u00A7a\\u00A7lhello",
        "strip|\\u00A7zhello",
        "strip|hello",
        "strip|\\u00A7A\\u00A7B\\u00A7C",
        // trunc：三个分支
        "trunc|abcdefgh|5|true",
        "trunc|abcdefgh|4|true",
        "trunc|abcdefgh|3|true",
        "trunc|abcdefgh|5|false",
        "trunc|abc|5|true",
        // lines
        "lines|",
        "lines|abc",
        "lines|a\\nb",
        "lines|a\\r\\nb",
        "lines|a\\vb",
        // trimChat
        "trimChat|hello",
        // allowedChat
        "allowedChat|65",
        "allowedChat|31",
        "allowedChat|127",
        "allowedChat|167",
        // playerName
        "playerName|Steve",
        "playerName|Steve123",
        "playerName|abcdefghijklmnopq",
        "playerName|abc def",
        "playerName|abc\\u00A7def",
        // filter
        "filter|ab\\u0001cd|false",
        "filter|ab\\ncd|false",
        "filter|ab\\ncd|true",
        // whitespace / blank
        "whitespace|32",
        "whitespace|9",
        "whitespace|65",
        "blank|   ",
        "blank|",
        "blank|abc",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("string_util: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "duration" && parts.size() == 3) {
        return json{
            {"unit", unit}, {"value", formatTickDuration(std::stoi(parts[1]), static_cast<f32>(std::stod(parts[2])))}}
            .dump();
    }
    if (mode == "strip" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", stripColor(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "trunc" && parts.size() == 4) {
        return json{{"unit", unit},
            {"value", truncateStringIfNecessary(decodeEscapes(parts[1]), std::stoi(parts[2]), parts[3] == "true")}}
            .dump();
    }
    if (mode == "lines" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", lineCount(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "trimChat" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", truncateStringIfNecessary(decodeEscapes(parts[1]), 256, false)}}.dump();
    }
    if (mode == "allowedChat" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", isAllowedChatCharacter(std::stoi(parts[1]))}}.dump();
    }
    if (mode == "playerName" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", isValidPlayerName(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "filter" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", filterText(decodeEscapes(parts[1]), parts[2] == "true")}}.dump();
    }
    if (mode == "whitespace" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", isWhitespace(std::stoi(parts[1]))}}.dump();
    }
    if (mode == "blank" && parts.size() == 2) {
        const std::string text = decodeEscapes(parts[1]);
        bool blank = true;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (!isWhitespace(static_cast<i32>(static_cast<unsigned char>(text[i])))) {
                blank = false;
                break;
            }
        }
        return json{{"unit", unit}, {"value", blank}}.dump();
    }

    throw std::runtime_error("string_util: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(string_util, &writeList, &runLine)

} // namespace mc::diff
