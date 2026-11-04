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
 * @brief 差分能力：`ChatFormatting` 的名字双向映射。
 *
 * **本探针调用 Cubium 的真实实现**（`mc::text::toName` / `fromName`）。
 *
 * Cubium 侧：
 *
 * ```cpp
 * [[nodiscard]] TextFormatting fromName(const std::string& name) noexcept;  // 大小写不敏感
 * [[nodiscard]] std::string toName(TextFormatting formatting);              // 未知返回 ""
 * ```
 *
 * 原版侧：
 *
 * ```java
 * public String getName() { return this.name.toLowerCase(Locale.ROOT); }
 * public String getSerializedName() { return this.getName(); }      // 两者相同
 * public static Collection<String> getNames(boolean colors, boolean formats);
 * ```
 *
 * ## 四个关键点
 *
 * 1. **原版 `getName()` 与 `getSerializedName()` 返回同一值。**
 *
 *    两者都是 `name.toLowerCase(Locale.ROOT)`——**没有 PascalCase 或大写形式**。
 *
 * 2. **名字是 snake_case，与枚举名的大小写不同。**
 *
 *    | 枚举 | `getName()` |
 *    |---|---|
 *    | `DARK_BLUE` | `dark_blue` |
 *    | `LIGHT_PURPLE` | `light_purple` |
 *    | `DARK_AQUA` | `dark_aqua` |
 *
 *    **不是 `darkBlue`，也不是 `DARK_BLUE`**。
 *
 * 3. **`fromName` 大小写不敏感，原版 `getNames` 返回的是小写名。**
 *
 *    Cubium 的 `fromName` 先 `tolower` 再查表——因此 `"DARK_BLUE"` / `"Dark_Blue"` /
 *    `"dark_blue"` 全部命中。
 *
 *    **原版没有 `fromName` 这个入口**（只有 `getByCode`），所以 `fromName` 的正向行为
 *    以「能还原 `getName()` 的结果」为准——本能力用 `fromName(toName(x)) == x` 的
 *    往返性来验证。
 *
 * 4. **`toName` 对 `None`（`255`）返回空串，原版无此枚举项。**
 *
 *    `TextFormatting::None` 是 Cubium 特有的哨兵值，不在原版的 22 项之中。
 *
 * 单元格式：
 * - `name|<formatting>` —— Cubium `toName` vs 原版 `getName`
 * - `roundTrip|<formatting>` —— `fromName(toName(x)) == x` 是否为真
 * - `fromName|<text>` —— `fromName(text)` 的枚举名，未命中输出 `NONE`
 * - `names` —— 原版 `getNames(true, true)` 的全部小写名，逗号分隔
 * - `count` —— 真实枚举项数（不含 `None`）
 */
#include <cstdio>
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

/// 22 个真实的 `ChatFormatting`。
struct FormattingName {
    const char* javaName;
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

/// 把 Cubium 的枚举值转回原版枚举名。
std::string toJavaName(text::TextFormatting formatting)
{
    for (const FormattingName& entry : FORMATTINGS) {
        if (entry.cubium == formatting) {
            return entry.javaName;
        }
    }
    return "NONE";
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // name：全部 22 项
        "name|BLACK",
        "name|DARK_BLUE",
        "name|DARK_GREEN",
        "name|DARK_AQUA",
        "name|DARK_RED",
        "name|DARK_PURPLE",
        "name|GOLD",
        "name|GRAY",
        "name|DARK_GRAY",
        "name|BLUE",
        "name|GREEN",
        "name|AQUA",
        "name|RED",
        "name|LIGHT_PURPLE",
        "name|YELLOW",
        "name|WHITE",
        "name|OBFUSCATED",
        "name|BOLD",
        "name|STRIKETHROUGH",
        "name|UNDERLINE",
        "name|ITALIC",
        "name|RESET",
        // roundTrip：toName 与 fromName 的往返性
        "roundTrip|BLACK",
        "roundTrip|DARK_BLUE",
        "roundTrip|LIGHT_PURPLE",
        "roundTrip|BOLD",
        "roundTrip|RESET",
        // fromName：大小写不敏感
        "fromName|dark_blue",
        "fromName|DARK_BLUE",
        "fromName|Dark_Blue",
        "fromName|bold",
        "fromName|reset",
        // fromName：未命中
        "fromName|darkBlue",
        "fromName|not_a_color",
        "fromName|",
        // names / count
        "names",
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
        throw std::runtime_error("text_formatting_name: 空单元");
    }

    const std::string mode = parts[0];
    std::string value;

    if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const FormattingName& entry : FORMATTINGS) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += text::toName(entry.cubium);
        }
        value = names;
    } else if (mode == "count" && parts.size() == 1) {
        value = std::to_string(FORMATTING_COUNT);
    } else if (parts.size() != 2) {
        throw std::runtime_error("text_formatting_name: 单元段数不符：" + unit);
    } else if (mode == "name") {
        const FormattingName* entry = findFormatting(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("text_formatting_name: 未知格式：" + parts[1]);
        }
        value = text::toName(entry->cubium);
    } else if (mode == "roundTrip") {
        const FormattingName* entry = findFormatting(parts[1]);
        if (entry == nullptr) {
            throw std::runtime_error("text_formatting_name: 未知格式：" + parts[1]);
        }
        // 往返：先取名字，再按名字反查，应还原为同一枚举值。
        const text::TextFormatting back = text::fromName(text::toName(entry->cubium));
        value = back == entry->cubium ? "true" : "false";
    } else if (mode == "fromName") {
        value = toJavaName(text::fromName(parts[1]));
    } else {
        throw std::runtime_error("text_formatting_name: 无法识别的单元：" + unit);
    }

    return json{{"unit", unit}, {"value", value}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(text_formatting_name, &writeList, &runLine)

} // namespace mc::diff
