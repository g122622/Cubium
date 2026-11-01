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
 * @brief 差分能力：`Style` 的格式应用与合并（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": "<style>" }
 *
 * 复刻原版 `net.minecraft.network.chat.Style`：
 *
 * ```java
 * public Style applyFormat(ChatFormatting format) {
 *     TextColor color = this.color;
 *     Boolean bold = this.bold, italic = this.italic;
 *     Boolean strikethrough = this.strikethrough, underlined = this.underlined;
 *     Boolean obfuscated = this.obfuscated;
 *     switch (format) {
 *         case OBFUSCATED:    obfuscated = true; break;
 *         case BOLD:          bold = true; break;
 *         case STRIKETHROUGH: strikethrough = true; break;
 *         case UNDERLINE:     underlined = true; break;
 *         case ITALIC:        italic = true; break;
 *         case RESET:         return EMPTY;
 *         default:            color = TextColor.fromLegacyFormat(format);
 *     }
 *     return new Style(color, shadowColor, bold, italic, underlined, strikethrough,
 *                      obfuscated, clickEvent, hoverEvent, insertion, font);
 * }
 *
 * public Style applyLegacyFormat(ChatFormatting format) {
 *     // 与 applyFormat 相同，但 default 分支多清空五个格式标志：
 *     default:
 *         obfuscated = false; bold = false; strikethrough = false;
 *         underlined = false; italic = false;
 *         color = TextColor.fromLegacyFormat(format);
 * }
 *
 * public Style applyTo(Style other) {
 *     if (this == EMPTY) return other;
 *     if (other == EMPTY) return this;
 *     return new Style(
 *         this.color != null ? this.color : other.color,
 *         this.shadowColor != null ? this.shadowColor : other.shadowColor,
 *         this.bold != null ? this.bold : other.bold,
 *         ... 每个字段都是「this 非空则取 this，否则取 other」);
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`applyFormat` 与 `applyLegacyFormat` 的唯一区别是 `default`（颜色）分支。**
 *
 *    | 分支 | `applyFormat` | `applyLegacyFormat` |
 *    |---|---|---|
 *    | `OBFUSCATED` / `BOLD` / … | 置该位为 `true` | 同 |
 *    | `RESET` | 返回 `EMPTY` | 同 |
 *    | **颜色码（`default`）** | **只换颜色** | **清空全部 5 个格式标志再换颜色** |
 *
 *    **这是本能力最核心的语义**——`applyLegacyFormat` 模拟 `§` 传统格式码「换色即重置」的行为，
 *    而 `applyFormat` 不会。
 *
 * 2. **`RESET` 直接返回 `EMPTY`，丢弃所有其他属性。**
 *
 *    ```java
 *    case RESET: return EMPTY;
 *    ```
 *
 *    **不是**「把各字段设为 `false`」——`EMPTY` 的所有字段都是 `null`，包括 `clickEvent` /
 *    `hoverEvent` / `insertion` / `font`。
 *
 * 3. **`Style` 构造器的参数顺序是 `(color, shadowColor, bold, italic, underlined, strikethrough, obfuscated, ...)`。**
 *
 *    **`underlined` 在 `strikethrough` 之前**——这与 `applyFormat` 里的局部变量声明顺序
 *    （`strikethrough` 在 `underlined` 前）不同，极易传反。
 *
 * 4. **`applyTo` 的优先级是「`this` 胜利」。**
 *
 *    ```java
 *    this.color != null ? this.color : other.color
 *    ```
 *
 *    `this` 是「覆盖层」，`other` 是「基础层」。**`null` 表示「不设置」，会回落到 `other`**——
 *    这意味着无法用 `applyTo` 把某个字段**清空**（`null` 不是有效值）。
 *
 *    且有两个短路：`this == EMPTY` 返回 `other`；`other == EMPTY` 返回 `this`。
 *
 * 单元格式：
 * - 样式编码：`-` 表示 `EMPTY`；否则是 `color=<hex|name>;bold=<0|1>;italic=...` 的紧凑形式。
 *   为简化，本能力用固定 7 字段：`<color>|<bold>|<italic>|<underlined>|<strikethrough>|<obfuscated>|<insertion>`
 *   其中 `color` 为 `-` 或 6 位十六进制，布尔为 `-` / `0` / `1`，`insertion` 为 `-` 或文本。
 * - `applyFormat|<style 7 段>|<formatting>` —— `applyFormat`
 * - `applyLegacyFormat|<style 7 段>|<formatting>` —— `applyLegacyFormat`
 * - `applyTo|<style 7 段>|<style 7 段>` —— `applyTo`（前者是 `this`）
 *
 * `formatting` 取 `black`/`dark_blue`/…/`bold`/`italic`/`underline`/`strikethrough`/
 * `obfuscated`/`reset`。
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

/// `ChatFormatting` 的 22 个枚举项，按原版序数排列。
struct Formatting {
    const char* name;
    i32 colorValue; ///< -1 表示「不是颜色码」
    i32 isFormat;   ///< 是否为格式码
};

constexpr Formatting FORMATTINGS[] = {
    {"black", 0x000000, 0},
    {"dark_blue", 0x0000AA, 0},
    {"dark_green", 0x00AA00, 0},
    {"dark_aqua", 0x00AAAA, 0},
    {"dark_red", 0xAA0000, 0},
    {"dark_purple", 0xAA00AA, 0},
    {"gold", 0xFFAA00, 0},
    {"gray", 0xAAAAAA, 0},
    {"dark_gray", 0x555555, 0},
    {"blue", 0x5555FF, 0},
    {"green", 0x55FF55, 0},
    {"aqua", 0x55FFFF, 0},
    {"red", 0xFF5555, 0},
    {"light_purple", 0xFF55FF, 0},
    {"yellow", 0xFFFF55, 0},
    {"white", 0xFFFFFF, 0},
    {"obfuscated", -1, 1},
    {"bold", -1, 1},
    {"strikethrough", -1, 1},
    {"underline", -1, 1},
    {"italic", -1, 1},
    {"reset", -1, 1},
};

constexpr i32 FORMATTING_COUNT = 22;

/// 一个 `Style`（只保留与格式相关的 7 个字段）。
struct Style {
    bool hasColor = false;
    i32 color = 0;
    /// -1 表示 `null`，0 表示 `false`，1 表示 `true`。
    i32 bold = -1;
    i32 italic = -1;
    i32 underlined = -1;
    i32 strikethrough = -1;
    i32 obfuscated = -1;
    bool hasInsertion = false;
    std::string insertion;
};

bool isEmpty(const Style& style)
{
    return !style.hasColor && style.bold < 0 && style.italic < 0 && style.underlined < 0 && style.strikethrough < 0 &&
        style.obfuscated < 0 && !style.hasInsertion;
}

const Formatting* findFormatting(const std::string& name)
{
    for (const Formatting& formatting : FORMATTINGS) {
        if (name == formatting.name) {
            return &formatting;
        }
    }
    throw std::runtime_error("style_apply: 未知 ChatFormatting：" + name);
}

/// 原版 `Style.applyFormat`。
Style applyFormat(const Style& style, const Formatting& formatting)
{
    Style result = style;
    const std::string name = formatting.name;
    if (name == "obfuscated") {
        result.obfuscated = 1;
    } else if (name == "bold") {
        result.bold = 1;
    } else if (name == "strikethrough") {
        result.strikethrough = 1;
    } else if (name == "underline") {
        result.underlined = 1;
    } else if (name == "italic") {
        result.italic = 1;
    } else if (name == "reset") {
        return Style{};
    } else {
        result.hasColor = true;
        result.color = formatting.colorValue;
    }
    return result;
}

/// 原版 `Style.applyLegacyFormat`：颜色码会清空全部格式标志。
Style applyLegacyFormat(const Style& style, const Formatting& formatting)
{
    Style result = style;
    const std::string name = formatting.name;
    if (name == "obfuscated") {
        result.obfuscated = 1;
    } else if (name == "bold") {
        result.bold = 1;
    } else if (name == "strikethrough") {
        result.strikethrough = 1;
    } else if (name == "underline") {
        result.underlined = 1;
    } else if (name == "italic") {
        result.italic = 1;
    } else if (name == "reset") {
        return Style{};
    } else {
        // 与 applyFormat 的唯一区别：把五个格式标志置为 **false**（不是 null）。
        // false 会覆盖继承来的 true，而 null 不会——这是关键差异。
        result.obfuscated = 0;
        result.bold = 0;
        result.strikethrough = 0;
        result.underlined = 0;
        result.italic = 0;
        result.hasColor = true;
        result.color = formatting.colorValue;
    }
    return result;
}

/// 原版 `Style.applyTo`：`this` 的非空字段优先。
Style applyTo(const Style& self, const Style& other)
{
    if (isEmpty(self)) {
        return other;
    }
    if (isEmpty(other)) {
        return self;
    }
    Style result;
    result.hasColor = self.hasColor || other.hasColor;
    result.color = self.hasColor ? self.color : other.color;
    result.bold = self.bold >= 0 ? self.bold : other.bold;
    result.italic = self.italic >= 0 ? self.italic : other.italic;
    result.underlined = self.underlined >= 0 ? self.underlined : other.underlined;
    result.strikethrough = self.strikethrough >= 0 ? self.strikethrough : other.strikethrough;
    result.obfuscated = self.obfuscated >= 0 ? self.obfuscated : other.obfuscated;
    result.hasInsertion = self.hasInsertion || other.hasInsertion;
    result.insertion = self.hasInsertion ? self.insertion : other.insertion;
    return result;
}

std::string renderBool(i32 value)
{
    return value < 0 ? "-" : (value == 0 ? "0" : "1");
}

std::string render(const Style& style)
{
    std::string color = "-";
    if (style.hasColor) {
        char buffer[8];
        std::snprintf(buffer, sizeof(buffer), "%06x", style.color);
        color = buffer;
    }
    return color + "|" + renderBool(style.bold) + "|" + renderBool(style.italic) + "|" + renderBool(style.underlined) +
        "|" + renderBool(style.strikethrough) + "|" + renderBool(style.obfuscated) + "|" +
        (style.hasInsertion ? style.insertion : "-");
}

Style parseStyle(const std::vector<std::string>& parts, std::size_t offset)
{
    Style style;
    if (parts[offset] != "-") {
        style.hasColor = true;
        style.color = static_cast<i32>(std::stoll(parts[offset], nullptr, 16));
    }
    const auto parseTri = [](const std::string& text) -> i32 {
        if (text == "-") {
            return -1;
        }
        return text == "1" ? 1 : 0;
    };
    style.bold = parseTri(parts[offset + 1]);
    style.italic = parseTri(parts[offset + 2]);
    style.underlined = parseTri(parts[offset + 3]);
    style.strikethrough = parseTri(parts[offset + 4]);
    style.obfuscated = parseTri(parts[offset + 5]);
    if (parts[offset + 6] != "-") {
        style.hasInsertion = true;
        style.insertion = parts[offset + 6];
    }
    return style;
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // applyFormat：格式码置 true
        "applyFormat|-|-|-|-|-|-|-|bold",
        "applyFormat|ff0000|-|-|-|-|-|-|bold",
        // applyFormat：颜色码只换色，保留格式
        "applyFormat|ff0000|1|1|-|-|-|-|green",
        "applyFormat|-|1|1|1|1|1|-|green",
        // applyFormat：RESET 返回 EMPTY
        "applyFormat|ff0000|1|1|1|1|1|hello|reset",
        // applyLegacyFormat：颜色码清空格式（与 applyFormat 对比）
        "applyLegacyFormat|ff0000|1|1|1|1|1|-|green",
        "applyLegacyFormat|-|1|1|1|1|1|-|green",
        // applyLegacyFormat：格式码行为相同
        "applyLegacyFormat|ff0000|-|-|-|-|-|-|bold",
        // applyLegacyFormat：RESET
        "applyLegacyFormat|ff0000|1|1|1|1|1|hello|reset",
        // applyTo：this 优先
        "applyTo|ff0000|1|-|-|-|-|-|-|0|1|-|-|-|-",
        // applyTo：this 空返回 other
        "applyTo|-|-|-|-|-|-|-|ff0000|1|-|-|-|-|-",
        // applyTo：other 空返回 this
        "applyTo|ff0000|0|-|-|-|-|-|-|-|-|-|-|-|-",
        // applyTo：双方都空
        "applyTo|-|-|-|-|-|-|-|-|-|-|-|-|-|-",
        // applyTo：insertion 也遵循 this 优先
        "applyTo|-|-|-|-|-|-|aaa|-|-|-|-|-|-|bbb",
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

    if (parts.size() == 1 && parts[0] == "empty") {
        return json{{"unit", unit}, {"value", render(Style{})}}.dump();
    }
    if (parts.size() != 9 && parts.size() != 15) {
        throw std::runtime_error("style_apply: 单元段数不符：" + unit);
    }

    const std::string mode = parts[0];
    if (mode == "applyFormat" || mode == "applyLegacyFormat") {
        const Style style = parseStyle(parts, 1);
        const Formatting* formatting = findFormatting(parts[8]);
        const Style result =
            mode == "applyFormat" ? applyFormat(style, *formatting) : applyLegacyFormat(style, *formatting);
        return json{{"unit", unit}, {"value", render(result)}}.dump();
    }
    if (mode == "applyTo" && parts.size() == 15) {
        const Style self = parseStyle(parts, 1);
        const Style other = parseStyle(parts, 8);
        return json{{"unit", unit}, {"value", render(applyTo(self, other))}}.dump();
    }

    throw std::runtime_error("style_apply: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("style_apply", &writeList, &runLine)

} // namespace mc::diff
