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
#include "common/util/StringUtils.hpp"

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
        return json{{"unit", unit},
            {"value", util::formatTickDuration(std::stoi(parts[1]), static_cast<f32>(std::stod(parts[2])))}}
            .dump();
    }
    if (mode == "strip" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::stripColor(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "trunc" && parts.size() == 4) {
        return json{{"unit", unit},
            {"value",
                util::truncateStringIfNecessary(decodeEscapes(parts[1]), std::stoi(parts[2]), parts[3] == "true")}}
            .dump();
    }
    if (mode == "lines" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::lineCount(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "trimChat" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::truncateStringIfNecessary(decodeEscapes(parts[1]), 256, false)}}
            .dump();
    }
    if (mode == "allowedChat" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::isAllowedChatCharacter(std::stoi(parts[1]))}}.dump();
    }
    if (mode == "playerName" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::isValidPlayerName(decodeEscapes(parts[1]))}}.dump();
    }
    if (mode == "filter" && parts.size() == 3) {
        return json{{"unit", unit}, {"value", util::filterText(decodeEscapes(parts[1]), parts[2] == "true")}}.dump();
    }
    if (mode == "whitespace" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", util::isWhitespace(std::stoi(parts[1]))}}.dump();
    }
    if (mode == "blank" && parts.size() == 2) {
        const std::string text = decodeEscapes(parts[1]);
        bool blank = true;
        for (std::size_t i = 0; i < text.size(); ++i) {
            if (!util::isWhitespace(static_cast<i32>(static_cast<unsigned char>(text[i])))) {
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
