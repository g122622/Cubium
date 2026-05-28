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
#include "util/assert/AssertMacros.hpp"
#include "util/text/ITextComponent.hpp"

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

} // namespace mc::world::map
