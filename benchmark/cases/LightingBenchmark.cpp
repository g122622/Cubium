#include "../BenchmarkRegistry.hpp"

#include "common/world/WorldConstants.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/manager/WorldLightManager.hpp"

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
        if (!config.parameters.contains("updatesPerIteration") || !config.parameters.at("updatesPerIteration").is_number_integer()) {
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
        MC_TRACE_EVENT("benchmark.case", "LightingBenchmark::runOnce");
        if (m_lightManager == nullptr) {
            return Error(ErrorCode::IllegalState, "lighting benchmark is not initialized");
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
    BenchmarkRegistry::instance().registerCase("lighting", []() {
        return std::make_unique<LightingBenchmark>();
    });
    return true;
}();

} // namespace
} // namespace mc::benchmark
