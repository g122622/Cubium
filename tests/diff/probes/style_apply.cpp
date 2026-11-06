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
 */

/**
 * @file style_apply.cpp
 * @brief 差分能力：Style 的三种格式化套用方法（applyFormat / applyLegacyFormat / applyTo）。
 *
 * 本探针调用生产代码 text::Style，不在探针里重写这些方法的逻辑。
 *
 * ## 关键点
 *
 * 1. applyFormat 与 applyLegacyFormat 只在颜色码分支不同。前者保留已有的加粗、斜体等
 *    标志；后者把它们全部置 false——§ 语义下换颜色等同开新段落。
 *
 *    这个区别只在「先加粗、再换颜色」这种组合下才暴露。单独测每个格式码永远发现不了，
 *    所以用例里必须成对出现。
 *
 * 2. RESET 返回 EMPTY，不是「清空后的原样式」。两者在 applyTo 里行为不同：
 *    EMPTY.applyTo(x) 返回 x（短路），而「样式位全为 false 的样式」不算空、不短路。
 *
 * 3. applyTo 的两处短路只判 isEmpty()。未设置与显式 false 是两种值。
 *
 * 4. 样式码分支两者行为完全一致——都只把对应标志置 true，不动其他。
 *
 * ## 单元格式
 *
 * applyFormat|<颜色>|<bold>|<italic>|<underlined>|<strikethrough>|<obfuscated>|<insertion>|<格式码>
 *
 * - 颜色：- 表示未设置，否则 6 位十六进制；
 * - 三态标志：- = 未设置，0 = 显式 false，1 = 显式 true；
 * - insertion：- 表示未设置。
 *
 * applyTo 把两组（每组 7 段）串起来，共 15 段。
 */

#include <cstdio>
#include <optional>
#include <stdexcept>
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
using text::Style;
using text::TextColor;
using text::TextFormatting;

struct FormattingEntry {
    const char* name;
    TextFormatting formatting;
};

constexpr FormattingEntry FORMATTINGS[] = {
    {"black", TextFormatting::Black},
    {"dark_blue", TextFormatting::DarkBlue},
    {"dark_green", TextFormatting::DarkGreen},
    {"dark_aqua", TextFormatting::DarkAqua},
    {"dark_red", TextFormatting::DarkRed},
    {"dark_purple", TextFormatting::DarkPurple},
    {"gold", TextFormatting::Gold},
    {"gray", TextFormatting::Gray},
    {"dark_gray", TextFormatting::DarkGray},
    {"blue", TextFormatting::Blue},
    {"green", TextFormatting::Green},
    {"aqua", TextFormatting::Aqua},
    {"red", TextFormatting::Red},
    {"light_purple", TextFormatting::LightPurple},
    {"yellow", TextFormatting::Yellow},
    {"white", TextFormatting::White},
    {"obfuscated", TextFormatting::Obfuscated},
    {"bold", TextFormatting::Bold},
    {"strikethrough", TextFormatting::Strikethrough},
    {"underline", TextFormatting::Underline},
    {"italic", TextFormatting::Italic},
    {"reset", TextFormatting::Reset},
};

const TextFormatting* findFormatting(const std::string& name)
{
    for (const FormattingEntry& entry : FORMATTINGS) {
        if (name == entry.name) {
            return &entry.formatting;
        }
    }
    throw std::runtime_error("style_apply: unknown ChatFormatting: " + name);
}

std::string renderTri(const std::optional<bool>& value)
{
    if (!value.has_value()) {
        return "-";
    }
    return *value ? "1" : "0";
}

std::optional<bool> parseTri(const std::string& text)
{
    if (text == "-") {
        return std::nullopt;
    }
    return text == "1";
}

std::string render(const Style& style)
{
    std::string color = "-";
    if (style.getTextColor().has_value()) {
        char buffer[8];
        std::snprintf(buffer, sizeof(buffer), "%06x", static_cast<unsigned>(style.getTextColor()->value()));
        color = buffer;
    }
    const std::string insertion = style.getInsertion().has_value() ? *style.getInsertion() : "-";
    return color + "|" + renderTri(style.bold()) + "|" + renderTri(style.italic()) + "|" +
        renderTri(style.underlined()) + "|" + renderTri(style.strikethrough()) + "|" + renderTri(style.obfuscated()) +
        "|" + insertion;
}

Style parseStyle(const std::vector<std::string>& parts, std::size_t offset)
{
    Style style;
    if (parts[offset] != "-") {
        style.setTextColor(TextColor::fromRgb(static_cast<i32>(std::stoll(parts[offset], nullptr, 16))));
    }
    style.setBold(parseTri(parts[offset + 1]));
    style.setItalic(parseTri(parts[offset + 2]));
    style.setUnderlined(parseTri(parts[offset + 3]));
    style.setStrikethrough(parseTri(parts[offset + 4]));
    style.setObfuscated(parseTri(parts[offset + 5]));
    if (parts[offset + 6] != "-") {
        style.setInsertion(parts[offset + 6]);
    }
    return style;
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
        "applyTo|-|-|-|-|-|-|-|-|0|1|-|-|-|-",
        // applyTo：other 空返回 this
        "applyTo|ff0000|1|-|-|-|-|-|-|-|-|-|-|-|-",
        // applyTo：两者都满
        "applyTo|ff0000|1|0|-|-|-|-|00ff00|0|1|-|-|-|x",
        // applyTo：insertion 也参与
        "applyTo|-|-|-|-|-|-|ins|-|-|-|-|-|-|-",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
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
        throw std::runtime_error("style_apply: unexpected field count in unit: " + unit);
    }

    const std::string mode = parts[0];
    if (mode == "applyFormat" || mode == "applyLegacyFormat") {
        const Style style = parseStyle(parts, 1);
        const TextFormatting formatting = *findFormatting(parts[8]);
        const Style result =
            mode == "applyFormat" ? style.applyFormat(formatting) : style.applyLegacyFormat(formatting);
        return json{{"unit", unit}, {"value", render(result)}}.dump();
    }
    if (mode == "applyTo" && parts.size() == 15) {
        const Style self = parseStyle(parts, 1);
        const Style other = parseStyle(parts, 8);
        return json{{"unit", unit}, {"value", render(self.applyTo(other))}}.dump();
    }

    throw std::runtime_error("style_apply: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(style_apply, &writeList, &runLine)

} // namespace mc::diff
