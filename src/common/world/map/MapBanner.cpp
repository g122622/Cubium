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

#include "MapBanner.hpp"
#include "common/core/Types.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/map/MapDecoration.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "util/text/ITextComponent.hpp"
#include "util/text/StringTextComponent.hpp"
#include "world/IWorld.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/interactive/BannerEntity.hpp"
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

MapBanner::MapBanner(BlockPos pos, DyeColor color, std::unique_ptr<text::ITextComponent> name)
    : m_pos(pos)
    , m_color(color)
    , m_name(std::move(name))
{}

MapBanner::MapBanner(const MapBanner& other)
    : m_pos(other.m_pos)
    , m_color(other.m_color)
    , m_name(other.m_name ? other.m_name->deepCopy() : nullptr)
{}

MapBanner& MapBanner::operator=(const MapBanner& other)
{
    if (this != &other) {
        m_pos = other.m_pos;
        m_color = other.m_color;
        m_name = other.m_name ? other.m_name->deepCopy() : nullptr;
    }
    return *this;
}

MapBanner MapBanner::fromNbt(const nbt::tags::compound_tag& tag)
{
    auto x = nbt_helper::tryGetInt(tag, "X").value_or(0);
    auto y = nbt_helper::tryGetInt(tag, "Y").value_or(0);
    auto z = nbt_helper::tryGetInt(tag, "Z").value_or(0);
    auto colorInt = nbt_helper::tryGetInt(tag, "Color").value_or(0);

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

    return MapBanner(BlockPos(x, y, z), static_cast<DyeColor>(colorInt), std::move(name));
}

void MapBanner::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("X", m_pos.x);
    tag.put("Y", m_pos.y);
    tag.put("Z", m_pos.z);
    tag.put("Color", static_cast<i32>(m_color));

    if (m_name) {
        tag.put("name", m_name->toJson().dump());
    }
}

DecorationType MapBanner::getDecorationType() const
{
    // 旗帜装饰类型根据颜色映射: BANNER_WHITE=10, BANNER_ORANGE=11, ...
    i32 base = static_cast<i32>(DecorationType::BANNER_WHITE);
    i32 colorIndex = static_cast<i32>(m_color);
    return static_cast<DecorationType>(base + colorIndex);
}

std::string MapBanner::getMapDecorationId() const
{
    return "banner-" + std::to_string(m_pos.x) + "-" + std::to_string(m_pos.y) + "-" + std::to_string(m_pos.z);
}

std::optional<MapBanner> MapBanner::fromWorld(IWorld& world, const BlockPos& pos)
{
    auto* blockEntity = world.getBlockEntity(pos);
    auto* bannerEntity = dynamic_cast<blockentity::BannerEntity*>(blockEntity);
    if (bannerEntity == nullptr) {
        return std::nullopt;
    }

    DyeColor baseColor = bannerEntity->getBaseColor();
    std::unique_ptr<text::ITextComponent> customName;
    const text::ITextComponent* displayName = bannerEntity->getCustomDisplayName();
    if (displayName != nullptr) {
        customName = displayName->deepCopy();
    }

    return MapBanner(pos, baseColor, std::move(customName));
}

bool MapBanner::equals(const MapBanner& other) const
{
    return m_pos == other.m_pos && m_color == other.m_color;
}

} // namespace mc::world::map
