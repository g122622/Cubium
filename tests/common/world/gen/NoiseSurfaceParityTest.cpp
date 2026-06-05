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

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
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

        m_chunks.resize(9);
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

        // 保存中心区块指针用于测试
        m_centerChunk = dynamic_cast<ChunkPrimer*>(m_chunks[4]);
        std::vector<IChunk*> regionChunks = m_chunks;
        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(regionChunks));
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
        for (i32 section = 0; section < BiomeContainer::SECTION_COUNT; ++section) {
            for (i32 y = 0; y < BiomeContainer::VERT_SIZE; ++y) {
                for (i32 z = 0; z < BiomeContainer::HORIZ_SIZE; ++z) {
                    for (i32 x = 0; x < BiomeContainer::HORIZ_SIZE; ++x) {
                        biomes.setBiome(section, x, y, z, biomeId);
                    }
                }
            }
        }
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    ChunkPrimer* m_centerChunk = nullptr;
};

TEST_F(NoiseSurfaceParityTest, PlainsSurfaceUsesDirtUnderTopLayer)
{
    ASSERT_NE(m_region, nullptr);

    ASSERT_NE(m_centerChunk, nullptr);

    DimensionSettings settings = DimensionSettings::overworld();
    NoiseChunkGenerator generator(
        123456789ULL, std::move(settings), mc::world::biome::source::MultiNoiseBiomeSource::createOverworld(123456789ULL, false));

    generator.buildSurface(*m_region, *m_centerChunk);

    auto chunkData = m_centerChunk->toChunkData();
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
