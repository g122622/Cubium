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

#pragma once

#include "Packet.hpp"
#include "world/map/MapDecoration.hpp"
#include <vector>

namespace mc::world::map {
class MapData;
}

namespace mc::network {

/**
 * @brief 地图数据更新包 (服务端 -> 客户端)
 *
 * 将地图的更新数据发送给客户端，包括缩放级别、追踪状态、锁定状态、
 * 装饰物列表和颜色像素数据。
 *
 * 发送时机：
 * - 玩家持有地图物品时，每tick检测脏区域并发送增量更新
 * - 地图初始化时发送全量数据
 * - 装饰物变化时发送完整装饰列表
 *
 * 协议格式参考 MC 1.16.5 SMapPacket:
 *   VarInt mapId
 *   Byte   scale
 *   Boolean trackingPosition
 *   Boolean locked
 *   VarInt decorationCount (若trackingPosition)
 *   [decorationCount × (Byte type, Byte x, Byte y, Byte rotation, Optional<Text> name)]
 *   Byte   columns (0表示无颜色更新)
 *   若columns > 0:
 *     Byte   rows
 *     Byte   minX
 *     Byte   minZ
 *     Byte[] colorData (columns × rows 字节)
 */
class MapDataPacket : public Packet {
public:
    MapDataPacket();
    ~MapDataPacket() override = default;

    // MapDecoration is move-only, so the packet is move-only
    MapDataPacket(const MapDataPacket&) = delete;
    MapDataPacket& operator=(const MapDataPacket&) = delete;
    MapDataPacket(MapDataPacket&&) noexcept = default;
    MapDataPacket& operator=(MapDataPacket&&) noexcept = default;

    /**
     * @brief 从MapData构造增量更新包
     *
     * @param mapData 地图数据
     * @param minX 脏区域最小X (0-127)
     * @param minY 脏区域最小Y (0-127)
     * @param maxX 脏区域最大X (0-127)
     * @param maxY 脏区域最大Y (0-127)
     */
    MapDataPacket(const world::map::MapData& mapData, i32 minX, i32 minY, i32 maxX, i32 maxY);

    /**
     * @brief 构造只有装饰更新的包（无颜色数据）
     */
    static MapDataPacket decorationsOnly(i32 mapId,
        i32 scale,
        bool trackingPosition,
        bool locked,
        const std::vector<world::map::MapDecoration>& decorations);

    // ========== Packet 接口实现 ==========

    [[nodiscard]] Result<std::vector<u8>> serialize() const override;
    [[nodiscard]] Result<void> deserialize(const u8* data, size_t size) override;
    size_t expectedSize() const override;

    // ========== Getter ==========

    [[nodiscard]] i32 mapId() const { return m_mapId; }
    [[nodiscard]] i32 scale() const { return m_scale; }
    [[nodiscard]] bool trackingPosition() const { return m_trackingPosition; }
    [[nodiscard]] bool locked() const { return m_locked; }
    [[nodiscard]] const std::vector<world::map::MapDecoration>& decorations() const { return m_decorations; }
    [[nodiscard]] i32 columns() const { return m_columns; }
    [[nodiscard]] i32 rows() const { return m_rows; }
    [[nodiscard]] i32 minX() const { return m_minX; }
    [[nodiscard]] i32 minZ() const { return m_minZ; }
    [[nodiscard]] const std::vector<u8>& colorData() const { return m_colorData; }

    /**
     * @brief 将包数据应用到客户端MapData
     */
    void applyTo(world::map::MapData& mapData) const;

private:
    i32 m_mapId = 0;
    i32 m_scale = 0;
    bool m_trackingPosition = true;
    bool m_locked = false;
    std::vector<world::map::MapDecoration> m_decorations;
    i32 m_columns = 0; // 颜色更新的列数，0表示无颜色更新
    i32 m_rows = 0;
    i32 m_minX = 0;
    i32 m_minZ = 0;
    std::vector<u8> m_colorData;
};

} // namespace mc::network
