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

#include "common/util/text/numbers/NumberFormatCodec.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/buffer/NbtIo.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/util/text/ComponentNbtSerialization.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/util/text/StyleNbt.hpp"
#include "common/util/text/numbers/NumberFormat.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mc::text::numbers {

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

} // namespace

nbt::tags::compound_tag numberFormatToNbt(const NumberFormat& format)
{
    nbt::tags::compound_tag tag;
    tag.put("type", std::string(numberFormatTypeName(format.type())));

    switch (format.type()) {
        case NumberFormatType::Blank:
            break;
        case NumberFormatType::Styled:
            // 样式字段与 type 平铺（原版 dispatchMap 的 MapCodec 合并语义）
            ::mc::text::styleToNbt(tag, *format.style());
            break;
        case NumberFormatType::Fixed:
            tag.value.emplace("value", ::mc::text::componentToNbtTag(format.value()));
            break;
    }

    return tag;
}

Result<NumberFormat> numberFormatFromNbt(const nbt::tags::compound_tag& tag)
{
    const auto* typeTag = _findString(tag, "type");
    if (typeTag == nullptr) {
        return Error(ErrorCode::InvalidData, "NumberFormat NBT has no string 'type'", "numbers::numberFormatFromNbt");
    }

    const auto type = numberFormatTypeFromName(typeTag->value);
    if (!type.has_value()) {
        return Error(
            ErrorCode::InvalidData, "Unknown NumberFormat type: " + typeTag->value, "numbers::numberFormatFromNbt");
    }

    switch (*type) {
        case NumberFormatType::Blank:
            return NumberFormat();
        case NumberFormatType::Styled: {
            auto style = ::mc::text::styleFromNbt(tag);
            if (style.failed()) {
                return style.error();
            }
            return NumberFormat::styled(std::move(style.value()));
        }
        case NumberFormatType::Fixed: {
            const auto it = tag.value.find("value");
            if (it == tag.value.end()) {
                return Error(
                    ErrorCode::InvalidData, "Fixed NumberFormat NBT has no 'value'", "numbers::numberFormatFromNbt");
            }
            auto component = ::mc::text::componentFromNbtTag(*it->second);
            if (component.failed()) {
                return component.error();
            }
            return NumberFormat::fixed(component.value());
        }
    }

    return Error(ErrorCode::InvalidData, "Unhandled NumberFormat type", "numbers::numberFormatFromNbt");
}

std::vector<u8> numberFormatToWireBytes(const NumberFormat& format)
{
    // VarInt(注册表 id)：三个 id 都 < 128，单字节即可
    std::vector<u8> bytes;
    bytes.push_back(static_cast<u8>(format.type()));

    switch (format.type()) {
        case NumberFormatType::Blank:
            break;
        case NumberFormatType::Styled: {
            nbt::tags::compound_tag styleTag;
            ::mc::text::styleToNbt(styleTag, *format.style());
            const auto styleBytes = network::buffer::nbt_io::serializeRootCompoundToBytes(styleTag);
            bytes.insert(bytes.end(), styleBytes.begin(), styleBytes.end());
            break;
        }
        case NumberFormatType::Fixed: {
            const auto componentBytes =
                network::buffer::nbt_io::serializeRootTagToBytes(*::mc::text::componentToNbtTag(format.value()));
            bytes.insert(bytes.end(), componentBytes.begin(), componentBytes.end());
            break;
        }
    }

    return bytes;
}

std::vector<u8> numberFormatToWireBytes(const std::optional<NumberFormat>& format)
{
    if (!format.has_value()) {
        return {};
    }
    return numberFormatToWireBytes(*format);
}

} // namespace mc::text::numbers
