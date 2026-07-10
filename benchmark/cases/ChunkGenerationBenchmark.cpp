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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "../BenchmarkRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkPrimer.hpp"
#include "common/world/chunk/gen/ChunkPyramid.hpp"
#include "common/world/chunk/gen/ChunkStatus.hpp"
#include "common/world/chunk/gen/ChunkStep.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

using namespace mc::trace;

namespace mc::benchmark {

using mc::world::chunk::ChunkPrimer;
using mc::world::chunk::ChunkPyramid;
using mc::world::chunk::ChunkStatus;
using mc::world::chunk::ChunkStep;
namespace ChunkStatuses = mc::world::chunk::ChunkStatuses;
using mc::world::chunk::IChunk;
namespace {

class ChunkGenerationBenchmark final : public IBenchmarkCase {
public:
    [[nodiscard]] std::string name() const override { return "chunk_generation"; }

    [[nodiscard]] Result<void> validateConfig(const CaseRuntimeConfig& config) const override
    {
        if (!config.parameters.contains("seed") || !config.parameters.at("seed").is_number_unsigned()) {
            return Error(ErrorCode::InvalidArgument, "chunk_generation requires unsigned integer parameter: seed");
        }
        if (!config.parameters.contains("chunkX") || !config.parameters.at("chunkX").is_number_integer()) {
            return Error(ErrorCode::InvalidArgument, "chunk_generation requires integer parameter: chunkX");
        }
        if (!config.parameters.contains("chunkZ") || !config.parameters.at("chunkZ").is_number_integer()) {
            return Error(ErrorCode::InvalidArgument, "chunk_generation requires integer parameter: chunkZ");
        }
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> setUp(const CaseRuntimeConfig& config) override
    {
        VanillaBlocks::initialize();
        const u64 seed = config.parameters.at("seed").get<u64>();
        m_chunkX = config.parameters.at("chunkX").get<i32>();
        m_chunkZ = config.parameters.at("chunkZ").get<i32>();

        auto settings = DimensionSettings::overworld();
        auto randomState = world::gen::RandomState::create(settings, seed);
        auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
        m_generator =
            std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));
        m_random = std::make_unique<math::Random>(seed);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "ChunkGenerationBenchmark::runOnce");
        if (m_generator == nullptr) {
            return Error(ErrorCode::InvalidState, "chunk_generation benchmark is not initialized");
        }

        // 创建新的区块 primer
        auto chunk = std::make_unique<ChunkPrimer>(m_chunkX, m_chunkZ);

        // 完整的区块生成流程，参考 ServerChunkManager::enqueueChunkGenerationAsync
        // 遍历所有生成阶段
        const auto& allStatuses = ChunkStatus::getAll();
        for (const auto& status : allStatuses) {
            // 跳过 EMPTY 和 FULL 阶段
            if (status.ordinal() <= ChunkStatuses::EMPTY_ORDINAL || status.ordinal() >= ChunkStatuses::FULL_ORDINAL) {
                continue;
            }

            // 跳过 LIGHT 和 SPAWN 阶段（需要额外依赖）
            if (status.ordinal() == ChunkStatuses::LIGHT_ORDINAL || status.ordinal() == ChunkStatuses::SPAWN_ORDINAL) {
                continue;
            }

            // 根据 accumulatedRadius 创建合适大小的 WorldGenRegion
            const ChunkStep& step = ChunkPyramid::generationPyramid().getStepTo(status);
            const i32 regionRadius = std::max(0, step.accumulatedRadius());
            const i32 diameter = regionRadius * 2 + 1;
            const size_t regionSize = static_cast<size_t>(diameter * diameter);
            std::vector<IChunk*> neighbors(regionSize, nullptr);

            // 为需要邻居的阶段创建空的 ChunkPrimer
            std::vector<std::unique_ptr<ChunkPrimer>> neighborPrimers;
            if (regionRadius > 0) {
                neighborPrimers.resize(regionSize);
                for (i32 dz = -regionRadius; dz <= regionRadius; ++dz) {
                    for (i32 dx = -regionRadius; dx <= regionRadius; ++dx) {
                        const size_t index = static_cast<size_t>((dz + regionRadius) * diameter + (dx + regionRadius));
                        if (dx == 0 && dz == 0) {
                            neighbors[index] = chunk.get();
                        } else {
                            neighborPrimers[index] = std::make_unique<ChunkPrimer>(m_chunkX + dx, m_chunkZ + dz);
                            neighbors[index] = neighborPrimers[index].get();
                        }
                    }
                }
            } else {
                // taskRange = 0，只有中心区块
                neighbors[0] = chunk.get();
            }

            WorldGenRegion region(m_chunkX, m_chunkZ, regionRadius, std::move(neighbors));
            region.setSeed(m_random->nextLong());

            // 执行对应阶段的生成任务
            if (status == ChunkStatuses::STRUCTURE_STARTS) {
                m_generator->generateStructureStarts(region, *chunk);
            } else if (status == ChunkStatuses::STRUCTURE_REFERENCES) {
                m_generator->generateStructureReferences(region, *chunk);
            } else if (status == ChunkStatuses::BIOMES) {
                m_generator->generateBiomes(region, *chunk);
            } else if (status == ChunkStatuses::NOISE) {
                m_generator->generateNoise(region, *chunk);
            } else if (status == ChunkStatuses::SURFACE) {
                m_generator->buildSurface(region, *chunk);
            } else if (status == ChunkStatuses::CARVERS) {
                m_generator->applyCarvers(region, *chunk);
            } else if (status == ChunkStatuses::FEATURES) {
                m_generator->placeFeatures(region, *chunk);
            } else if (status == ChunkStatuses::INITIALIZE_LIGHT) {
                chunk->initializeLightSources();
            } else if (status == ChunkStatuses::LIGHT) {
                // 光照传播由光照引擎异步处理
            } else if (status == ChunkStatuses::SPAWN) {
                std::vector<SpawnedEntityData> entities;
                m_generator->spawnInitialMobs(region, *chunk, entities);
                for (auto& entityData : entities) {
                    chunk->addSpawnedEntity(std::move(entityData));
                }
            }

            chunk->setChunkStatus(status);
        }

        return Result<void>::ok();
    }

    void tearDown() override
    {
        m_random.reset();
        m_generator.reset();
    }

private:
    std::unique_ptr<NoiseChunkGenerator> m_generator;
    std::unique_ptr<math::Random> m_random;
    i32 m_chunkX = 0;
    i32 m_chunkZ = 0;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase(
        "chunk_generation", []() { return std::make_unique<ChunkGenerationBenchmark>(); });
    return true;
}();

} // namespace
} // namespace mc::benchmark
