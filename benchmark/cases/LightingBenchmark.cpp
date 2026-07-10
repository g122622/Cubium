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

#include "../BenchmarkRegistry.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

using namespace mc::trace;

namespace mc::benchmark {
namespace {

class BenchmarkLightProvider final : public StarLightLightingProvider {
public:
    explicit BenchmarkLightProvider(ChunkData* chunk)
        : m_chunk(chunk)
    {}

    IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const IChunk* getChunkForLight(ChunkCoord x, ChunkCoord z) const override
    {
        if (m_chunk != nullptr && m_chunk->x() == x && m_chunk->z() == z) {
            return m_chunk;
        }
        return nullptr;
    }

    const BlockState* getBlockStateForLight(const BlockPos& pos) const override
    {
        if (m_chunk == nullptr) {
            return nullptr;
        }
        return m_chunk->getBlockState(pos.x & 0xF, pos.y, pos.z & 0xF);
    }

    IWorld* getWorld() override { return nullptr; }
    const IWorld* getWorld() const override { return nullptr; }
    void markLightChanged(LightType, const SectionPos&) override {}
    bool hasSkyLight() const override { return false; }
    i32 getMinBuildHeight() const override { return world::MIN_BUILD_HEIGHT; }
    i32 getMaxBuildHeight() const override { return world::MAX_BUILD_HEIGHT; }
    i32 getSectionCount() const override { return (world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT) >> 4; }

private:
    ChunkData* m_chunk;
};

class LightingBenchmark final : public IBenchmarkCase {
public:
    [[nodiscard]] std::string name() const override { return "lighting"; }

    [[nodiscard]] Result<void> validateConfig(const CaseRuntimeConfig& config) const override
    {
        if (!config.parameters.contains("updatesPerIteration") ||
            !config.parameters.at("updatesPerIteration").is_number_integer()) {
            return Error(ErrorCode::InvalidArgument, "lighting requires integer parameter: updatesPerIteration");
        }
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> setUp(const CaseRuntimeConfig&) override
    {
        VanillaBlocks::initialize();
        m_chunk = std::make_unique<ChunkData>(0, 0);
        m_chunk->setStatus(ChunkLoadStatus::Generated);
        m_provider = std::make_unique<BenchmarkLightProvider>(m_chunk.get());
        m_lightManager = std::make_unique<WorldLightManager>(m_provider.get(), true, false);
        m_chunk->setBlockState(8, 70, 8, &VanillaBlocks::GLOWSTONE->defaultState());
        m_lightManager->updateSectionStatus(SectionPos(0, 4, 0), false);
        m_lightManager->lightChunk(m_chunk.get(), false);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Benchmark.Run, "LightingBenchmark::runOnce");
        if (m_lightManager == nullptr) {
            return Error(ErrorCode::InvalidState, "lighting benchmark is not initialized");
        }
        m_lightManager->checkBlock(8, 70, 8);
        m_lightManager->tick(1024, false, true);
        return Result<void>::ok();
    }

    void tearDown() override
    {
        m_lightManager.reset();
        m_provider.reset();
        m_chunk.reset();
    }

private:
    std::unique_ptr<ChunkData> m_chunk;
    std::unique_ptr<BenchmarkLightProvider> m_provider;
    std::unique_ptr<WorldLightManager> m_lightManager;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase("lighting", []() { return std::make_unique<LightingBenchmark>(); });
    return true;
}();

} // namespace
} // namespace mc::benchmark
