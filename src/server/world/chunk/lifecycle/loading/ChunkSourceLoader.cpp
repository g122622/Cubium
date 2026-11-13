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

#include "server/world/chunk/lifecycle/loading/ChunkSourceLoader.hpp"
#include "common/profiler/TraceCategories.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "server/world/chunk/base/request/ChunkRequestPriority.hpp"
#include "server/world/gen/chunk/ChunkPrimer.hpp"
#include <spdlog/spdlog.h>
using namespace mc::trace;

namespace mc::server {
namespace ChunkStatuses = world::chunk::ChunkStatuses;

ChunkSourceLoader::ChunkSourceLoader(ChunkRegistry& registry,
    IChunkWorldLifecycle& integration,
    std::function<world::storage::SingleLevelStorageManager*()> storage,
    DimensionId dimension,
    Advance advance,
    std::function<void(Holder&)> ready,
    std::function<void(Holder&)> fail)
    : m_registry(registry)
    , m_integration(integration)
    , m_storage(std::move(storage))
    , m_dimension(dimension)
    , m_advance(std::move(advance))
    , m_ready(std::move(ready))
    , m_fail(std::move(fail))
{}

void ChunkSourceLoader::resolve(Holder& holder)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkSourceLoader::resolve");
    auto owner = m_registry.findShared(holder.x(), holder.z());
    if (owner.get() != &holder) {
        return;
    }
    auto* storage = m_storage();
    if (storage == nullptr) {
        m_advance(holder, holder.noteStorageMissing());
        return;
    }
    {
        std::lock_guard lock(m_pendingLoadTasksMutex);
        auto [it, inserted] = m_pendingLoadTasks.try_emplace(posToKey(holder.x(), holder.z()), owner);
        MC_ASSERT_RELEASE(it->second == owner);
        if (!inserted) {
            return;
        }
    }
    storage->loadChunkAsyncCallback(
        holder.x(),
        holder.z(),
        m_dimension,
        [completion = m_completions, owner](
            ChunkCoord, ChunkCoord, DimensionId, Result<std::optional<ChunkData>> result) {
            completion->push({owner, std::move(result)});
        },
        holder.abortSignal(),
        chunkTaskPriority(holder.requestPriority()));
}

void ChunkSourceLoader::_onChunkLoadComplete(PendingLoadComplete&& completion)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkSourceLoader::onChunkLoadComplete");
    MC_ASSERT_RELEASE(m_drainingLoadCompletes);
    auto& owner = completion.holder;
    const auto x = owner->x(), z = owner->z();
    {
        std::lock_guard lock(m_pendingLoadTasksMutex);
        auto it = m_pendingLoadTasks.find(posToKey(x, z));
        if (it == m_pendingLoadTasks.end() || it->second != owner) {
            return;
        }
        m_pendingLoadTasks.erase(it);
    }
    if (m_registry.findShared(x, z) != owner) {
        return;
    }
    auto& result = completion.result;
    if (result.failed()) {
        if (result.error().code() != ErrorCode::InvalidState) {
            spdlog::error("Stored chunk could not be loaded ({}, {}): {}", x, z, result.error().message());
            m_fail(*owner);
            return;
        }
        spdlog::warn("Async load chunk cancelled ({}, {}): {}", x, z, result.error().message());
        m_advance(*owner, owner->noteStorageMissing());
        return;
    }
    auto& loaded = result.value();
    if (!loaded) {
        m_advance(*owner, owner->noteStorageMissing());
        return;
    }
    auto data = std::make_unique<ChunkData>(std::move(*loaded));
    data->setStatus(ChunkLoadStatus::Loaded);
    auto primer = std::make_unique<world::chunk::ChunkPrimer>(std::move(data));
    auto sharedData = primer->shareChunkData();
    owner->publishStorageLoaded(std::move(primer));
    if (m_registry.publish(x, z, std::move(sharedData), *owner)) {
        m_integration.enqueue(x, z, {}, false);
    }
    m_ready(*owner);
}

void ChunkSourceLoader::drain()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk, "ChunkSourceLoader::drain");
    if (m_drainingLoadCompletes) {
        return;
    }
    ChunkCompletionGuard guard(m_drainingLoadCompletes);
    m_completions->drain(std::chrono::milliseconds(1), [this](auto& item) {
        _onChunkLoadComplete(std::move(item));
    });
}

void ChunkSourceLoader::forget(ChunkCoord x, ChunkCoord z, const std::shared_ptr<Holder>& holder)
{
    std::lock_guard lock(m_pendingLoadTasksMutex);
    auto it = m_pendingLoadTasks.find(posToKey(x, z));
    if (it != m_pendingLoadTasks.end() && it->second == holder) {
        m_pendingLoadTasks.erase(it);
    }
}

void ChunkSourceLoader::stop()
{
    m_completions->close();
}

void ChunkSourceLoader::clear()
{
    m_completions->close();
    std::lock_guard lock(m_pendingLoadTasksMutex);
    m_pendingLoadTasks.clear();
}

bool ChunkSourceLoader::pending() const
{
    return m_completions->pending();
}
} // namespace mc::server
