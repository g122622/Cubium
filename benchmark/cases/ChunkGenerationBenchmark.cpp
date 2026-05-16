#include "../BenchmarkRegistry.hpp"

#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

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
        const i32 chunkX = config.parameters.at("chunkX").get<i32>();
        const i32 chunkZ = config.parameters.at("chunkZ").get<i32>();

        m_generator = std::make_unique<NoiseChunkGenerator>(seed, DimensionSettings::overworld());
        m_chunk = std::make_unique<ChunkPrimer>(chunkX, chunkZ);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_EVENT("benchmark.case", "ChunkGenerationBenchmark::runOnce");
        if (m_generator == nullptr || m_chunk == nullptr) {
            return Error(ErrorCode::IllegalState, "chunk_generation benchmark is not initialized");
        }

        WorldGenRegion* region = nullptr;
        m_generator->generateBiomes(*region, *m_chunk);
        return Result<void>::ok();
    }

    void tearDown() override
    {
        m_chunk.reset();
        m_generator.reset();
    }

private:
    std::unique_ptr<NoiseChunkGenerator> m_generator;
    std::unique_ptr<ChunkPrimer> m_chunk;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase("chunk_generation", []() {
        return std::make_unique<ChunkGenerationBenchmark>();
    });
    return true;
}();

} // namespace
} // namespace mc::benchmark
