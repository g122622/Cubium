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
#include "common/core/Types.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "util/assert/AssertMacros.hpp"
#include "util/text/ITextComponent.hpp"
#include "util/text/StringTextComponent.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
    /* DESERT_VILLAGE */ {true, 10066329}, // MapColor.COLOR_LIGHT_GRAY.col
    /* PLAINS_VILLAGE */ {true, 10066329},
    /* SAVANNA_VILLAGE */ {true, 10066329},
    /* SNOWY_VILLAGE */ {true, 10066329},
    /* TAIGA_VILLAGE */ {true, 10066329},
    /* JUNGLE_TEMPLE */ {true, 10066329},
    /* SWAMP_HUT */ {true, 10066329},
    /* TRIAL_CHAMBERS */ {true, 12741452}, // COPPER_COLOR
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

std::optional<DecorationType> decorationTypeFromString(const std::string& str) noexcept
{
    // 支持带命名空间和不带命名空间两种格式
    std::string name = str;
    constexpr std::string_view PREFIX = "minecraft:";
    if (name.size() > PREFIX.size() && name.substr(0, PREFIX.size()) == PREFIX) {
        name = name.substr(PREFIX.size());
    }

    // MC 1.16.5 风格的名称（蛇形命名）
    if (name == "player") {
        return DecorationType::PLAYER;
    }
    if (name == "frame") {
        return DecorationType::FRAME;
    }
    if (name == "red_marker") {
        return DecorationType::RED_MARKER;
    }
    if (name == "blue_marker") {
        return DecorationType::BLUE_MARKER;
    }
    if (name == "target_x") {
        return DecorationType::TARGET_X;
    }
    if (name == "target_point") {
        return DecorationType::TARGET_POINT;
    }
    if (name == "player_off_map") {
        return DecorationType::PLAYER_OFF_MAP;
    }
    if (name == "player_off_limits") {
        return DecorationType::PLAYER_OFF_LIMITS;
    }
    if (name == "mansion") {
        return DecorationType::MANSION;
    }
    if (name == "monument") {
        return DecorationType::MONUMENT;
    }
    if (name == "banner_white") {
        return DecorationType::BANNER_WHITE;
    }
    if (name == "banner_orange") {
        return DecorationType::BANNER_ORANGE;
    }
    if (name == "banner_magenta") {
        return DecorationType::BANNER_MAGENTA;
    }
    if (name == "banner_light_blue") {
        return DecorationType::BANNER_LIGHT_BLUE;
    }
    if (name == "banner_yellow") {
        return DecorationType::BANNER_YELLOW;
    }
    if (name == "banner_lime") {
        return DecorationType::BANNER_LIME;
    }
    if (name == "banner_pink") {
        return DecorationType::BANNER_PINK;
    }
    if (name == "banner_gray") {
        return DecorationType::BANNER_GRAY;
    }
    if (name == "banner_light_gray") {
        return DecorationType::BANNER_LIGHT_GRAY;
    }
    if (name == "banner_cyan") {
        return DecorationType::BANNER_CYAN;
    }
    if (name == "banner_purple") {
        return DecorationType::BANNER_PURPLE;
    }
    if (name == "banner_blue") {
        return DecorationType::BANNER_BLUE;
    }
    if (name == "banner_brown") {
        return DecorationType::BANNER_BROWN;
    }
    if (name == "banner_green") {
        return DecorationType::BANNER_GREEN;
    }
    if (name == "banner_red") {
        return DecorationType::BANNER_RED;
    }
    if (name == "banner_black") {
        return DecorationType::BANNER_BLACK;
    }
    if (name == "red_x") {
        return DecorationType::RED_X;
    }
    // 村庄/神庙/小屋/试炼密室：同时接受 registry key（village_desert 等）与 assetId（desert_village 等）
    if (name == "desert_village" || name == "village_desert") {
        return DecorationType::DESERT_VILLAGE;
    }
    if (name == "plains_village" || name == "village_plains") {
        return DecorationType::PLAINS_VILLAGE;
    }
    if (name == "savanna_village" || name == "village_savanna") {
        return DecorationType::SAVANNA_VILLAGE;
    }
    if (name == "snowy_village" || name == "village_snowy") {
        return DecorationType::SNOWY_VILLAGE;
    }
    if (name == "taiga_village" || name == "village_taiga") {
        return DecorationType::TAIGA_VILLAGE;
    }
    if (name == "jungle_temple") {
        return DecorationType::JUNGLE_TEMPLE;
    }
    if (name == "swamp_hut") {
        return DecorationType::SWAMP_HUT;
    }
    if (name == "trial_chambers") {
        return DecorationType::TRIAL_CHAMBERS;
    }

    return std::nullopt;
}

const char* decorationTypeToString(DecorationType type) noexcept
{
    switch (type) {
        case DecorationType::PLAYER:
            return "player";
        case DecorationType::FRAME:
            return "frame";
        case DecorationType::RED_MARKER:
            return "red_marker";
        case DecorationType::BLUE_MARKER:
            return "blue_marker";
        case DecorationType::TARGET_X:
            return "target_x";
        case DecorationType::TARGET_POINT:
            return "target_point";
        case DecorationType::PLAYER_OFF_MAP:
            return "player_off_map";
        case DecorationType::PLAYER_OFF_LIMITS:
            return "player_off_limits";
        case DecorationType::MANSION:
            return "mansion";
        case DecorationType::MONUMENT:
            return "monument";
        case DecorationType::BANNER_WHITE:
            return "banner_white";
        case DecorationType::BANNER_ORANGE:
            return "banner_orange";
        case DecorationType::BANNER_MAGENTA:
            return "banner_magenta";
        case DecorationType::BANNER_LIGHT_BLUE:
            return "banner_light_blue";
        case DecorationType::BANNER_YELLOW:
            return "banner_yellow";
        case DecorationType::BANNER_LIME:
            return "banner_lime";
        case DecorationType::BANNER_PINK:
            return "banner_pink";
        case DecorationType::BANNER_GRAY:
            return "banner_gray";
        case DecorationType::BANNER_LIGHT_GRAY:
            return "banner_light_gray";
        case DecorationType::BANNER_CYAN:
            return "banner_cyan";
        case DecorationType::BANNER_PURPLE:
            return "banner_purple";
        case DecorationType::BANNER_BLUE:
            return "banner_blue";
        case DecorationType::BANNER_BROWN:
            return "banner_brown";
        case DecorationType::BANNER_GREEN:
            return "banner_green";
        case DecorationType::BANNER_RED:
            return "banner_red";
        case DecorationType::BANNER_BLACK:
            return "banner_black";
        case DecorationType::RED_X:
            return "red_x";
        case DecorationType::DESERT_VILLAGE:
            return "desert_village";
        case DecorationType::PLAINS_VILLAGE:
            return "plains_village";
        case DecorationType::SAVANNA_VILLAGE:
            return "savanna_village";
        case DecorationType::SNOWY_VILLAGE:
            return "snowy_village";
        case DecorationType::TAIGA_VILLAGE:
            return "taiga_village";
        case DecorationType::JUNGLE_TEMPLE:
            return "jungle_temple";
        case DecorationType::SWAMP_HUT:
            return "swamp_hut";
        case DecorationType::TRIAL_CHAMBERS:
            return "trial_chambers";
        default:
            return "player";
    }
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
    auto nameStr = nbt_helper::tryGetString(tag, "name");
    if (nameStr.has_value()) {
        try {
            auto json = nlohmann::json::parse(nameStr.value());
            name = text::ITextComponent::fromJson(json);
        }
        catch (const nlohmann::json::exception&) {
            // JSON 解析失败，回退为纯文本组件
            name = std::make_unique<text::StringTextComponent>(nameStr.value());
        }
    }

    return MapDecoration(type, x, y, rotation, std::move(name));
}

void MapDecoration::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("type", static_cast<i8>(m_type));
    tag.put("x", m_x);
    tag.put("y", m_y);
    tag.put("rot", static_cast<i8>(m_rotation));

    if (m_customName) {
        tag.put("name", m_customName->toJson().dump());
    }
}

} // namespace mc::world::map
