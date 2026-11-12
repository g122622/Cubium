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

#include "TextStyle.hpp"
#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/json/JsonRead.hpp"
#include "common/util/text/TextEvents.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc::text {

namespace {

/// 格式化类型信息
struct FormattingInfo {
    TextFormatting formatting;
    char code;        // § 代码字符
    const char* name; // 名称
    u32 color;        // ARGB 颜色值（仅颜色类型有效）
};

/// 格式化类型查找表
constexpr FormattingInfo FORMATTING_TABLE[] = {
    // 颜色
    {TextFormatting::Black, '0', "black", 0xFF000000},
    {TextFormatting::DarkBlue, '1', "dark_blue", 0xFF0000AA},
    {TextFormatting::DarkGreen, '2', "dark_green", 0xFF00AA00},
    {TextFormatting::DarkAqua, '3', "dark_aqua", 0xFF00AAAA},
    {TextFormatting::DarkRed, '4', "dark_red", 0xFFAA0000},
    {TextFormatting::DarkPurple, '5', "dark_purple", 0xFFAA00AA},
    {TextFormatting::Gold, '6', "gold", 0xFFFFAA00},
    {TextFormatting::Gray, '7', "gray", 0xFFAAAAAA},
    {TextFormatting::DarkGray, '8', "dark_gray", 0xFF555555},
    {TextFormatting::Blue, '9', "blue", 0xFF5555FF},
    {TextFormatting::Green, 'a', "green", 0xFF55FF55},
    {TextFormatting::Aqua, 'b', "aqua", 0xFF55FFFF},
    {TextFormatting::Red, 'c', "red", 0xFFFF5555},
    {TextFormatting::LightPurple, 'd', "light_purple", 0xFFFF55FF},
    {TextFormatting::Yellow, 'e', "yellow", 0xFFFFFF55},
    {TextFormatting::White, 'f', "white", 0xFFFFFFFF},
    // 样式
    {TextFormatting::Obfuscated, 'k', "obfuscated", 0xFFFFFFFF},
    {TextFormatting::Bold, 'l', "bold", 0xFFFFFFFF},
    {TextFormatting::Strikethrough, 'm', "strikethrough", 0xFFFFFFFF},
    {TextFormatting::Underline, 'n', "underline", 0xFFFFFFFF},
    {TextFormatting::Italic, 'o', "italic", 0xFFFFFFFF},
    {TextFormatting::Reset, 'r', "reset", 0xFFFFFFFF},
};

constexpr size_t FORMATTING_TABLE_SIZE = sizeof(FORMATTING_TABLE) / sizeof(FORMATTING_TABLE[0]);

/// 查找格式化类型信息
const FormattingInfo* findFormattingInfo(TextFormatting formatting) noexcept
{
    for (size_t i = 0; i < FORMATTING_TABLE_SIZE; ++i) {
        if (FORMATTING_TABLE[i].formatting == formatting) {
            return &FORMATTING_TABLE[i];
        }
    }
    return nullptr;
}

} // namespace

// ========== TextFormatting 工具函数 ==========

std::optional<u32> findFormattingARGB(TextFormatting formatting) noexcept
{
    // 对应原版 ChatFormatting#getColor()：样式码没有颜色（返回 null）。
    if (!isColor(formatting)) {
        return std::nullopt;
    }
    const auto* info = findFormattingInfo(formatting);
    return info ? std::optional<u32>{info->color} : std::nullopt;
}

u32 getFormattingColor(TextFormatting formatting) noexcept
{
    // 渲染便捷入口：无颜色时退化为白色，与既有调用方（如 FontRenderer 的
    // 默认文字色）保持一致。需要区分「无颜色」与「白色」请用 findFormattingARGB。
    return findFormattingARGB(formatting).value_or(0xFFFFFFFF);
}

bool isColor(TextFormatting formatting) noexcept
{
    return static_cast<u8>(formatting) <= static_cast<u8>(TextFormatting::White);
}

bool isStyle(TextFormatting formatting) noexcept
{
    const u8 value = static_cast<u8>(formatting);
    return value >= static_cast<u8>(TextFormatting::Obfuscated) && value <= static_cast<u8>(TextFormatting::Italic);
}

TextFormatting fromCode(char code) noexcept
{
    // 转换为小写
    char lower = static_cast<char>(std::tolower(static_cast<unsigned char>(code)));

    for (size_t i = 0; i < FORMATTING_TABLE_SIZE; ++i) {
        if (FORMATTING_TABLE[i].code == lower) {
            return FORMATTING_TABLE[i].formatting;
        }
    }
    return TextFormatting::None;
}

char toCode(TextFormatting formatting) noexcept
{
    const auto* info = findFormattingInfo(formatting);
    return info ? info->code : '\0';
}

TextFormatting fromName(const std::string& name) noexcept
{
    // 转换为小写
    std::string lowerName;
    lowerName.reserve(name.size());
    std::transform(name.begin(), name.end(), std::back_inserter(lowerName), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });

    for (size_t i = 0; i < FORMATTING_TABLE_SIZE; ++i) {
        if (FORMATTING_TABLE[i].name == lowerName) {
            return FORMATTING_TABLE[i].formatting;
        }
    }
    return TextFormatting::None;
}

std::string toName(TextFormatting formatting)
{
    const auto* info = findFormattingInfo(formatting);
    return info ? info->name : "";
}

// ========== Style 类实现 ==========

u32 Style::getColorARGB() const noexcept
{
    if (m_color.has_value()) {
        return 0xFF000000U | static_cast<u32>(m_color->value());
    }
    return 0xFFFFFFFF; // 默认白色
}

std::optional<TextFormatting> Style::getColor() const noexcept
{
    if (!m_color.has_value()) {
        return std::nullopt;
    }
    // 具名颜色才能映射回 TextFormatting；自定义 RGB 没有对应的枚举值
    if (!m_color->name().has_value()) {
        return std::nullopt;
    }
    const TextFormatting formatting = fromName(*m_color->name());
    return formatting == TextFormatting::None ? std::nullopt : std::optional<TextFormatting>{formatting};
}

Style Style::applyFormat(TextFormatting formatting) const
{
    Style result = *this;
    switch (formatting) {
        case TextFormatting::Obfuscated:
            result.m_obfuscated = true;
            break;
        case TextFormatting::Bold:
            result.m_bold = true;
            break;
        case TextFormatting::Strikethrough:
            result.m_strikethrough = true;
            break;
        case TextFormatting::Underline:
            result.m_underlined = true;
            break;
        case TextFormatting::Italic:
            result.m_italic = true;
            break;
        case TextFormatting::Reset:
            return Style();
        default:
            result.m_color = TextColor::fromLegacyFormat(formatting);
            break;
    }
    return result;
}

Style Style::applyLegacyFormat(TextFormatting formatting) const
{
    Style result = *this;
    switch (formatting) {
        case TextFormatting::Obfuscated:
            result.m_obfuscated = true;
            break;
        case TextFormatting::Bold:
            result.m_bold = true;
            break;
        case TextFormatting::Strikethrough:
            result.m_strikethrough = true;
            break;
        case TextFormatting::Underline:
            result.m_underlined = true;
            break;
        case TextFormatting::Italic:
            result.m_italic = true;
            break;
        case TextFormatting::Reset:
            return Style();
        default:
            // § 语义：换颜色等同开新段落，旧的样式标志一并作废
            result.m_obfuscated = false;
            result.m_bold = false;
            result.m_strikethrough = false;
            result.m_underlined = false;
            result.m_italic = false;
            result.m_color = TextColor::fromLegacyFormat(formatting);
            break;
    }
    return result;
}

Style Style::applyTo(const Style& other) const
{
    if (isEmpty()) {
        return other;
    }
    if (other.isEmpty()) {
        return *this;
    }

    Style result;
    // 本样式优先，other 兜底
    result.m_color = m_color.has_value() ? m_color : other.m_color;
    const auto pick = [](const std::optional<bool>& primary, const std::optional<bool>& fallback) {
        return primary.has_value() ? primary : fallback;
    };
    result.m_bold = pick(m_bold, other.m_bold);
    result.m_italic = pick(m_italic, other.m_italic);
    result.m_underlined = pick(m_underlined, other.m_underlined);
    result.m_strikethrough = pick(m_strikethrough, other.m_strikethrough);
    result.m_obfuscated = pick(m_obfuscated, other.m_obfuscated);
    result.m_clickEvent = m_clickEvent.has_value() ? m_clickEvent : other.m_clickEvent;
    result.m_hoverEvent = m_hoverEvent.has_value() ? m_hoverEvent : other.m_hoverEvent;
    result.m_insertion = m_insertion.has_value() ? m_insertion : other.m_insertion;
    result.m_shadowColor = m_shadowColor.has_value() ? m_shadowColor : other.m_shadowColor;
    result.m_font = m_font.has_value() ? m_font : other.m_font;
    return result;
}

Style Style::mergeWithParent(const Style& parent) const noexcept
{
    Style result;

    // 颜色：子样式优先
    result.m_color = m_color.has_value() ? m_color : parent.m_color;

    // 样式标志：子样式**显式**给出就优先，否则继承父样式（三态语义）。
    // 这里不能写 `子 || 父`：子样式显式写 false 时要盖掉父样式的 true。
    const auto pick = [](const std::optional<bool>& child, const std::optional<bool>& parent) {
        return child.has_value() ? child : parent;
    };
    result.m_bold = pick(m_bold, parent.m_bold);
    result.m_italic = pick(m_italic, parent.m_italic);
    result.m_underlined = pick(m_underlined, parent.m_underlined);
    result.m_strikethrough = pick(m_strikethrough, parent.m_strikethrough);
    result.m_obfuscated = pick(m_obfuscated, parent.m_obfuscated);

    // 事件：子样式优先
    result.m_clickEvent = m_clickEvent.has_value() ? m_clickEvent : parent.m_clickEvent;
    result.m_hoverEvent = m_hoverEvent.has_value() ? m_hoverEvent : parent.m_hoverEvent;
    result.m_insertion = m_insertion.has_value() ? m_insertion : parent.m_insertion;
    result.m_shadowColor = m_shadowColor.has_value() ? m_shadowColor : parent.m_shadowColor;
    result.m_font = m_font.has_value() ? m_font : parent.m_font;

    return result;
}

bool Style::isEmpty() const noexcept
{
    // 三态：任一标志被显式设置（true 或 false）就不算空样式。
    return !m_color.has_value() && !m_bold.has_value() && !m_italic.has_value() && !m_underlined.has_value() &&
        !m_strikethrough.has_value() && !m_obfuscated.has_value() && !m_clickEvent.has_value() &&
        !m_hoverEvent.has_value() && !m_insertion.has_value() && !m_shadowColor.has_value() && !m_font.has_value();
}

nlohmann::json Style::toJson() const
{
    nlohmann::json json = nlohmann::json::object();

    if (m_color.has_value()) {
        // 原版 TextColor.serialize()：**具名颜色写名字**（"red"），只有自定义 RGB 才写
        // "#RRGGBB"。具名颜色经 `TextColor.fromLegacyFormat(ChatFormatting)` 构造，
        // 其 name 字段非空，故 serialize() 返回名字。
        //
        // 一律写十六进制是错的：原版 JSON（以及由原版解析的文本组件）里 `"color":"red"`
        // 与 `"color":"#FF5555"` 语义等价但**文本不等**，存档/网络逐字节比对会不一致。
        json["color"] = m_color->serialize();
    }
    // 三态：显式 false 也要写出来（`"italic": false`）。原版存档里它与「没写」是两种值。
    if (m_bold.has_value()) {
        json["bold"] = *m_bold;
    }
    if (m_italic.has_value()) {
        json["italic"] = *m_italic;
    }
    if (m_underlined.has_value()) {
        json["underlined"] = *m_underlined;
    }
    if (m_strikethrough.has_value()) {
        json["strikethrough"] = *m_strikethrough;
    }
    if (m_obfuscated.has_value()) {
        json["obfuscated"] = *m_obfuscated;
    }
    if (m_clickEvent.has_value()) {
        json["clickEvent"] = m_clickEvent->toJson();
    }
    if (m_hoverEvent.has_value()) {
        json["hoverEvent"] = m_hoverEvent->toJson();
    }

    if (m_insertion.has_value()) {
        json["insertion"] = *m_insertion;
    }
    // 键名与顺序按原版 `Style.Serializer.MAP_CODEC`：`shadow_color` 是 ARGB **整数**，
    // `font` 是资源位置字符串。这两个字段此前完全缺失，导致读取带它们的文本再写回时
    // 静默丢字段（不是显示差异，是数据丢失）。
    if (m_shadowColor.has_value()) {
        json["shadow_color"] = *m_shadowColor;
    }
    if (m_font.has_value()) {
        json["font"] = *m_font;
    }
    return json;
}

Style Style::fromJson(const nlohmann::json& json)
{
    // 文本组件来自数据包/网络等不可信输入：一律走 util::json 的返回 Result 的读取函数，
    // 不使用会在类型不符时抛异常的 json.get<T>()。
    Style style;

    const auto color = util::json::getString(json, "color");
    if (color.success()) {
        // 原版写 "#RRGGBB"；这里也接受颜色名，以便读取旧数据与手写 JSON。
        style.m_color = TextColor::parseColor(color.value());
    }

    const auto bold = util::json::getBool(json, "bold");
    if (bold.success()) {
        style.m_bold = bold.value();
    }
    const auto italic = util::json::getBool(json, "italic");
    if (italic.success()) {
        style.m_italic = italic.value();
    }
    const auto underlined = util::json::getBool(json, "underlined");
    if (underlined.success()) {
        style.m_underlined = underlined.value();
    }
    const auto strikethrough = util::json::getBool(json, "strikethrough");
    if (strikethrough.success()) {
        style.m_strikethrough = strikethrough.value();
    }
    const auto obfuscated = util::json::getBool(json, "obfuscated");
    if (obfuscated.success()) {
        style.m_obfuscated = obfuscated.value();
    }

    if (const auto* clickEvent = util::json::findField(json, "clickEvent");
        clickEvent != nullptr && clickEvent->is_object()) {
        style.m_clickEvent = ClickEvent::fromJson(*clickEvent);
    }
    if (const auto* hoverEvent = util::json::findField(json, "hoverEvent");
        hoverEvent != nullptr && hoverEvent->is_object()) {
        style.m_hoverEvent = HoverEvent::fromJson(*hoverEvent);
    }

    const auto insertion = util::json::getString(json, "insertion");
    if (insertion.success()) {
        style.m_insertion = insertion.value();
    }

    // ARGB 颜色按整数读取；原版 `Codec.INT` 不接受浮点，这里同样只认整型。
    const auto shadowColor = util::json::getInt32(json, "shadow_color");
    if (shadowColor.success()) {
        style.m_shadowColor = shadowColor.value();
    }

    const auto font = util::json::getString(json, "font");
    if (font.success()) {
        style.m_font = font.value();
    }

    return style;
}

bool Style::operator==(const Style& other) const noexcept
{
    return m_color == other.m_color && m_bold == other.m_bold && m_italic == other.m_italic &&
        m_underlined == other.m_underlined && m_strikethrough == other.m_strikethrough &&
        m_obfuscated == other.m_obfuscated && m_clickEvent == other.m_clickEvent &&
        m_hoverEvent == other.m_hoverEvent && m_insertion == other.m_insertion &&
        m_shadowColor == other.m_shadowColor && m_font == other.m_font;
}

// ========== 工具函数 ==========

std::string getStyleCodes(const Style& style)
{
    std::string codes;

    // 颜色代码
    if (style.getColor().has_value()) {
        char code = toCode(*style.getColor());
        if (code != '\0') {
            codes += "§";
            codes += code;
        }
    }

    // 样式代码（顺序重要：粗体、斜体、下划线、删除线、混淆）
    if (style.isBold()) {
        codes += "§l";
    }
    if (style.isItalic()) {
        codes += "§o";
    }
    if (style.isUnderlined()) {
        codes += "§n";
    }
    if (style.isStrikethrough()) {
        codes += "§m";
    }
    if (style.isObfuscated()) {
        codes += "§k";
    }

    return codes;
}

// ============================================================================
// TextColor
// ============================================================================

TextColor::TextColor(i32 value, std::optional<std::string> name)
    : m_value(value)
    , m_name(std::move(name))
{}

TextColor TextColor::fromRgb(i32 rgb)
{
    return TextColor(rgb & 0xFFFFFF, std::nullopt);
}

std::optional<TextColor> TextColor::fromLegacyFormat(TextFormatting formatting)
{
    // 样式码与 Reset/None 没有颜色，原版同样返回 null
    const std::optional<u32> argb = findFormattingARGB(formatting);
    if (!argb.has_value()) {
        return std::nullopt;
    }
    const std::string name = toName(formatting);
    return TextColor(static_cast<i32>(*argb & 0xFFFFFFU), name);
}

std::optional<TextColor> TextColor::parseColor(std::string_view text)
{
    if (text.empty()) {
        return std::nullopt;
    }

    // 十六进制形式：按 Java `Integer.parseInt(s, 16)` 的语义解析——
    // 允许前导 +/-、允许任意位数（`#f00` 得 0xF00、`#0` 得 0），
    // 只要求最终值落在 [0, 0xFFFFFF]。
    if (text[0] == '#') {
        const std::string digits(text.substr(1));
        if (digits.empty()) {
            return std::nullopt;
        }

        std::size_t index = 0;
        bool negative = false;
        if (digits[0] == '+' || digits[0] == '-') {
            negative = digits[0] == '-';
            index = 1;
            if (digits.size() == 1) {
                return std::nullopt;
            }
        }

        i64 value = 0;
        for (; index < digits.size(); ++index) {
            const char digit = digits[index];
            i32 nibble = 0;
            if (digit >= '0' && digit <= '9') {
                nibble = digit - '0';
            } else if (digit >= 'a' && digit <= 'f') {
                nibble = digit - 'a' + 10;
            } else if (digit >= 'A' && digit <= 'F') {
                nibble = digit - 'A' + 10;
            } else {
                return std::nullopt;
            }
            value = value * 16 + nibble;
            // 超出 int 范围后原版 parseInt 会抛 NumberFormatException；这里同样按非法处理
            if (value > 0x7FFFFFFFLL) {
                return std::nullopt;
            }
        }
        if (negative) {
            value = -value;
        }
        if (value < 0 || value > 0xFFFFFFLL) {
            return std::nullopt;
        }
        return TextColor(static_cast<i32>(value), std::nullopt);
    }

    // 具名形式：原版是 `NAMED_COLORS.get(text)`——**精确匹配、区分大小写**。
    // 不能走 `fromName`：那个函数会先转小写，会把 `RED` 当成 `red` 接受。
    for (std::size_t i = 0; i < FORMATTING_TABLE_SIZE; ++i) {
        if (FORMATTING_TABLE[i].name == text) {
            return fromLegacyFormat(FORMATTING_TABLE[i].formatting);
        }
    }
    return std::nullopt;
}

std::string TextColor::serialize() const
{
    if (m_name.has_value()) {
        return *m_name;
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "#%06X", static_cast<unsigned>(m_value));
    return buffer;
}

} // namespace mc::text
