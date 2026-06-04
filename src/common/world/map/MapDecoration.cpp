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

#include "MapDecoration.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "network/packet/PacketDeserializer.hpp"
#include "network/packet/PacketSerializer.hpp"
#include "util/assert/AssertMacros.hpp"
#include "util/text/StringTextComponent.hpp"
#include <algorithm>

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

MapDecoration::~MapDecoration() = default;

// 装饰类型属性查找表
struct DecorationTypeInfo {
    bool renderedOnFrame;
    i32 mapColor; // -1 表示无颜色
};

static constexpr DecorationTypeInfo DECORATION_TYPE_INFOS[] = {
    /* PLAYER */ {false, -1},
    /* FRAME */ {true, -1},
    /* RED_MARKER */ {false, -1},
    /* BLUE_MARKER */ {false, -1},
    /* TARGET_X */ {true, -1},
    /* TARGET_POINT */ {true, -1},
    /* PLAYER_OFF_MAP */ {false, -1},
    /* PLAYER_OFF_LIMITS */ {false, -1},
    /* MANSION */ {true, 5393476},  // 0x524C4D
    /* MONUMENT */ {true, 3830373}, // 0x3A7F5D
    /* BANNER_WHITE */ {true, -1},
    /* BANNER_ORANGE */ {true, -1},
    /* BANNER_MAGENTA */ {true, -1},
    /* BANNER_LIGHT_BLUE */ {true, -1},
    /* BANNER_YELLOW */ {true, -1},
    /* BANNER_LIME */ {true, -1},
    /* BANNER_PINK */ {true, -1},
    /* BANNER_GRAY */ {true, -1},
    /* BANNER_LIGHT_GRAY */ {true, -1},
    /* BANNER_CYAN */ {true, -1},
    /* BANNER_PURPLE */ {true, -1},
    /* BANNER_BLUE */ {true, -1},
    /* BANNER_BROWN */ {true, -1},
    /* BANNER_GREEN */ {true, -1},
    /* BANNER_RED */ {true, -1},
    /* BANNER_BLACK */ {true, -1},
    /* RED_X */ {true, -1},
};

static_assert(sizeof(DECORATION_TYPE_INFOS) / sizeof(DecorationTypeInfo) == static_cast<size_t>(DecorationType::COUNT),
    "DECORATION_TYPE_INFOS size must match DecorationType::COUNT");

bool isRenderedOnFrame(DecorationType type) noexcept
{
    const auto index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(DecorationType::COUNT));
    return DECORATION_TYPE_INFOS[index].renderedOnFrame;
}

bool hasMapColor(DecorationType type) noexcept
{
    const auto index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(DecorationType::COUNT));
    return DECORATION_TYPE_INFOS[index].mapColor >= 0;
}

i32 getMapColor(DecorationType type) noexcept
{
    const auto index = static_cast<size_t>(type);
    MC_ASSERT_RELEASE(index < static_cast<size_t>(DecorationType::COUNT));
    return DECORATION_TYPE_INFOS[index].mapColor;
}

DecorationType decorationTypeByIcon(u8 icon) noexcept
{
    if (icon >= static_cast<u8>(DecorationType::COUNT)) {
        return DecorationType::PLAYER;
    }
    return static_cast<DecorationType>(icon);
}

MapDecoration::MapDecoration(
    DecorationType type, i8 x, i8 y, u8 rotation, std::unique_ptr<text::ITextComponent> customName)
    : m_type(type)
    , m_x(x)
    , m_y(y)
    , m_rotation(rotation & 0x0F)
    , m_customName(std::move(customName))
{}

MapDecoration MapDecoration::deepCopy() const
{
    return MapDecoration(m_type, m_x, m_y, m_rotation, m_customName ? m_customName->deepCopy() : nullptr);
}

MapDecoration MapDecoration::fromNbt(const nbt::tags::compound_tag& tag)
{
    auto type = decorationTypeByIcon(static_cast<u8>(nbt_helper::tryGetByte(tag, "type").value_or(0)));
    i8 x = static_cast<i8>(nbt_helper::tryGetByte(tag, "x").value_or(0));
    i8 y = static_cast<i8>(nbt_helper::tryGetByte(tag, "y").value_or(0));
    u8 rotation = static_cast<u8>(nbt_helper::tryGetByte(tag, "rot").value_or(0)) & 0x0F;

    std::unique_ptr<text::ITextComponent> name;
    // TODO: 从NBT读取ITextComponent名称

    return MapDecoration(type, x, y, rotation, std::move(name));
}

void MapDecoration::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("type", static_cast<i8>(m_type));
    tag.put("x", m_x);
    tag.put("y", m_y);
    tag.put("rot", static_cast<i8>(m_rotation));

    // TODO: 写入ITextComponent名称
}

void MapDecoration::serialize(network::PacketSerializer& ser) const
{
    ser.writeU8(static_cast<u8>(m_type));
    ser.writeI8(m_x);
    ser.writeI8(m_y);
    ser.writeU8(m_rotation & 0x0F);

    if (m_customName) {
        ser.writeBool(true);
        // TODO: 序列化ITextComponent
        ser.writeString(m_customName->getUnformattedText());
    } else {
        ser.writeBool(false);
    }
}

MapDecoration MapDecoration::deserialize(network::PacketDeserializer& deser)
{
    auto typeResult = deser.readU8();
    auto xResult = deser.readI8();
    auto yResult = deser.readI8();
    auto rotationResult = deser.readU8();

    auto type = decorationTypeByIcon(typeResult.valueOr(0));
    i8 x = xResult.valueOr(0);
    i8 y = yResult.valueOr(0);
    u8 rotation = rotationResult.valueOr(0) & 0x0F;

    std::unique_ptr<text::ITextComponent> name;
    auto hasNameResult = deser.readBool();
    bool hasName = hasNameResult.valueOr(false);
    if (hasName) {
        auto nameStrResult = deser.readString();
        if (nameStrResult.success()) {
            // TODO: 解析ITextComponent
            name = std::make_unique<text::StringTextComponent>(nameStrResult.value());
        }
    }

    return MapDecoration(type, x, y, rotation, std::move(name));
}

} // namespace mc::world::map
