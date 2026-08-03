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

#include "MapDataManager.hpp"
#include "common/core/Types.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include "common/world/map/MapData.hpp"
#include "util/assert/AssertMacros.hpp"
#include "util/text/ITextComponent.hpp"
#include "world/IWorld.hpp"
#include <memory>
#include <utility>

namespace mc::world::map {

MapData* MapDataManager::getMapData(i32 mapId)
{
    auto it = m_mapData.find(mapId);
    if (it != m_mapData.end()) {
        return it->second.get();
    }
    return nullptr;
}

const MapData* MapDataManager::getMapData(i32 mapId) const
{
    auto it = m_mapData.find(mapId);
    if (it != m_mapData.end()) {
        return it->second.get();
    }
    return nullptr;
}

MapData* MapDataManager::createMapData(i32 mapId)
{
    if (m_mapData.count(mapId) > 0) {
        return nullptr;
    }
    auto data = std::make_unique<MapData>(mapId);
    auto* ptr = data.get();
    m_mapData[mapId] = std::move(data);
    return ptr;
}

i32 MapDataManager::allocateMapId()
{
    return m_idTracker.getNextId();
}

i32 MapDataManager::createMap(i32 x, i32 z, i32 scale, bool trackingPosition, bool unlimitedTracking)
{
    i32 mapId = allocateMapId();
    auto* data = createMapData(mapId);
    MC_ASSERT(data != nullptr);

    i32 centerX, centerZ;
    MapData::calculateMapCenter(static_cast<f64>(x), static_cast<f64>(z), scale, centerX, centerZ);
    data->initialize(centerX, centerZ, scale, trackingPosition, unlimitedTracking, MapDimensionId::Overworld);

    return mapId;
}

void MapDataManager::tick(IWorld& world)
{
    (void)world;
    // 遍历所有地图，重置脏标记
    // 实际的地形更新和玩家追踪由FilledMapItem::inventoryTick()负责
    // 网络包发送由ServerWorld在tick后处理
    for (auto& [mapId, mapData] : m_mapData) {
        MC_UNUSED(mapId);
        MC_ASSERT_RELEASE(mapData != nullptr);
        if (mapData->isDirty()) {
            // 脏标记由地形更新和装饰物更新设置
            // tick时重置脏标记，网络包发送在ServerWorld中处理
            mapData->clearDirty();
        }
    }
}

} // namespace mc::world::map
