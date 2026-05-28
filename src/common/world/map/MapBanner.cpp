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
#include "entity/serialization/NbtHelper.hpp"
#include "util/text/ITextComponent.hpp"

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

    return MapBanner(BlockPos(x, y, z), static_cast<DyeColor>(colorInt), nullptr);
}

void MapBanner::toNbt(nbt::tags::compound_tag& tag) const
{
    tag.put("X", m_pos.x);
    tag.put("Y", m_pos.y);
    tag.put("Z", m_pos.z);
    tag.put("Color", static_cast<i32>(m_color));
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

} // namespace mc::world::map
