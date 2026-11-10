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

#include "common/util/text/numbers/NumberFormat.hpp"

#include "common/core/Types.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StringTextComponent.hpp"
#include "common/util/text/TextStyle.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace mc::text::numbers {

namespace {

/// 去掉 "minecraft:" 前缀后的路径部分
std::string_view _stripNamespace(std::string_view name) noexcept
{
    constexpr std::string_view DEFAULT_NAMESPACE = "minecraft:";
    if (name.starts_with(DEFAULT_NAMESPACE)) {
        return name.substr(DEFAULT_NAMESPACE.size());
    }
    return name;
}

} // namespace

std::string_view numberFormatTypeName(NumberFormatType type) noexcept
{
    switch (type) {
        case NumberFormatType::Blank:
            return "minecraft:blank";
        case NumberFormatType::Styled:
            return "minecraft:styled";
        case NumberFormatType::Fixed:
            return "minecraft:fixed";
    }
    return "minecraft:blank";
}

std::optional<NumberFormatType> numberFormatTypeFromName(std::string_view name) noexcept
{
    const std::string_view path = _stripNamespace(name);
    if (path == "blank") {
        return NumberFormatType::Blank;
    }
    if (path == "styled") {
        return NumberFormatType::Styled;
    }
    if (path == "fixed") {
        return NumberFormatType::Fixed;
    }
    return std::nullopt;
}

NumberFormat::NumberFormat() = default;

NumberFormat::NumberFormat(const NumberFormat& other)
    : m_type(other.m_type)
    , m_style(other.m_style)
    , m_value(other.m_value != nullptr ? other.m_value->deepCopy() : nullptr)
{}

NumberFormat& NumberFormat::operator=(const NumberFormat& other)
{
    if (this == &other) {
        return *this;
    }
    m_type = other.m_type;
    m_style = other.m_style;
    m_value = other.m_value != nullptr ? other.m_value->deepCopy() : nullptr;
    return *this;
}

NumberFormat::NumberFormat(NumberFormat&& other) noexcept
    : m_type(other.m_type)
    , m_style(std::move(other.m_style))
    , m_value(std::move(other.m_value))
{
    // 被移走的对象退化为 blank，避免留下「类型是 fixed 但组件已空」的不一致状态
    other.m_type = NumberFormatType::Blank;
    other.m_value = nullptr;
}

NumberFormat& NumberFormat::operator=(NumberFormat&& other) noexcept
{
    if (this == &other) {
        return *this;
    }
    m_type = other.m_type;
    m_style = std::move(other.m_style);
    m_value = std::move(other.m_value);
    other.m_type = NumberFormatType::Blank;
    other.m_value = nullptr;
    return *this;
}

NumberFormat::~NumberFormat() = default;

NumberFormat NumberFormat::styled(Style style)
{
    NumberFormat format;
    format.m_type = NumberFormatType::Styled;
    format.m_style = std::move(style);
    return format;
}

NumberFormat NumberFormat::fixed(std::unique_ptr<ITextComponent> value)
{
    NumberFormat format;
    format.m_type = NumberFormatType::Fixed;
    format.m_value = std::move(value);
    return format;
}

const Style* NumberFormat::style() const noexcept
{
    return m_type == NumberFormatType::Styled ? &m_style : nullptr;
}

const ITextComponent* NumberFormat::value() const noexcept
{
    return m_type == NumberFormatType::Fixed ? m_value.get() : nullptr;
}

std::unique_ptr<ITextComponent> NumberFormat::format(i32 value) const
{
    switch (m_type) {
        case NumberFormatType::Blank:
            // 原版 BlankFormat#format 返回 Component.empty()
            return std::make_unique<StringTextComponent>(std::string{});
        case NumberFormatType::Styled: {
            // 原版 StyledFormat#format：literal(Integer.toString(value)).withStyle(style)
            auto component = std::make_unique<StringTextComponent>(std::to_string(value));
            component->setStyle(m_style);
            return component;
        }
        case NumberFormatType::Fixed:
            // 原版 FixedFormat#format 返回 value.copy()
            return m_value != nullptr ? m_value->deepCopy() : std::make_unique<StringTextComponent>(std::string{});
    }
    return std::make_unique<StringTextComponent>(std::string{});
}

bool NumberFormat::operator==(const NumberFormat& other) const noexcept
{
    if (m_type != other.m_type) {
        return false;
    }
    switch (m_type) {
        case NumberFormatType::Blank:
            return true;
        case NumberFormatType::Styled:
            return m_style == other.m_style;
        case NumberFormatType::Fixed:
            if (m_value == nullptr || other.m_value == nullptr) {
                return m_value == other.m_value;
            }
            return *m_value == *other.m_value;
    }
    return false;
}

} // namespace mc::text::numbers