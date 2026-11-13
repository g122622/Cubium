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

#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/TimeUtils.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/world/chunk/base/completion/ChunkRequestCompletion.hpp"
#include "server/world/chunk/base/pipeline/ChunkPyramid.hpp"
#include "server/world/chunk/lifecycle/state/ChunkRegistry.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <utility>
#include <spdlog/spdlog.h>
using namespace mc::trace;

namespace mc::server {
using namespace mc::world::chunk;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;

ChunkRegistry::ChunkRegistry(std::function<i32(ChunkCoord, ChunkCoord)> level, Holder::UnloadStateObserver observer)
    : m_level(std::move(level))
    , m_observer(std::move(observer))
{}

ChunkRegistry::Holder& ChunkRegistry::getOrCreate(ChunkCoord x, ChunkCoord z)
{
    std::lock_guard lock(m_mutex);
    auto& slot = m_holders[ChunkPos(x, z).toId()];
    if (!slot) {
        slot = std::make_shared<Holder>(x, z);
        slot->setLevel(m_level(x, z));
        slot->setUnloadStateObserver(m_observer);
    }
    return *slot;
}

std::shared_ptr<ChunkRegistry::Holder> ChunkRegistry::findShared(ChunkCoord x, ChunkCoord z) const
{
    std::lock_guard lock(m_mutex);
    auto it = m_holders.find(ChunkPos(x, z).toId());
    return it == m_holders.end() ? nullptr : it->second;
}

ChunkRegistry::Holder* ChunkRegistry::find(ChunkCoord x, ChunkCoord z) const
{
    return findShared(x, z).get();
}

std::shared_ptr<ChunkRegistry::Data> ChunkRegistry::chunk(ChunkCoord x, ChunkCoord z) const
{
    std::lock_guard lock(m_mutex);
    auto it = m_chunks.find(ChunkPos(x, z).toId());
    return it == m_chunks.end() ? nullptr : it->second;
}

ChunkRegistry::Data* ChunkRegistry::publish(ChunkCoord x, ChunkCoord z, std::shared_ptr<Data> data, Holder& owner)
{
    MC_ASSERT_RELEASE(data != nullptr);
    std::shared_ptr<Holder> current;
    {
        std::lock_guard lock(m_mutex);
        auto it = m_holders.find(ChunkPos(x, z).toId());
        if (it == m_holders.end() || it->second.get() != &owner) {
            return nullptr;
        }
        current = it->second;
        m_chunks[ChunkPos(x, z).toId()] = std::move(data);
    }
    current->markDataPublished(ChunkStatuses::FULL);
    completeReady(*current);
    return chunk(x, z).get();
}

bool ChunkRegistry::retire(ChunkCoord x, ChunkCoord z, const std::shared_ptr<Holder>& owner)
{
    std::lock_guard lock(m_mutex);
    auto key = ChunkPos(x, z).toId();
    auto it = m_holders.find(key);
    if (it != m_holders.end()) {
        if (it->second != owner) {
            return false;
        }
        m_holders.erase(it);
    } else if (owner) {
        return false;
    }
    m_chunks.erase(key);
    return true;
}

std::vector<std::shared_ptr<ChunkRegistry::Holder>> ChunkRegistry::holders() const
{
    std::lock_guard lock(m_mutex);
    std::vector<std::shared_ptr<Holder>> result;
    result.reserve(m_holders.size());
    for (const auto& [key, holder] : m_holders) {
        (void)key;
        result.push_back(holder);
    }
    return result;
}

std::vector<std::shared_ptr<ChunkRegistry::Data>> ChunkRegistry::chunks() const
{
    std::lock_guard lock(m_mutex);
    std::vector<std::shared_ptr<Data>> result;
    result.reserve(m_chunks.size());
    for (const auto& [key, data] : m_chunks) {
        (void)key;
        result.push_back(data);
    }
    return result;
}

size_t ChunkRegistry::loadedCount() const
{
    std::lock_guard lock(m_mutex);
    return m_chunks.size();
}

size_t ChunkRegistry::holderCount() const
{
    std::lock_guard lock(m_mutex);
    return m_holders.size();
}

void ChunkRegistry::completeReady(Holder& holder)
{
    auto data = chunk(holder.x(), holder.z());
    auto* primer = holder.getCurrentChunk();
    auto* result = data ? data.get() : (primer ? primer->getChunkData() : nullptr);
    completeChunkRequests(holder.takeReadyWaiters(), result != nullptr, result);
}

void ChunkRegistry::clear()
{
    std::unordered_map<u64, std::shared_ptr<Holder>> holders;
    {
        std::lock_guard lock(m_mutex);
        holders.swap(m_holders);
        m_chunks.clear();
    }
    for (auto& [key, holder] : holders) {
        (void)key;
        completeChunkRequests(holder->takeAllWaiters(), false, nullptr);
    }
}

} // namespace mc::server
