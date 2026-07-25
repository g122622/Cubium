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

#include "ClientMapDataCache.hpp"
#include "common/network/codec/MapDataPacket.hpp"
#include "common/util/text/ITextComponent.hpp"

namespace mc::client {

void ClientMapDataCache::handleMapDataPacket(const network::MapDataPacket& packet)
{
    auto& data = getOrCreateMapData(packet.mapId());
    packet.applyTo(data);
}

world::map::MapData* ClientMapDataCache::getMapData(i32 mapId)
{
    auto it = m_cache.find(mapId);
    if (it != m_cache.end()) {
        return it->second.get();
    }
    return nullptr;
}

const world::map::MapData* ClientMapDataCache::getMapData(i32 mapId) const
{
    auto it = m_cache.find(mapId);
    if (it != m_cache.end()) {
        return it->second.get();
    }
    return nullptr;
}

world::map::MapData& ClientMapDataCache::getOrCreateMapData(i32 mapId)
{
    auto it = m_cache.find(mapId);
    if (it != m_cache.end()) {
        return *it->second;
    }
    auto data = std::make_unique<world::map::MapData>(mapId);
    auto& ref = *data;
    m_cache[mapId] = std::move(data);
    return ref;
}

void ClientMapDataCache::clear() noexcept
{
    m_cache.clear();
}

} // namespace mc::client
