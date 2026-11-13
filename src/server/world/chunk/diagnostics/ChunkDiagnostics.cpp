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

#include "server/world/chunk/diagnostics/ChunkDiagnostics.hpp"
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

std::vector<ChunkDiagnosticSnapshot> ChunkDiagnostics::collect(const ChunkRegistry& registry)
{
    std::vector<ChunkDiagnosticSnapshot> rows;
    for (const auto& holder : registry.holders()) {
        ChunkDiagnosticSnapshot row;
        row.x = holder->x();
        row.z = holder->z();
        row.genStatus = holder->getCurrentGenStatus().name();
        row.reqStatus = holder->requestedGenStatus().name();
        row.safeToUnload = holder->isSafeToUnload();
        row.shouldLoad = holder->shouldLoad();
        if (auto* primer = holder->getCurrentChunk()) {
            row.hasPrimer = true;
            row.biomeBytes = primer->getBiomes().estimatedMemoryUsage();
            row.hasNoiseChunk = primer->hasNoiseChunk();
            row.hasCarvingMask = primer->hasCarvingMask();
        }
        rows.push_back(std::move(row));
    }
    return rows;
}

void ChunkDiagnostics::dump(const ChunkRegistry& registry)
{
    spdlog::info("Chunk diagnostics: holders={} loaded={}", registry.holderCount(), registry.loadedCount());
    size_t count = 0;
    for (const auto& holder : registry.holders()) {
        if (holder->isSafeToUnload() && !holder->getCurrentGenStatus().isBefore(holder->requestedGenStatus())) {
            continue;
        }
        if (++count <= 30) {
            spdlog::info(
                "Chunk ({},{}) gen={} target={} source={} level={} task={} blocking={} waiting={} users={} requests={}",
                holder->x(),
                holder->z(),
                holder->getCurrentGenStatus().name(),
                holder->requestedGenStatus().name(),
                static_cast<i32>(holder->sourceState()),
                holder->level(),
                holder->hasGenerationTask(),
                holder->blockingNeighbourCount(),
                holder->waitingNeighbours().size(),
                holder->neighboursUsingThisChunkCount(),
                holder->waiterCount());
        }
    }
    spdlog::info("Chunk diagnostics: incomplete holders={}", count);
}
} // namespace mc::server
