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
#include "common/util/text/TextEvents.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
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

u32 getFormattingColor(TextFormatting formatting) noexcept
{
    const auto* info = findFormattingInfo(formatting);
    return info ? info->color : 0xFFFFFFFF;
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
        return getFormattingColor(*m_color);
    }
    return 0xFFFFFFFF; // 默认白色
}

Style Style::mergeWithParent(const Style& parent) const noexcept
{
    Style result;

    // 颜色：子样式优先
    result.m_color = m_color.has_value() ? m_color : parent.m_color;

    // 样式标志：子样式优先（OR 逻辑）
    result.m_bold = m_bold || parent.m_bold;
    result.m_italic = m_italic || parent.m_italic;
    result.m_underlined = m_underlined || parent.m_underlined;
    result.m_strikethrough = m_strikethrough || parent.m_strikethrough;
    result.m_obfuscated = m_obfuscated || parent.m_obfuscated;

    // 事件：子样式优先
    result.m_clickEvent = m_clickEvent.has_value() ? m_clickEvent : parent.m_clickEvent;
    result.m_hoverEvent = m_hoverEvent.has_value() ? m_hoverEvent : parent.m_hoverEvent;

    return result;
}

bool Style::isEmpty() const noexcept
{
    return !m_color.has_value() && !m_bold && !m_italic && !m_underlined && !m_strikethrough && !m_obfuscated &&
        !m_clickEvent.has_value() && !m_hoverEvent.has_value();
}

nlohmann::json Style::toJson() const
{
    nlohmann::json json = nlohmann::json::object();

    if (m_color.has_value()) {
        json["color"] = toName(*m_color);
    }
    if (m_bold) {
        json["bold"] = true;
    }
    if (m_italic) {
        json["italic"] = true;
    }
    if (m_underlined) {
        json["underlined"] = true;
    }
    if (m_strikethrough) {
        json["strikethrough"] = true;
    }
    if (m_obfuscated) {
        json["obfuscated"] = true;
    }
    if (m_clickEvent.has_value()) {
        json["clickEvent"] = m_clickEvent->toJson();
    }
    if (m_hoverEvent.has_value()) {
        json["hoverEvent"] = m_hoverEvent->toJson();
    }

    return json;
}

Style Style::fromJson(const nlohmann::json& json)
{
    Style style;

    if (json.contains("color") && json["color"].is_string()) {
        style.m_color = fromName(json["color"].get<std::string>());
    }
    if (json.contains("bold") && json["bold"].is_boolean()) {
        style.m_bold = json["bold"].get<bool>();
    }
    if (json.contains("italic") && json["italic"].is_boolean()) {
        style.m_italic = json["italic"].get<bool>();
    }
    if (json.contains("underlined") && json["underlined"].is_boolean()) {
        style.m_underlined = json["underlined"].get<bool>();
    }
    if (json.contains("strikethrough") && json["strikethrough"].is_boolean()) {
        style.m_strikethrough = json["strikethrough"].get<bool>();
    }
    if (json.contains("obfuscated") && json["obfuscated"].is_boolean()) {
        style.m_obfuscated = json["obfuscated"].get<bool>();
    }
    if (json.contains("clickEvent") && json["clickEvent"].is_object()) {
        style.m_clickEvent = ClickEvent::fromJson(json["clickEvent"]);
    }
    if (json.contains("hoverEvent") && json["hoverEvent"].is_object()) {
        style.m_hoverEvent = HoverEvent::fromJson(json["hoverEvent"]);
    }

    return style;
}

bool Style::operator==(const Style& other) const noexcept
{
    return m_color == other.m_color && m_bold == other.m_bold && m_italic == other.m_italic &&
        m_underlined == other.m_underlined && m_strikethrough == other.m_strikethrough &&
        m_obfuscated == other.m_obfuscated && m_clickEvent == other.m_clickEvent && m_hoverEvent == other.m_hoverEvent;
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

} // namespace mc::text
