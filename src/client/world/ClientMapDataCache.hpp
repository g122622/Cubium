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

#include "common/world/map/MapData.hpp"
#include <memory>
#include <unordered_map>

namespace mc::network {
class MapDataPacket;
}

namespace mc::client {

/**
 * @brief 客户端地图数据缓存
 *
 * 缓存从服务端收到的地图数据，用于客户端渲染地图物品。
 * 当收到MapDataPacket时，更新对应地图的缓存数据。
 */
class ClientMapDataCache {
public:
    ClientMapDataCache() = default;

    /**
     * @brief 处理服务端发来的地图数据包
     *
     * 解析包内容，更新或创建本地MapData缓存。
     */
    void handleMapDataPacket(const network::MapDataPacket& packet);

    /**
     * @brief 获取指定ID的地图数据
     *
     * @return 地图数据指针，如果不存在返回nullptr
     */
    [[nodiscard]] world::map::MapData* getMapData(i32 mapId);
    [[nodiscard]] const world::map::MapData* getMapData(i32 mapId) const;

    /**
     * @brief 创建或获取指定ID的地图数据
     *
     * 如果不存在，则创建一个空的MapData。
     */
    [[nodiscard]] world::map::MapData& getOrCreateMapData(i32 mapId);

    /**
     * @brief 清除所有缓存
     */
    void clear();

    /**
     * @brief 获取缓存中的地图数量
     */
    [[nodiscard]] size_t size() const { return m_cache.size(); }

private:
    std::unordered_map<i32, std::unique_ptr<world::map::MapData>> m_cache;
};

} // namespace mc::client
