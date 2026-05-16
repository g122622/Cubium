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

#include "common/world/WorldConstants.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/util/math/random/Random.hpp"

namespace mc::benchmark {
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

        m_generator = std::make_unique<NoiseChunkGenerator>(seed, DimensionSettings::overworld());
        m_chunk = std::make_unique<ChunkPrimer>(m_chunkX, m_chunkZ);

        // 创建 WorldGenRegion，包含单个区块（半径为 0）
        std::array<IChunk*, 1> chunks = {m_chunk.get()};
        m_region = std::make_unique<WorldGenRegion>(m_chunkX, m_chunkZ, chunks);
        m_region->setSeed(seed);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_EVENT("benchmark.case", "ChunkGenerationBenchmark::runOnce");
        if (m_generator == nullptr || m_chunk == nullptr || m_region == nullptr) {
            return Error(ErrorCode::InvalidState, "chunk_generation benchmark is not initialized");
        }

        // 重置区块状态以便重新生成
        m_chunk = std::make_unique<ChunkPrimer>(m_chunkX, m_chunkZ);
        std::array<IChunk*, 1> chunks = {m_chunk.get()};
        m_region = std::make_unique<WorldGenRegion>(m_chunkX, m_chunkZ, chunks);

        m_generator->generateBiomes(*m_region, *m_chunk);
        return Result<void>::ok();
    }

    void tearDown() override
    {
        m_region.reset();
        m_chunk.reset();
        m_generator.reset();
    }

private:
    std::unique_ptr<NoiseChunkGenerator> m_generator;
    std::unique_ptr<ChunkPrimer> m_chunk;
    std::unique_ptr<WorldGenRegion> m_region;
    i32 m_chunkX = 0;
    i32 m_chunkZ = 0;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase("chunk_generation", []() {
        return std::make_unique<ChunkGenerationBenchmark>();
    });
    return true;
}();

} // namespace
} // namespace mc::benchmark
