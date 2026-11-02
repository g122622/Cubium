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
 * @brief 差分能力：`ChatFormatting` 的颜色 / 码位 / 分类查询。
 *
 * **本探针调用 Cubium 的真实实现**（`common/util/text/TextStyle.hpp` 与
 * `TextStyle.cpp`），而非手抄原版逻辑——这样比对的是「原版 vs Cubium」，
 * 而不是「原版 vs 我的转写」。
 *
 * Cubium 侧接口：
 *
 * ```cpp
 * enum class TextFormatting : u8 { Black = 0, ..., Reset = 21, None = 255 };
 * u32  getFormattingColor(TextFormatting formatting) noexcept;   // ARGB
 * bool isColor(TextFormatting formatting) noexcept;
 * bool isStyle(TextFormatting formatting) noexcept;
 * TextFormatting fromCode(char code) noexcept;
 * char toCode(TextFormatting formatting) noexcept;
 * ```
 *
 * 原版侧对应：
 *
 * ```java
 * public int getColor();            // @Nullable Integer，格式码为 null
 * public boolean isColor();
 * public boolean isFormat();
 * public char getChar();            // '0'..'f', 'k','l','m','n','o','r'
 * public static ChatFormatting getByCode(char code);   // 未知名返回 null
 * ```
 *
 * ## 四个关键点
 *
 * 1. **格式码的颜色：Cubium 返回 `0xFFFFFFFF`（白色），原版返回 `null`。**
 *
 *    ```cpp
 *    // TextStyle.cpp
 *    {TextFormatting::Bold, 'l', "bold", 0xFFFFFFFF},
 *    return info ? info->color : 0xFFFFFFFF;
 *    ```
 *
 *    16 个颜色码的值与原版完全一致，但 6 个格式码（`obfuscated` / `bold` /
 *    `strikethrough` / `underline` / `italic` / `reset`）原版是 `null`，Cubium 是白色。
 *
 *    **这是表示法差异**：Cubium 用 `u32` 无法表达 `null`，退化为「白色哨兵值」。
 *    是否构成缺陷取决于调用方是否判断 `isColor()`——`GlowEffect.cpp` 里确实配合
 *    `isColor()` 使用，因此**当前不致错**，但接口语义与原版不同。
 *
 * 2. **`isColor` / `isStyle` 是范围判断，不是查表。**
 *
 *    ```cpp
 *    bool isColor(TextFormatting f) { return static_cast<u8>(f) <= static_cast<u8>(White); }
 *    bool isStyle(TextFormatting f) { return v >= Obfuscated && v <= Italic; }
 *    ```
 *
 *    注意 `isStyle` 的上界是 **`Italic`（20）**，**不含 `Reset`（21）**——
 *    `Reset` 既不是颜色也不是样式。
 *
 * 3. **`fromCode` 对未知字符返回 `None`（`255`），原版 `getByCode` 返回 `null`。**
 *
 *    同样是「用哨兵值代替 `null`」的表示法差异。
 *
 * 4. **`fromCode` 会做 `tolower`，因此 `'A'` 与 `'a'` 等价。**
 *
 *    原版的 `getByCode` 也是先 `toLowerCase`。
 *
 * 单元格式：
 * - `color|<formatting>` —— Cubium `findFormattingARGB` vs 原版 `getColor`
 * - `isColor|<formatting>` —— 两版 `isColor`
 * - `isStyle|<formatting>` —— Cubium `isStyle` vs 原版 `isFormat`
 * - `code|<formatting>` —— `toCode` vs `getChar`
 * - `fromCode|<char>` —— `fromCode` vs `getByCode`
 * - `count` —— 枚举项数（Cubium 含 `None`，需排除）
 */
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/text/TextStyle.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 22 个真实的 `ChatFormatting`（不含 Cubium 特有的 `None` 哨兵）。
struct FormattingName {
    const char* javaName; ///< 原版枚举名
    text::TextFormatting cubium;
};

constexpr FormattingName FORMATTINGS[] = {
    {"BLACK", text::TextFormatting::Black},
    {"DARK_BLUE", text::TextFormatting::DarkBlue},
    {"DARK_GREEN", text::TextFormatting::DarkGreen},
    {"DARK_AQUA", text::TextFormatting::DarkAqua},
    {"DARK_RED", text::TextFormatting::DarkRed},
    {"DARK_PURPLE", text::TextFormatting::DarkPurple},
    {"GOLD", text::TextFormatting::Gold},
    {"GRAY", text::TextFormatting::Gray},
    {"DARK_GRAY", text::TextFormatting::DarkGray},
    {"BLUE", text::TextFormatting::Blue},
    {"GREEN", text::TextFormatting::Green},
    {"AQUA", text::TextFormatting::Aqua},
    {"RED", text::TextFormatting::Red},
    {"LIGHT_PURPLE", text::TextFormatting::LightPurple},
    {"YELLOW", text::TextFormatting::Yellow},
    {"WHITE", text::TextFormatting::White},
    {"OBFUSCATED", text::TextFormatting::Obfuscated},
    {"BOLD", text::TextFormatting::Bold},
    {"STRIKETHROUGH", text::TextFormatting::Strikethrough},
    {"UNDERLINE", text::TextFormatting::Underline},
    {"ITALIC", text::TextFormatting::Italic},
    {"RESET", text::TextFormatting::Reset},
};

constexpr i32 FORMATTING_COUNT = 22;

const FormattingName* findFormatting(const std::string& javaName)
{
    for (const FormattingName& entry : FORMATTINGS) {
        if (javaName == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // color：16 个颜色码（应与原版一致）
        "color|BLACK",
        "color|DARK_BLUE",
        "color|DARK_GREEN",
        "color|DARK_AQUA",
        "color|DARK_RED",
        "color|DARK_PURPLE",
        "color|GOLD",
        "color|GRAY",
        "color|DARK_GRAY",
        "color|BLUE",
        "color|GREEN",
        "color|AQUA",
        "color|RED",
        "color|LIGHT_PURPLE",
        "color|YELLOW",
        "color|WHITE",
        // color：6 个格式码（原版 null，预期差异点）
        "color|OBFUSCATED",
        "color|BOLD",
        "color|STRIKETHROUGH",
        "color|UNDERLINE",
        "color|ITALIC",
        "color|RESET",
        // isColor
        "isColor|BLACK",
        "isColor|WHITE",
        "isColor|BOLD",
        "isColor|RESET",
        // isStyle
        "isStyle|OBFUSCATED",
        "isStyle|ITALIC",
        "isStyle|RESET",
        "isStyle|BLACK",
        // code
        "code|BLACK",
        "code|GREEN",
        "code|WHITE",
        "code|BOLD",
        "code|RESET",
        // fromCode
        "fromCode|0",
        "fromCode|a",
        "fromCode|f",
        "fromCode|k",
        "fromCode|r",
        "fromCode|A",
        "fromCode|z",
        "fromCode|q",
        // count
        "count",
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
    if (parts.empty()) {
        throw std::runtime_error("text_formatting_color: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "count" && parts.size() == 1) {
        value = std::to_string(FORMATTING_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("text_formatting_color: 单元段数不符：" + unit);
    } else if (mode == "fromCode") {
        // Cubium 侧的 fromCode 是真实实现；这里把结果转回原版枚举名。
        const text::TextFormatting converted = text::fromCode(parts[1].empty() ? '\0' : parts[1][0]);
        value = "NONE";
        for (const FormattingName& entry : FORMATTINGS) {
            if (entry.cubium == converted) {
                value = entry.javaName;
                break;
            }
        }
    } else {
        const FormattingName* entry = findFormatting(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("text_formatting_color: 未知格式：" + parts[1]);
        }
        if (mode == "color") {
            // Cubium 的真实实现：findFormattingARGB 精确对应原版 getColor() 的 null 语义。
            const std::optional<u32> argb = text::findFormattingARGB(entry->cubium);
            if (!argb.has_value()) {
                value = "null";
            } else {
                char buffer[16];
                std::snprintf(buffer, sizeof(buffer), "%08X", *argb);
                value = buffer;
            }
        } else if (mode == "isColor") {
            value = text::isColor(entry->cubium) ? "true" : "false";
        } else if (mode == "isStyle") {
            value = text::isStyle(entry->cubium) ? "true" : "false";
        } else if (mode == "code") {
            const char code = text::toCode(entry->cubium);
            value = std::string(1, code);
        } else {
            throw std::runtime_error("text_formatting_color: 无法识别的单元：" + unit);
        }
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("text_formatting_color", &writeList, &runLine)

} // namespace mc::diff
