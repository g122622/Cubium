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

#include "MapData.hpp"
#include "MapIdTracker.hpp"
#include "common/core/Types.hpp"
#include "core/Result.hpp"
#include <cstddef>
#include <memory>
#include <unordered_map>

namespace mc {
class IWorld;
}

namespace mc::world::map {

/**
 * @brief 地图数据管理器
 *
 * 管理所有地图数据的生命周期，包括CRUD操作和ID分配。
 * 挂载在ServerWorld上，由ServerWorld调用tick更新持有地图的玩家可见区域。
 */
class MapDataManager {
public:
    MapDataManager() noexcept = default;

    /**
     * @brief 获取指定ID的地图数据
     */
    [[nodiscard]] MapData* getMapData(i32 mapId);
    [[nodiscard]] const MapData* getMapData(i32 mapId) const;

    /**
     * @brief 创建新地图数据
     *
     * @param mapId 地图ID
     * @return 新创建的MapData指针，如果ID已存在则返回nullptr
     */
    MapData* createMapData(i32 mapId);

    /**
     * @brief 分配新的地图ID
     */
    [[nodiscard]] i32 allocateMapId();

    /**
     * @brief 创建一张新地图的便捷方法
     *
     * @param x 中心X坐标
     * @param z 中心Z坐标
     * @param scale 缩放级别(0-4)
     * @param trackingPosition 是否追踪玩家位置
     * @param unlimitedTracking 是否无限追踪
     * @return 新创建的地图ID
     */
    i32 createMap(i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking);

    /**
     * @brief 每tick更新
     *
     * 遍历所有地图数据，更新脏区域标记。
     * 实际的网络包发送由ServerWorld在调用tick后负责。
     *
     * @param world 世界引用（用于获取地形数据等）
     */
    void tick(IWorld& world);

    /**
     * @brief 获取地图总数
     */
    [[nodiscard]] size_t mapCount() const { return m_mapData.size(); }

    /**
     * @brief 获取所有地图数据（用于遍历更新）
     */
    [[nodiscard]] const std::unordered_map<i32, std::unique_ptr<MapData>>& getAllMapData() const { return m_mapData; }

    /**
     * @brief 获取所有地图数据（可变，用于更新）
     */
    [[nodiscard]] std::unordered_map<i32, std::unique_ptr<MapData>>& getAllMapData() { return m_mapData; }

private:
    std::unordered_map<i32, std::unique_ptr<MapData>> m_mapData;
    MapIdTracker m_idTracker;
};

} // namespace mc::world::map
