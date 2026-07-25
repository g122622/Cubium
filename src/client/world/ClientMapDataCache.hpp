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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/network/ir/packets/play/PlayPacketsExtended.hpp"
#include "common/util/text/ITextComponent.hpp"
#include "common/world/map/MapData.hpp"

#include <unordered_map>

namespace mc::client {

/**
 * @brief 客户端地图数据缓存
 *
 * 客户端 ClientWorld 不继承 IWorld、无 MapDataManager，无法从世界反查 MapData。
 * 地图数据只能来自服务端下发的 ir::play::MapItemData。本缓存按 mapId 存储 MapData，
 * 并把网络包还原成 MapData（对齐 Java ClientboundMapItemDataPacket.applyToMap：
 * decorations→applyClientDecorations，colorPatch→applyColorPatch）。
 *
 * 供 MapRenderer（纹理刷新）/ MapScreen（全屏查看）/ HudWidget（手持缩略图）共享访问。
 *
 * 注意：本类放在 mc::client 命名空间（与 ClientWorld 同级），而非 mc::client::world。
 * 若放入 mc::client::world 子命名空间，会令该子命名空间在被 ClientApplication.hpp 等
 * 广泛 include 的头链中实体化，进而破坏 mc::client 内其它裸 `world::X`（如
 * world::CHUNK_WIDTH / world::DimensionRenderSettings，本意指 mc::world）的限定名查找
 * ——编译器会在 mc::client::world 内查找并停止回退。详见 BiomeColorCache.hpp 等。
 */
class ClientMapDataCache {
public:
    ClientMapDataCache() = default;
    ~ClientMapDataCache() = default;

    // 禁止拷贝
    ClientMapDataCache(const ClientMapDataCache&) = delete;
    ClientMapDataCache& operator=(const ClientMapDataCache&) = delete;

    /**
     * @brief 应用 MapItemData 网络包到缓存
     *
     * 若 mapId 不存在则新建 MapData。更新 scale/locked/decorations/colorPatch。
     * @return 是否有变化（用于决定是否刷新 MapRenderer 纹理）
     */
    bool apply(const mc::network::ir::play::MapItemData& pkt);

    /**
     * @brief 获取指定 mapId 的 MapData（只读）
     *
     * @return MapData 指针，不存在返回 nullptr
     */
    [[nodiscard]] const mc::world::map::MapData* getMapData(i32 mapId) const;

    /**
     * @brief 获取指定 mapId 的 MapData（可变，供 MapRenderer 直接更新纹理）
     */
    [[nodiscard]] mc::world::map::MapData* getMapDataMutable(i32 mapId);

    /**
     * @brief 移除指定 mapId 的缓存
     */
    void removeMap(i32 mapId) { m_mapData.erase(mapId); }

    /**
     * @brief 清空所有缓存
     */
    void clear() { m_mapData.clear(); }

private:
    std::unordered_map<i32, mc::world::map::MapData> m_mapData;
};

} // namespace mc::client
