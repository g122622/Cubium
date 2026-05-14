#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"

#include <array>
#include <memory>
#include <vector>

namespace mc {
namespace {

class NoiseSurfaceParityTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();

        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                fillChunkWithStonePlateau(*chunk);
                fillChunkBiomes(*chunk, Biomes::Plains);
                chunk->updateAllHeightmaps();

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, m_chunks);
    }

    static void fillChunkWithStonePlateau(ChunkPrimer& chunk)
    {
        const BlockState* stone = &VanillaBlocks::STONE->defaultState();
        const BlockState* air = &VanillaBlocks::AIR->defaultState();

        for (i32 x = 0; x < 16; ++x) {
            for (i32 z = 0; z < 16; ++z) {
                for (i32 y = 0; y <= 63; ++y) {
                    chunk.setBlockState(x, y, z, stone);
                }
                for (i32 y = 64; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
                    chunk.setBlockState(x, y, z, air);
                }
            }
        }
    }

    static void fillChunkBiomes(ChunkPrimer& chunk, BiomeId biomeId)
    {
        BiomeContainer& biomes = chunk.getBiomes();
        for (i32 y = 0; y < BiomeContainer::BIOME_HEIGHT; ++y) {
            for (i32 z = 0; z < BiomeContainer::BIOME_DEPTH; ++z) {
                for (i32 x = 0; x < BiomeContainer::BIOME_WIDTH; ++x) {
                    biomes.setBiome(x, y, z, biomeId);
                }
            }
        }
    }

    std::array<IChunk*, 9> m_chunks{};
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(NoiseSurfaceParityTest, PlainsSurfaceUsesDirtUnderTopLayer)
{
    ASSERT_NE(m_region, nullptr);

    auto* centerChunk = dynamic_cast<ChunkPrimer*>(m_chunks[4]);
    ASSERT_NE(centerChunk, nullptr);

    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(123456789ULL, std::move(settings));

    generator.buildSurface(*m_region, *centerChunk);

    auto chunkData = centerChunk->toChunkData();
    ASSERT_NE(chunkData, nullptr);

    i32 checkedColumns = 0;
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const i32 topY = chunkData->getTopBlockY(HeightmapType::WorldSurfaceWG, x, z);
            if (topY < 1) {
                continue;
            }

            const BlockState* topState = chunkData->getBlockState(x, topY, z);
            if (topState == nullptr || !topState->is(VanillaBlocks::GRASS_BLOCK)) {
                continue;
            }

            ++checkedColumns;
            const BlockState* belowTop = chunkData->getBlockState(x, topY - 1, z);
            ASSERT_NE(belowTop, nullptr);
            EXPECT_TRUE(belowTop->is(VanillaBlocks::DIRT));
        }
    }

    EXPECT_GT(checkedColumns, 0);
}

} // namespace
} // namespace mc
