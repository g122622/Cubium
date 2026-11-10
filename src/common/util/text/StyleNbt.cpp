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

#include "common/util/text/StyleNbt.hpp"

#include "common/core/Types.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace mc::text {

namespace {

/// 取某个键的字符串标签值；键不存在返回 nullptr
const nbt::tags::string_tag* _findString(const nbt::tags::compound_tag& tag, const char* key)
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return nullptr;
    }
    return dynamic_cast<const nbt::tags::string_tag*>(it->second.get());
}

/// 取某个键的数值（Byte/Short/Int/Long）；键不存在或不是整数标签返回 nullopt
std::optional<i64> _findInteger(const nbt::tags::compound_tag& tag, const char* key)
{
    const auto it = tag.value.find(key);
    if (it == tag.value.end()) {
        return std::nullopt;
    }

    // 原版 Codec.BOOL 在 NbtOps 下接受任意数值标签，按其是否为 0 解释。
    switch (it->second->id()) {
        case nbt::TagId::Byte:
            return static_cast<i64>(dynamic_cast<const nbt::tags::byte_tag&>(*it->second).value);
        case nbt::TagId::Short:
            return static_cast<i64>(dynamic_cast<const nbt::tags::short_tag&>(*it->second).value);
        case nbt::TagId::Int:
            return static_cast<i64>(dynamic_cast<const nbt::tags::int_tag&>(*it->second).value);
        case nbt::TagId::Long:
            return dynamic_cast<const nbt::tags::long_tag&>(*it->second).value;
        default:
            return std::nullopt;
    }
}

} // namespace

void styleToNbt(nbt::tags::compound_tag& out, const Style& style)
{
    // 颜色：具名色写名字，自定义 RGB 写 "#RRGGBB"（原版 TextColor.CODEC 的 serialize 方向）。
    // 用 getTextColor 而不是 getColor：后者只覆盖具名色，会把自定义 RGB 静默丢掉。
    if (const auto& color = style.getTextColor(); color.has_value()) {
        out.put("color", color->serialize());
    }

    // 三态标志：只有显式设置过才写出。原版 `Style.Serializer` 用 Optional<Boolean>，
    // 「显式 false」与「未设置」在存档与网络里是两种不同的值。
    const auto putFlag = [&out](const char* key, const std::optional<bool>& value) {
        if (value.has_value()) {
            out.put(key, static_cast<std::int8_t>(*value ? 1 : 0));
        }
    };
    putFlag("bold", style.bold());
    putFlag("italic", style.italic());
    putFlag("underlined", style.underlined());
    putFlag("strikethrough", style.strikethrough());
    putFlag("obfuscated", style.obfuscated());

    if (const auto& insertion = style.getInsertion(); insertion.has_value()) {
        out.put("insertion", *insertion);
    }

    // 阴影色：原版 `ExtraCodecs.ARGB_COLOR_CODEC` 以 `Codec.INT` 为主形态，
    // 在 NbtOps 下就是 IntTag（ARGB 打包整数，可为负）。
    if (const auto& shadowColor = style.getShadowColor(); shadowColor.has_value()) {
        out.put("shadow_color", *shadowColor);
    }

    // 字体：`FontDescription.CODEC` 是 `Identifier.CODEC.flatComapMap`，形态是资源位置字符串。
    if (const auto& font = style.getFont(); font.has_value()) {
        out.put("font", *font);
    }
}

Result<Style> styleFromNbt(const nbt::tags::compound_tag& tag)
{
    Style style;

    if (const auto* colorTag = _findString(tag, "color"); colorTag != nullptr) {
        const auto color = TextColor::parseColor(colorTag->value);
        if (!color.has_value()) {
            return Error(ErrorCode::InvalidData, "Invalid style color: " + colorTag->value, "text::styleFromNbt");
        }
        style.setTextColor(color);
    }

    if (const auto value = _findInteger(tag, "bold"); value.has_value()) {
        style.setBold(*value != 0);
    }
    if (const auto value = _findInteger(tag, "italic"); value.has_value()) {
        style.setItalic(*value != 0);
    }
    if (const auto value = _findInteger(tag, "underlined"); value.has_value()) {
        style.setUnderlined(*value != 0);
    }
    if (const auto value = _findInteger(tag, "strikethrough"); value.has_value()) {
        style.setStrikethrough(*value != 0);
    }
    if (const auto value = _findInteger(tag, "obfuscated"); value.has_value()) {
        style.setObfuscated(*value != 0);
    }

    if (const auto* insertionTag = _findString(tag, "insertion"); insertionTag != nullptr) {
        style.setInsertion(insertionTag->value);
    }

    // 原版 `Codec.INT` 在 NbtOps 下接受任意数值标签，再按 `intValue()` 截断成 32 位。
    if (const auto value = _findInteger(tag, "shadow_color"); value.has_value()) {
        style.setShadowColor(static_cast<i32>(*value));
    }

    if (const auto* fontTag = _findString(tag, "font"); fontTag != nullptr) {
        style.setFont(fontTag->value);
    }

    return style;
}

} // namespace mc::text
