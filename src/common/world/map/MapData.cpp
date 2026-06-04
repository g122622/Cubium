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

#include "MapData.hpp"
#include "entity/serialization/NbtHelper.hpp"
#include "util/assert/AssertMacros.hpp"
#include "util/text/ITextComponent.hpp"
#include <algorithm>
#include <cmath>

namespace mc::world::map {

namespace nbt_helper = mc::entity::serialization::nbt_helper;

// ============================================================================
// MapInfo
// ============================================================================

MapData::MapInfo::MapInfo(i32 playerId)
    : m_playerId(playerId)
{}

void MapData::MapInfo::markDirty(i32 x, i32 y)
{
    if (m_isDirty) {
        m_minX = std::min(m_minX, x);
        m_minY = std::min(m_minY, y);
        m_maxX = std::max(m_maxX, x);
        m_maxY = std::max(m_maxY, y);
    } else {
        m_isDirty = true;
        m_minX = x;
        m_minY = y;
        m_maxX = x;
        m_maxY = y;
    }
}

void MapData::MapInfo::resetDirty()
{
    m_isDirty = false;
    m_minX = 0;
    m_minY = 0;
    m_maxX = MAP_SIZE - 1;
    m_maxY = MAP_SIZE - 1;
}

// ============================================================================
// MapData
// ============================================================================

MapData::MapData(i32 mapId)
    : m_mapId(mapId)
{}

void MapData::initialize(
    i32 xCenter, i32 zCenter, i32 scale, bool trackingPosition, bool unlimitedTracking, MapDimensionId dimension)
{
    m_xCenter = xCenter;
    m_zCenter = zCenter;
    m_scale = std::clamp(scale, 0, MAX_SCALE);
    m_trackingPosition = trackingPosition;
    m_unlimitedTracking = unlimitedTracking;
    m_dimension = dimension;
    m_dirty = true;
}

void MapData::calculateMapCenter(f64 x, f64 z, i32 scale, i32& outCenterX, i32& outCenterZ)
{
    i32 i = MAP_SIZE * (1 << scale);
    i32 halfMapSize = MAP_SIZE / 2;
    i32 j = static_cast<i32>(std::floor((x + static_cast<f64>(halfMapSize)) / static_cast<f64>(i)));
    i32 k = static_cast<i32>(std::floor((z + static_cast<f64>(halfMapSize)) / static_cast<f64>(i)));
    outCenterX = j * i + i / 2 - halfMapSize;
    outCenterZ = k * i + i / 2 - halfMapSize;
}

void MapData::updateDecoration(DecorationType type,
    const void* world,
    const std::string& decorationName,
    f64 worldX,
    f64 worldZ,
    f64 rotation,
    const text::ITextComponent* displayName)
{
    i32 i = 1 << m_scale; // 缩放因子

    // 将世界坐标转换为地图坐标
    f32 f = static_cast<f32>((worldX - static_cast<f64>(m_xCenter)) / static_cast<f64>(i));
    f32 f1 = static_cast<f32>((worldZ - static_cast<f64>(m_zCenter)) / static_cast<f64>(i));

    i8 b0 = static_cast<i8>(static_cast<f64>(f * 2.0F) + 0.5);
    i8 b1 = static_cast<i8>(static_cast<f64>(f1 * 2.0F) + 0.5);

    u8 b2 = 0;

    if (f >= -63.0F && f1 >= -63.0F && f <= 63.0F && f1 <= 63.0F) {
        // 在地图范围内
        f64 adjustedRotation = rotation + (rotation < 0.0 ? -8.0 : 8.0);
        b2 = static_cast<u8>(static_cast<i32>(adjustedRotation * 16.0 / 360.0));

        // TODO: 下界旋转随机化 - 需要世界时间信息
    } else {
        // 超出地图范围
        if (type != DecorationType::PLAYER) {
            m_decorations.erase(decorationName);
            return;
        }

        // 玩家类型特殊处理
        if (std::abs(f) < 320.0F && std::abs(f1) < 320.0F) {
            // 在边缘外但可追踪
            auto offMapType = DecorationType::PLAYER_OFF_MAP;
            m_decorations.insert_or_assign(
                decorationName, MapDecoration(offMapType, b0, b1, b2, displayName ? displayName->deepCopy() : nullptr));
            return;
        }

        if (m_unlimitedTracking) {
            auto offLimitsType = DecorationType::PLAYER_OFF_LIMITS;
            m_decorations.insert_or_assign(decorationName,
                MapDecoration(offLimitsType, b0, b1, b2, displayName ? displayName->deepCopy() : nullptr));
            return;
        }

        m_decorations.erase(decorationName);
        return;
    }

    m_decorations.insert_or_assign(
        decorationName, MapDecoration(type, b0, b1, b2, displayName ? displayName->deepCopy() : nullptr));
    m_dirty = true;
}

bool MapData::tryAddBanner(const void* world, const BlockPos& pos)
{
    // TODO: 从世界读取旗帜信息（需要BannerBlockEntity）
    // 暂时添加占位实现
    return false;
}

void MapData::removeStaleBanners(const void* world, i32 x, i32 z)
{
    // TODO: 检查旗帜是否仍然存在，移除不存在的
}

void MapData::addFrame(const MapFrame& frame)
{
    m_frames.insert_or_assign(frame.getId(), frame);
    // 在展示框位置添加装饰
    updateDecoration(DecorationType::FRAME,
        nullptr,
        frame.getId(),
        static_cast<f64>(frame.pos().x),
        static_cast<f64>(frame.pos().z),
        static_cast<f64>(frame.rotation()) * 90.0,
        nullptr);
}

void MapData::removeFrame(const std::string& frameId)
{
    m_frames.erase(frameId);
    m_decorations.erase(frameId);
}

void MapData::lockFrom(const MapData& source)
{
    m_locked = true;
    m_xCenter = source.m_xCenter;
    m_zCenter = source.m_zCenter;
    m_scale = source.m_scale;
    m_trackingPosition = source.m_trackingPosition;
    m_unlimitedTracking = source.m_unlimitedTracking;
    m_dimension = source.m_dimension;
    m_colors = source.m_colors;

    // 复制旗帜和装饰
    for (const auto& [key, banner] : source.m_banners) {
        m_banners.insert_or_assign(key, banner);
    }
    for (const auto& [key, deco] : source.m_decorations) {
        m_decorations.insert_or_assign(key, deco.deepCopy());
    }

    m_dirty = true;
}

void MapData::copyFrom(const MapData& source)
{
    m_xCenter = source.m_xCenter;
    m_zCenter = source.m_zCenter;
    m_scale = source.m_scale;
    m_trackingPosition = source.m_trackingPosition;
    m_unlimitedTracking = source.m_unlimitedTracking;
    m_dimension = source.m_dimension;
    m_colors = source.m_colors;

    for (const auto& [key, banner] : source.m_banners) {
        m_banners.insert_or_assign(key, banner);
    }
    for (const auto& [key, deco] : source.m_decorations) {
        m_decorations.insert_or_assign(key, deco.deepCopy());
    }

    m_dirty = true;
}

void MapData::markDirty(i32 x, i32 y)
{
    m_dirty = true;
    // 通知所有玩家该像素已变化
    for (auto& info : m_playerInfos) {
        info.markDirty(x, y);
    }
}

void MapData::setColor(i32 x, i32 y, u8 color)
{
    MC_ASSERT(x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE);
    if (m_colors[x + y * MAP_SIZE] != color) {
        m_colors[x + y * MAP_SIZE] = color;
        markDirty(x, y);
    }
}

u8 MapData::getColor(i32 x, i32 y) const
{
    MC_ASSERT(x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE);
    return m_colors[x + y * MAP_SIZE];
}

void MapData::toNbt(nbt::tags::compound_tag& tag) const
{
    // 维度
    // TODO: 维度ID序列化
    tag.put("xCenter", m_xCenter);
    tag.put("zCenter", m_zCenter);
    tag.put("scale", static_cast<i8>(m_scale));
    tag.put("trackingPosition", static_cast<i8>(m_trackingPosition ? 1 : 0));
    tag.put("unlimitedTracking", static_cast<i8>(m_unlimitedTracking ? 1 : 0));
    tag.put("locked", static_cast<i8>(m_locked ? 1 : 0));

    // 颜色数据
    nbt::tags::bytearray_tag colorsArray;
    colorsArray.value.reserve(COLOR_ARRAY_SIZE);
    for (u8 color : m_colors) {
        colorsArray.value.push_back(static_cast<i8>(color));
    }
    tag.value.emplace("colors", std::make_unique<nbt::tags::bytearray_tag>(std::move(colorsArray)));

    // 旗帜
    auto bannerList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& [key, banner] : m_banners) {
        nbt::tags::compound_tag bannerTag;
        banner.toNbt(bannerTag);
        bannerList->value.push_back(std::move(bannerTag));
    }
    tag.value.emplace("banners", std::move(bannerList));

    // 展示框
    auto frameList = std::make_unique<nbt::tags::compound_list_tag>();
    for (const auto& [key, frame] : m_frames) {
        nbt::tags::compound_tag frameTag;
        frame.toNbt(frameTag);
        frameList->value.push_back(std::move(frameTag));
    }
    tag.value.emplace("frames", std::move(frameList));
}

MapData MapData::fromNbt(const nbt::tags::compound_tag& tag, i32 mapId)
{
    MapData data(mapId);

    // TODO: 维度ID反序列化
    data.m_xCenter = nbt_helper::tryGetInt(tag, "xCenter").value_or(0);
    data.m_zCenter = nbt_helper::tryGetInt(tag, "zCenter").value_or(0);
    data.m_scale = std::clamp(static_cast<i32>(nbt_helper::tryGetByte(tag, "scale").value_or(0)), 0, MAX_SCALE);
    data.m_trackingPosition = nbt_helper::tryGetByte(tag, "trackingPosition").value_or(1) != 0;
    data.m_unlimitedTracking = nbt_helper::tryGetByte(tag, "unlimitedTracking").value_or(0) != 0;
    data.m_locked = nbt_helper::tryGetByte(tag, "locked").value_or(0) != 0;

    // 颜色数据
    {
        auto it = tag.value.find("colors");
        if (it != tag.value.end() && it->second->id() == nbt::TagId::ByteArray) {
            auto& colorsArray = dynamic_cast<const nbt::tags::bytearray_tag&>(*it->second).value;
            i32 copyLen = std::min(static_cast<i32>(colorsArray.size()), COLOR_ARRAY_SIZE);
            for (i32 i = 0; i < copyLen; ++i) {
                data.m_colors[static_cast<size_t>(i)] = static_cast<u8>(colorsArray[static_cast<size_t>(i)]);
            }
        }
    }

    // 旗帜
    {
        auto* bannerList = nbt_helper::tryGetList(tag, "banners");
        if (bannerList && bannerList->element_id() == nbt::TagId::Compound) {
            auto& banners = dynamic_cast<const nbt::tags::compound_list_tag&>(*bannerList).value;
            for (const auto& bannerTag : banners) {
                auto banner = MapBanner::fromNbt(bannerTag);
                data.m_banners[banner.getMapDecorationId()] = std::move(banner);
            }
        }
    }

    // 展示框
    {
        auto* frameList = nbt_helper::tryGetList(tag, "frames");
        if (frameList && frameList->element_id() == nbt::TagId::Compound) {
            auto& frames = dynamic_cast<const nbt::tags::compound_list_tag&>(*frameList).value;
            for (const auto& frameTag : frames) {
                auto frame = MapFrame::fromNbt(frameTag);
                data.m_frames[frame.getId()] = std::move(frame);
            }
        }
    }

    return data;
}

std::string MapData::getMapName(i32 mapId)
{
    return "map_" + std::to_string(mapId);
}

} // namespace mc::world::map
