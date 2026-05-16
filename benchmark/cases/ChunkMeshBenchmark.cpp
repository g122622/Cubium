#include "../BenchmarkRegistry.hpp"

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"

namespace mc::benchmark {
namespace {

class ChunkMeshBenchmark final : public IBenchmarkCase {
public:
    [[nodiscard]] std::string name() const override { return "chunk_mesh"; }

    [[nodiscard]] Result<void> validateConfig(const CaseRuntimeConfig& config) const override
    {
        if (!config.parameters.contains("fillBlock") || !config.parameters.at("fillBlock").is_string()) {
            return Error(ErrorCode::InvalidArgument, "chunk_mesh requires string parameter: fillBlock");
        }
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> setUp(const CaseRuntimeConfig&) override
    {
        VanillaBlocks::initialize();
        m_chunk = std::make_unique<ChunkData>(0, 0);

        for (i32 y = 0; y < 16; ++y) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 x = 0; x < 16; ++x) {
                    m_chunk->setBlockState(x, 64 + y, z, &VanillaBlocks::STONE->defaultState());
                }
            }
        }

        ChunkMesher::setLightingEnabled(false);
        ChunkMesher::setGreedyMeshing(true);
        ChunkMesher::setModelCache(nullptr);
        return Result<void>::ok();
    }

    [[nodiscard]] Result<void> runOnce() override
    {
        MC_TRACE_EVENT("benchmark.case", "ChunkMeshBenchmark::runOnce");
        MeshData mesh;
        ChunkMesher::generateMesh(*m_chunk, mesh, nullptr, nullptr);
        return Result<void>::ok();
    }

    void tearDown() override { m_chunk.reset(); }

private:
    std::unique_ptr<ChunkData> m_chunk;
};

const bool g_registered = []() {
    BenchmarkRegistry::instance().registerCase("chunk_mesh", []() {
        return std::make_unique<ChunkMeshBenchmark>();
    });
    return true;
}();

} // namespace
} // namespace mc::benchmark
