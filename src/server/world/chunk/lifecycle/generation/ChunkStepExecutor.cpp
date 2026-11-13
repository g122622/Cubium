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

#include "server/world/chunk/lifecycle/generation/ChunkStepExecutor.hpp"
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

ChunkStepExecutor::ChunkStepExecutor(std::unique_ptr<IChunkGenerator> generator,
    DimensionId dimension,
    std::function<void(WorldGenRegion&)> configure,
    std::function<void(ChunkPrimer&, WorldGenRegion&)> light)
    : m_generator(std::move(generator))
    , m_dimension(dimension)
    , m_configure(std::move(configure))
    , m_light(std::move(light))
{}

void ChunkStepExecutor::configureRegion(WorldGenRegion& region) const
{
    region.setSeed(m_generator->seed());
    m_configure(region);
}

void ChunkStepExecutor::noteActivity()
{
    m_lastActivity.store(util::TimeUtils::getCurrentTimeMs(), std::memory_order::release);
}

u64 ChunkStepExecutor::lastActivity() const
{
    return m_lastActivity.load(std::memory_order::acquire);
}

void ChunkStepExecutor::execute(ChunkPrimer& chunk, const ChunkStatus& status, WorldGenRegion& region)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Chunk,
        "ChunkStepExecutor::executeStepTask",
        "chunkX",
        chunk.x(),
        "chunkZ",
        chunk.z(),
        "status",
        status.name());
    noteActivity();

    switch (status.ordinal()) {
        case ChunkStatuses::STRUCTURE_STARTS_ORDINAL:
            m_generator->generateStructureStarts(region, chunk);
            break;
        case ChunkStatuses::STRUCTURE_REFERENCES_ORDINAL:
            m_generator->generateStructureReferences(region, chunk);
            break;
        case ChunkStatuses::BIOMES_ORDINAL:
            m_generator->generateBiomes(region, chunk);
            chunk.getBiomes().compact();
            break;
        case ChunkStatuses::NOISE_ORDINAL:
            m_generator->generateNoise(region, chunk);
            break;
        case ChunkStatuses::SURFACE_ORDINAL:
            m_generator->buildSurface(region, chunk);
            break;
        case ChunkStatuses::CARVERS_ORDINAL:
            m_generator->applyCarvers(region, chunk);
            chunk.releaseGenOnlyData(status);
            break;
        case ChunkStatuses::FEATURES_ORDINAL:
            chunk.primeHeightmaps(HeightmapFlag::POST_FEATURES);
            m_generator->placeFeatures(region, chunk);
            chunk.releaseGenOnlyData(status);
            break;
        case ChunkStatuses::INITIALIZE_LIGHT_ORDINAL:
            // 光源和空段准备由 LIGHT 步骤的光照引擎完成，此阶段无需额外扫描。
            break;
        case ChunkStatuses::LIGHT_ORDINAL:
            m_light(chunk, region);
            break;
        case ChunkStatuses::SPAWN_ORDINAL: {
            std::vector<SpawnedEntityData> entities;
            m_generator->spawnInitialMobs(region, chunk, entities);
            for (auto& entityData : entities) {
                chunk.addSpawnedEntity(std::move(entityData));
            }
            break;
        }
        case ChunkStatuses::FULL_ORDINAL:
            // FULL 的数据发布在任务完成路径处理。
            break;
        default:
            MC_ASSERT_UNREACHABLE_MSG("Unexpected chunk generation step");
    }
}
} // namespace mc::server
