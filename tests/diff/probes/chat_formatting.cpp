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
 * @brief 差分能力：`ChatFormatting` 的格式码解析与剥离（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.ChatFormatting`：
 *
 * ```java
 * public static final char PREFIX_CODE = '\u00a7';
 *
 * // 16 个颜色（id 0..15，带 RGB 值）+ 5 个格式（k/l/m/n/o，无 RGB）+ RESET（r，id -1）
 * BLACK("BLACK", '0', 0, 0)
 * ...
 * WHITE("WHITE", 'f', 15, 16777215)
 * OBFUSCATED("OBFUSCATED", 'k', true)
 * BOLD("BOLD", 'l', true)
 * STRIKETHROUGH("STRIKETHROUGH", 'm', true)
 * UNDERLINE("UNDERLINE", 'n', true)
 * ITALIC("ITALIC", 'o', true)
 * RESET("RESET", 'r', -1, null)
 *
 * private static final Pattern STRIP_FORMATTING_PATTERN = Pattern.compile("(?i)\u00a7[0-9A-FK-OR]");
 * private static String cleanName(String name) {
 *     return name.toLowerCase(Locale.ROOT).replaceAll("[^a-z]", "");
 * }
 *
 * public static String stripFormatting(String text) {
 *     return text == null ? null : STRIP_FORMATTING_PATTERN.matcher(text).replaceAll("");
 * }
 *
 * public static ChatFormatting getByCode(char code) {
 *     char lower = Character.toLowerCase(code);
 *     for (ChatFormatting f : values()) { if (f.code == lower) { return f; } }
 *     return null;
 * }
 *
 * public static ChatFormatting getById(int id) {
 *     if (id < 0) { return RESET; }
 *     for (ChatFormatting f : values()) { if (f.getId() == id) { return f; } }
 *     return null;
 * }
 *
 * public static ChatFormatting getByName(String name) {
 *     return name == null ? null : FORMATTING_BY_NAME.get(cleanName(name));
 * }
 *
 * public boolean isColor() { return !this.isFormat && this != RESET; }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`stripFormatting` 的字符集是 `[0-9A-FK-OR]`，不是「`§` 加任意字符」。**
 *
 *    这个正则**只认**：数字 `0-9`、`A-F`（颜色）、`K-O`（扁平化/粗体/删除线/下划线/斜体）、
 *    `R`（重置）。**不认 `G-J`、`P-Q`、`S-Z`** 等字母。
 *
 *    所以 `"§g"`、`"§z"`、`"§§"` 都**不会被剥离**，会原样保留。这与其他地方「剥掉所有 `§` 序列」
 *    的直觉不同，是数据校验的重要依据。
 *
 * 2. **正则带 `(?i)` 大小写不敏感，所以 `§A` 与 `§a` 都被剥离。**
 *
 * 3. **`getByCode` 先 `toLowerCase` 再匹配，因此 `'R'` 与 `'r'` 等价。**
 *
 *    但注意 `Character.toLowerCase` 只处理 ASCII 与部分 Unicode；本能力只用 ASCII 输入。
 *    找不到时返回 `null`（Java）而不是 `RESET`。
 *
 * 4. **`getById` 对负数一律返回 `RESET`，而不是 `null`。**
 *
 *    ```java
 *    if (id < 0) { return RESET; }
 *    ```
 *
 *    `RESET` 的 `id` 本身就是 `-1`，所以「负数 → RESET」与「id == -1 → RESET」在这里重合。
 *    但 `getById(-2)` **也会**返回 `RESET`，这是很多人会误以为返回 `null` 的地方。
 *
 *    正数找不到时返回 `null`，例如 `getById(16)`、`getById(100)`。
 *
 * 5. **`isColor()` 排除 `RESET`，`isFormat()` 对 `RESET` 为 `false`。**
 *
 *    `RESET` 既不是颜色也不是格式，两个判定都为 `false`。
 *
 * 单元格式：
 * - `strip|<文本>` —— 剥离格式码
 * - `byCode|<字符>` —— `getByCode`，输出枚举名或 `null`
 * - `byId|<id>` —— `getById`
 * - `byName|<名字>` —— `getByName`
 * - `info|<枚举名>` —— 输出 `code,id,isColor,isFormat,toString`
 *
 * `§` 用字面 UTF-8 字符传入（单元是 JSON 字符串，可直接承载）。
 *
 * TODO: 未覆盖 `getNames(boolean, boolean)`、`getName()` 的 Locale 行为、
 * 以及 `CODEC` / `CHARACTER_AND_FORMATTING_CODEC` 的序列化形式。
 */
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 一个格式项。
struct Formatting {
    const char* name;
    char code;
    i32 id;
    bool isFormat;
    bool hasColor;
};

/// 原版 `ChatFormatting` 的全部枚举值，顺序与 `values()` 一致。
constexpr Formatting ALL[] = {
    {"BLACK", '0', 0, false, true},
    {"DARK_BLUE", '1', 1, false, true},
    {"DARK_GREEN", '2', 2, false, true},
    {"DARK_AQUA", '3', 3, false, true},
    {"DARK_RED", '4', 4, false, true},
    {"DARK_PURPLE", '5', 5, false, true},
    {"GOLD", '6', 6, false, true},
    {"GRAY", '7', 7, false, true},
    {"DARK_GRAY", '8', 8, false, true},
    {"BLUE", '9', 9, false, true},
    {"GREEN", 'a', 10, false, true},
    {"AQUA", 'b', 11, false, true},
    {"RED", 'c', 12, false, true},
    {"LIGHT_PURPLE", 'd', 13, false, true},
    {"YELLOW", 'e', 14, false, true},
    {"WHITE", 'f', 15, false, true},
    {"OBFUSCATED", 'k', -1, true, false},
    {"BOLD", 'l', -1, true, false},
    {"STRIKETHROUGH", 'm', -1, true, false},
    {"UNDERLINE", 'n', -1, true, false},
    {"ITALIC", 'o', -1, true, false},
    {"RESET", 'r', -1, false, false},
};

/// `isColor()`：非格式且非 `RESET`。
bool isColor(const Formatting& f)
{
    return !f.isFormat && std::string(f.name) != "RESET";
}

/**
 * @brief 原版 `stripFormatting`：正则 `(?i)\u00a7[0-9A-FK-OR]` 的等价实现。
 *
 * `§` 在 UTF-8 下是两字节 `0xC2 0xA7`，因此按字节扫描时需匹配这两个字节。
 */
std::string stripFormatting(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    for (std::size_t i = 0; i < text.size();) {
        // 检查 UTF-8 形式的 `§`
        if (i + 2 < text.size() + 1 && i + 1 < text.size() && static_cast<unsigned char>(text[i]) == 0xC2 &&
            static_cast<unsigned char>(text[i + 1]) == 0xA7) {
            if (i + 2 < text.size()) {
                const char next = text[i + 2];
                const bool isColorDigit = next >= '0' && next <= '9';
                const bool isColorAlpha = (next >= 'a' && next <= 'f') || (next >= 'A' && next <= 'F');
                const bool isFormatChar = (next >= 'k' && next <= 'o') || (next >= 'K' && next <= 'O');
                const bool isReset = next == 'r' || next == 'R';
                if (isColorDigit || isColorAlpha || isFormatChar || isReset) {
                    // 整段跳过，反斜杠本身也不输出。
                    i += 3;
                    continue;
                }
            }
            // `§` 后不是可识别字符，保留 `§` 本身。
            result.push_back(text[i]);
            ++i;
            continue;
        }
        result.push_back(text[i]);
        ++i;
    }
    return result;
}

/// 原版 `getByCode`：先 `toLowerCase` 再匹配。
const Formatting* getByCode(char code)
{
    char lower = code;
    if (lower >= 'A' && lower <= 'Z') {
        lower = static_cast<char>(lower - 'A' + 'a');
    }
    for (const auto& f : ALL) {
        if (f.code == lower) {
            return &f;
        }
    }
    return nullptr;
}

/// 原版 `getById`：负数一律返回 RESET。
const Formatting* getById(i32 id)
{
    if (id < 0) {
        for (const auto& f : ALL) {
            if (std::string(f.name) == "RESET") {
                return &f;
            }
        }
        return nullptr;
    }
    for (const auto& f : ALL) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

/// 原版 `cleanName`：转小写并去掉所有非 `a-z` 字符。
std::string cleanName(const std::string& name)
{
    std::string result;
    result.reserve(name.size());
    for (char ch : name) {
        char lower = ch;
        if (lower >= 'A' && lower <= 'Z') {
            lower = static_cast<char>(lower - 'A' + 'a');
        }
        if (lower >= 'a' && lower <= 'z') {
            result.push_back(lower);
        }
    }
    return result;
}

/// 原版 `getByName`：按 `cleanName` 查表。
const Formatting* getByName(const std::string& name)
{
    const std::string cleaned = cleanName(name);
    for (const auto& f : ALL) {
        if (cleanName(f.name) == cleaned) {
            return &f;
        }
    }
    return nullptr;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // strip：识别全部合法的颜色码
        "strip|\xc2\xa7"
        "ahello",
        "strip|\xc2\xa7"
        "Ahello",
        "strip|\xc2\xa7"
        "fhello",
        "strip|\xc2\xa7"
        "0hello",
        "strip|\xc2\xa7"
        "9hello",
        // strip：格式码
        "strip|\xc2\xa7"
        "lhello",
        "strip|\xc2\xa7"
        "Lhello",
        "strip|\xc2\xa7"
        "khello",
        "strip|\xc2\xa7"
        "ohello",
        "strip|\xc2\xa7"
        "rhello",
        // strip：不认识的字符（不会被剥离）
        "strip|\xc2\xa7"
        "ghello",
        "strip|\xc2\xa7"
        "zhello",
        "strip|\xc2\xa7"
        " hello",
        "strip|\xc2\xa7",
        // strip：混合
        "strip|\xc2\xa7"
        "a\xc2\xa7"
        "lBold\xc2\xa7"
        "rPlain",
        "strip|no codes here",
        // byCode
        "byCode|a",
        "byCode|A",
        "byCode|0",
        "byCode|f",
        "byCode|r",
        "byCode|g",
        "byCode|1",
        // byId
        "byId|0",
        "byId|15",
        "byId|16",
        "byId|-1",
        "byId|-2",
        "byId|100",
        // byName
        "byName|black",
        "byName|BLACK",
        "byName|dark_blue",
        "byName|Dark-Blue",
        "byName|reset",
        "byName|not_a_color",
        // info
        "info|BLACK",
        "info|WHITE",
        "info|BOLD",
        "info|RESET",
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
        throw std::runtime_error("chat_formatting: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "strip" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", stripFormatting(parts[1])}}.dump();
    }
    if (mode == "byCode" && parts.size() == 2) {
        if (parts[1].empty()) {
            throw std::runtime_error("chat_formatting: byCode 需要非空字符");
        }
        const Formatting* found = getByCode(parts[1][0]);
        return json{{"unit", unit}, {"value", found ? std::string(found->name) : "null"}}.dump();
    }
    if (mode == "byId" && parts.size() == 2) {
        const Formatting* found = getById(std::stoi(parts[1]));
        return json{{"unit", unit}, {"value", found ? std::string(found->name) : "null"}}.dump();
    }
    if (mode == "byName" && parts.size() == 2) {
        const Formatting* found = getByName(parts[1]);
        return json{{"unit", unit}, {"value", found ? std::string(found->name) : "null"}}.dump();
    }
    if (mode == "info" && parts.size() == 2) {
        for (const auto& f : ALL) {
            if (std::string(f.name) == parts[1]) {
                const std::string value = std::string(1, f.code) + "," + std::to_string(f.id) + "," +
                    (isColor(f) ? "true" : "false") + "," + (f.isFormat ? "true" : "false") + ",\xc2\xa7" +
                    std::string(1, f.code);
                return json{{"unit", unit}, {"value", value}}.dump();
            }
        }
        return json{{"unit", unit}, {"value", "null"}}.dump();
    }

    throw std::runtime_error("chat_formatting: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(chat_formatting, &writeList, &runLine)

} // namespace mc::diff
