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

#include "MapDataPacket.hpp"
#include "PacketDeserializer.hpp"
#include "PacketSerializer.hpp"
#include "util/text/ITextComponent.hpp"
#include "world/map/MapData.hpp"
#include <algorithm>

namespace mc::network {

MapDataPacket::MapDataPacket()
    : Packet(PacketType::MapData)
{}

MapDataPacket::MapDataPacket(const world::map::MapData& mapData, i32 minX, i32 minY, i32 maxX, i32 maxY)
    : Packet(PacketType::MapData)
    , m_mapId(mapData.mapId())
    , m_scale(mapData.scale())
    , m_trackingPosition(mapData.trackingPosition())
    , m_locked(mapData.locked())
{
    // 收集装饰
    for (const auto& [key, deco] : mapData.decorations()) {
        m_decorations.push_back(deco.deepCopy());
    }

    // 计算颜色更新区域
    constexpr i32 MAP_SIZE = world::map::MapData::MAP_SIZE;

    // 裁剪脏区域到地图边界
    i32 clampMinX = std::clamp(minX, 0, MAP_SIZE - 1);
    i32 clampMinY = std::clamp(minY, 0, MAP_SIZE - 1);
    i32 clampMaxX = std::clamp(maxX, 0, MAP_SIZE - 1);
    i32 clampMaxY = std::clamp(maxY, 0, MAP_SIZE - 1);

    m_columns = clampMaxX - clampMinX + 1;
    m_rows = clampMaxY - clampMinY + 1;
    m_minX = clampMinX;
    m_minZ = clampMinY;

    // 提取颜色数据
    if (m_columns > 0 && m_rows > 0) {
        const auto& colors = mapData.colors();
        m_colorData.reserve(static_cast<size_t>(m_columns * m_rows));
        for (i32 row = 0; row < m_rows; ++row) {
            for (i32 col = 0; col < m_columns; ++col) {
                i32 x = clampMinX + col;
                i32 y = clampMinY + row;
                m_colorData.push_back(colors[static_cast<size_t>(x + y * MAP_SIZE)]);
            }
        }
    }
}

MapDataPacket MapDataPacket::decorationsOnly(
    i32 mapId, i32 scale, bool trackingPosition, bool locked, const std::vector<world::map::MapDecoration>& decorations)
{
    MapDataPacket packet;
    packet.m_mapId = mapId;
    packet.m_scale = scale;
    packet.m_trackingPosition = trackingPosition;
    packet.m_locked = locked;
    for (const auto& deco : decorations) {
        packet.m_decorations.push_back(deco.deepCopy());
    }
    // m_columns = 0 表示无颜色更新
    return packet;
}

Result<std::vector<u8>> MapDataPacket::serialize() const
{
    PacketSerializer ser;

    // 地图ID (VarInt)
    ser.writeVarInt(m_mapId);

    // 缩放级别 (Byte)
    ser.writeI8(static_cast<i8>(m_scale));

    // 追踪位置 (Boolean)
    ser.writeBool(m_trackingPosition);

    // 锁定状态 (Boolean)
    ser.writeBool(m_locked);

    // 装饰列表 (仅当trackingPosition时发送)
    if (m_trackingPosition) {
        ser.writeVarInt(static_cast<i32>(m_decorations.size()));
        for (const auto& deco : m_decorations) {
            deco.serialize(ser);
        }
    }

    // 颜色更新列数 (Byte, 0=无更新)
    ser.writeU8(static_cast<u8>(m_columns));

    if (m_columns > 0) {
        // 行数 (Byte)
        ser.writeU8(static_cast<u8>(m_rows));

        // 起始X (Byte)
        ser.writeU8(static_cast<u8>(m_minX));

        // 起始Z (Byte)
        ser.writeU8(static_cast<u8>(m_minZ));

        // 颜色数据
        ser.writeBytes(m_colorData.data(), m_colorData.size());
    }

    return ser.buffer();
}

Result<void> MapDataPacket::deserialize(const u8* data, size_t size)
{
    PacketDeserializer deser(data, size);

    // 地图ID
    auto mapIdResult = deser.readVarInt();
    if (mapIdResult.failed()) {
        return mapIdResult.error();
    }
    m_mapId = mapIdResult.value();

    // 缩放级别
    auto scaleResult = deser.readI8();
    if (scaleResult.failed()) {
        return scaleResult.error();
    }
    m_scale = static_cast<i32>(scaleResult.value());

    // 追踪位置
    auto trackingResult = deser.readBool();
    if (trackingResult.failed()) {
        return trackingResult.error();
    }
    m_trackingPosition = trackingResult.value();

    // 锁定状态
    auto lockedResult = deser.readBool();
    if (lockedResult.failed()) {
        return lockedResult.error();
    }
    m_locked = lockedResult.value();

    // 装饰列表
    if (m_trackingPosition) {
        auto decoCountResult = deser.readVarInt();
        if (decoCountResult.failed()) {
            return decoCountResult.error();
        }
        i32 decoCount = decoCountResult.value();

        m_decorations.clear();
        m_decorations.reserve(static_cast<size_t>(decoCount));
        for (i32 i = 0; i < decoCount; ++i) {
            auto deco = world::map::MapDecoration::deserialize(deser);
            m_decorations.push_back(std::move(deco));
        }
    }

    // 颜色更新列数
    auto columnsResult = deser.readU8();
    if (columnsResult.failed()) {
        return columnsResult.error();
    }
    m_columns = static_cast<i32>(columnsResult.value());

    if (m_columns > 0) {
        // 行数
        auto rowsResult = deser.readU8();
        if (rowsResult.failed()) {
            return rowsResult.error();
        }
        m_rows = static_cast<i32>(rowsResult.value());

        // 起始X
        auto minXResult = deser.readU8();
        if (minXResult.failed()) {
            return minXResult.error();
        }
        m_minX = static_cast<i32>(minXResult.value());

        // 起始Z
        auto minZResult = deser.readU8();
        if (minZResult.failed()) {
            return minZResult.error();
        }
        m_minZ = static_cast<i32>(minZResult.value());

        // 颜色数据
        size_t colorSize = static_cast<size_t>(m_columns * m_rows);
        auto colorResult = deser.readBytes(colorSize);
        if (colorResult.failed()) {
            return colorResult.error();
        }
        m_colorData = std::move(colorResult.value());
    }

    return Result<void>::ok();
}

size_t MapDataPacket::expectedSize() const
{
    size_t size = sizeof(PacketHeader);
    size += 4; // mapId VarInt (最多5字节，估算4)
    size += 1; // scale
    size += 1; // trackingPosition
    size += 1; // locked
    if (m_trackingPosition) {
        size += 4; // decorationCount VarInt
        for (const auto& deco : m_decorations) {
            size += 4; // type + x + y + rotation
            // customName: bool + optional string
            size += 1;
            if (deco.customName()) {
                size += 32; // 估算名称长度
            }
        }
    }
    size += 1; // columns
    if (m_columns > 0) {
        size += 3; // rows + minX + minZ
        size += m_colorData.size();
    }
    return size;
}

void MapDataPacket::applyTo(world::map::MapData& mapData) const
{
    constexpr i32 MAP_SIZE = world::map::MapData::MAP_SIZE;

    // 更新锁定状态
    if (m_locked && !mapData.locked()) {
        mapData.setLocked(true);
    }

    // 更新装饰 - 客户端收到的装饰列表用于更新现有装饰
    if (m_trackingPosition) {
        // 遍历收到的装饰并更新/添加到地图数据中
        for (const auto& deco : m_decorations) {
            std::string key = "deco-" + std::to_string(static_cast<i32>(deco.type())) + "-" + std::to_string(deco.x()) +
                "-" + std::to_string(deco.y());
            // 直接调用 updateDecoration 来添加/更新装饰
            mapData.updateDecoration(deco.type(),
                nullptr,
                key,
                static_cast<f64>(mapData.xCenter()) +
                    static_cast<f64>(deco.x()) * static_cast<f64>(1 << mapData.scale()) / 2.0,
                static_cast<f64>(mapData.zCenter()) +
                    static_cast<f64>(deco.y()) * static_cast<f64>(1 << mapData.scale()) / 2.0,
                static_cast<f64>(deco.rotation()) * 22.5, // 旋转: 0-15 → 0-337.5度
                deco.customName());
        }
    }

    // 更新颜色数据
    if (m_columns > 0 && !m_colorData.empty()) {
        for (i32 row = 0; row < m_rows; ++row) {
            for (i32 col = 0; col < m_columns; ++col) {
                i32 x = m_minX + col;
                i32 z = m_minZ + row;
                if (x >= 0 && x < MAP_SIZE && z >= 0 && z < MAP_SIZE) {
                    size_t srcIdx = static_cast<size_t>(row * m_columns + col);
                    if (srcIdx < m_colorData.size()) {
                        mapData.setColor(x, z, m_colorData[srcIdx]);
                    }
                }
            }
        }
    }
}

} // namespace mc::network
