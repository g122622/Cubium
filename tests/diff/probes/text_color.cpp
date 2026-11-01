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
 * @brief 差分能力：`TextColor` 的解析与序列化（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.network.chat.TextColor`：
 *
 * ```java
 * private TextColor(int value) { this.value = value & 16777215; this.name = null; }
 * private TextColor(int value, String name) { this.value = value & 16777215; this.name = name; }
 *
 * public static TextColor fromRgb(int rgb) { return new TextColor(rgb); }
 *
 * public int getValue() { return this.value; }
 *
 * public String serialize() { return this.name != null ? this.name : this.formatValue(); }
 * private String formatValue() { return String.format(Locale.ROOT, "#%06X", this.value); }
 *
 * public static DataResult<TextColor> parseColor(String text) {
 *     if (text.startsWith("#")) {
 *         try {
 *             int i = Integer.parseInt(text.substring(1), 16);
 *             return i >= 0 && i <= 16777215
 *                 ? DataResult.success(fromRgb(i), Lifecycle.stable())
 *                 : DataResult.error(() -> "Color value out of range: " + text);
 *         } catch (NumberFormatException exception) {
 *             return DataResult.error(() -> "Invalid color value: " + text);
 *         }
 *     }
 *     TextColor named = NAMED_COLORS.get(text);
 *     return named == null
 *         ? DataResult.error(() -> "Invalid color name: " + text)
 *         : DataResult.success(named, Lifecycle.stable());
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`parseColor` 的 `#` 分支只有 6 位十六进制，没有 `#RGB` 缩写。**
 *
 *    `Integer.parseInt("f00", 16)` 得 `3840`（合法），所以 `#f00` **被接受**，但解释成
 *    **`0x000F00`**（暗绿），**不是** RGB 缩写 `0xFF0000`（红）。
 *
 *    这是极易踩的坑：`#f00` 不是「红色简写」，而是「十六进制 0xF00」。
 *
 * 2. **`NAMED_COLORS` 只包含 16 个传统颜色名，且名字是 `ChatFormatting.getName()`（小写）。**
 *
 *    | 名字 | 值 |
 *    |---|---|
 *    | `black` | `0x000000` |
 *    | `dark_blue` | `0x0000AA` |
 *    | `dark_green` | `0x00AA00` |
 *    | `dark_aqua` | `0x00AAAA` |
 *    | `dark_red` | `0xAA0000` |
 *    | `dark_purple` | `0xAA00AA` |
 *    | `gold` | `0xFFAA00` |
 *    | `gray` | `0xAAAAAA` |
 *    | `dark_gray` | `0x555555` |
 *    | `blue` | `0x5555FF` |
 *    | `green` | `0x55FF55` |
 *    | `aqua` | `0x55FFFF` |
 *    | `red` | `0xFF5555` |
 *    | `light_purple` | `0xFF55FF` |
 *    | `yellow` | `0xFFFF55` |
 *    | `white` | `0xFFFFFF` |
 *
 *    **`bold` / `italic` / `reset` 等格式名不在表里**——它们没有颜色值。
 *
 * 3. **构造器用 `& 16777215` 截断，但 `parseColor` 的范围检查在截断之前。**
 *
 *    `fromRgb(0xFF123456)` 得 `0x123456`（截断），而 `parseColor("#FF123456")` 会失败
 *    （`parseInt` 溢出）。**两条路径的行为不同。**
 *
 * 4. **`serialize()` 对具名颜色返回名字，对 `fromRgb` 返回 `#RRGGBB`（6 位大写）。**
 *
 *    ```java
 *    return this.name != null ? this.name : this.formatValue();
 *    ```
 *
 *    `parseColor("#ff0000")` 得到无名 TextColor → `serialize()` 返回 **`#FF0000`**（大写）。
 *    `parseColor("red")` 得到具名 TextColor → `serialize()` 返回 **`red`**（原名）。
 *
 *    所以 **`parseColor(x).serialize()` 不等于 `x`**。
 *
 * 单元格式：
 * - `parse|<text>` —— `parseColor`，输出 `"<value_hex>|<serialize>"` 或 `"<error>"`
 * - `fromRgb|<int>` —— `fromRgb` + `serialize`
 * - `match|<a>|<b>` —— 两个颜色是否 `equals`（`parseColor` 后比较）
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

constexpr i32 COLOR_MASK = 16777215; // 0xFFFFFF

/// 16 个传统颜色名（`ChatFormatting.getName()` 小写）与值。
struct NamedColor {
    const char* name;
    i32 value;
};

constexpr NamedColor NAMED_COLORS[] = {
    {"black", 0x000000},
    {"dark_blue", 0x0000AA},
    {"dark_green", 0x00AA00},
    {"dark_aqua", 0x00AAAA},
    {"dark_red", 0xAA0000},
    {"dark_purple", 0xAA00AA},
    {"gold", 0xFFAA00},
    {"gray", 0xAAAAAA},
    {"dark_gray", 0x555555},
    {"blue", 0x5555FF},
    {"green", 0x55FF55},
    {"aqua", 0x55FFFF},
    {"red", 0xFF5555},
    {"light_purple", 0xFF55FF},
    {"yellow", 0xFFFF55},
    {"white", 0xFFFFFF},
};

constexpr i32 NAMED_COLOR_COUNT = 16;

/// 一个 `TextColor`。
struct TextColor {
    i32 value = 0;
    /// 具名颜色的名字；无名则为 `nullptr`。
    const char* name = nullptr;
    bool valid = false;
};

/// 原版 `TextColor.fromRgb`。
TextColor fromRgb(i32 rgb)
{
    TextColor color;
    color.value = rgb & COLOR_MASK;
    color.valid = true;
    return color;
}

/// 原版 `TextColor.serialize`。
std::string serialize(const TextColor& color)
{
    if (color.name != nullptr) {
        return color.name;
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "#%06X", static_cast<unsigned>(color.value));
    return buffer;
}

/// 原版 `Integer.parseInt(text, 16)`：失败返回 `false`。
bool parseHexInt(const std::string& text, i32& out)
{
    if (text.empty()) {
        return false;
    }
    // Java 的 parseInt 允许前导 '+' / '-'。
    std::size_t index = 0;
    bool negative = false;
    if (text[0] == '-' || text[0] == '+') {
        negative = text[0] == '-';
        index = 1;
        if (text.size() == 1) {
            return false;
        }
    }
    i64 accumulated = 0;
    for (; index < text.size(); ++index) {
        const char ch = text[index];
        i32 digit = -1;
        if (ch >= '0' && ch <= '9') {
            digit = ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            digit = ch - 'a' + 10;
        } else if (ch >= 'A' && ch <= 'F') {
            digit = ch - 'A' + 10;
        } else {
            return false;
        }
        accumulated = accumulated * 16 + digit;
        if (accumulated > 0x7FFFFFFFLL + 1) {
            return false;
        }
    }
    const i64 signedValue = negative ? -accumulated : accumulated;
    if (signedValue < -2147483648LL || signedValue > 2147483647LL) {
        return false;
    }
    out = static_cast<i32>(signedValue);
    return true;
}

/// 原版 `TextColor.parseColor`。
TextColor parseColor(const std::string& text)
{
    if (!text.empty() && text[0] == '#') {
        i32 parsed = 0;
        if (!parseHexInt(text.substr(1), parsed)) {
            return TextColor{0, nullptr, false};
        }
        if (parsed < 0 || parsed > COLOR_MASK) {
            return TextColor{0, nullptr, false};
        }
        return fromRgb(parsed);
    }
    for (i32 i = 0; i < NAMED_COLOR_COUNT; ++i) {
        if (text == NAMED_COLORS[i].name) {
            TextColor color;
            color.value = NAMED_COLORS[i].value & COLOR_MASK;
            color.name = NAMED_COLORS[i].name;
            color.valid = true;
            return color;
        }
    }
    return TextColor{0, nullptr, false};
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // parse：# 分支
        "parse|#ff0000",
        "parse|#FF0000",
        "parse|#000000",
        "parse|#ffffff",
        "parse|#f00",
        "parse|#0",
        // parse：# 分支的边界与失败
        "parse|#1000000",
        "parse|#ffffff",
        "parse|#",
        "parse|#gggggg",
        "parse|#-1",
        "parse|#+ff",
        // parse：命名颜色
        "parse|red",
        "parse|black",
        "parse|white",
        "parse|light_purple",
        "parse|dark_gray",
        // parse：非法名字
        "parse|RED",
        "parse|bold",
        "parse|reset",
        "parse|",
        // fromRgb：截断
        "fromRgb|16711680",
        "fromRgb|-1",
        "fromRgb|16777216",
        "fromRgb|4294967295",
        // match：具名与十六进制值相同
        "match|red|#ff5555",
        "match|red|red",
        "match|red|#ff0000",
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

std::string renderColor(const TextColor& color)
{
    if (!color.valid) {
        return "<error>";
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%06X", static_cast<unsigned>(color.value));
    return std::string(buffer) + "|" + serialize(color);
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("text_color: 单元至少两段：" + unit);
    }

    const std::string mode = parts[0];

    if (mode == "parse" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", renderColor(parseColor(parts[1]))}}.dump();
    }
    if (mode == "fromRgb" && parts.size() == 2) {
        const i32 rgb = static_cast<i32>(std::stoll(parts[1]));
        return json{{"unit", unit}, {"value", renderColor(fromRgb(rgb))}}.dump();
    }
    if (mode == "match" && parts.size() == 3) {
        const TextColor a = parseColor(parts[1]);
        const TextColor b = parseColor(parts[2]);
        const bool equal = a.valid && b.valid && a.value == b.value;
        return json{{"unit", unit}, {"value", equal}}.dump();
    }

    throw std::runtime_error("text_color: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("text_color", &writeList, &runLine)

} // namespace mc::diff
