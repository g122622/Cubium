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

#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/chunk/NetherChunkGenerator.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

class NetherSurfaceParityTest : public ::testing::Test {
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
                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_centerChunk = dynamic_cast<ChunkPrimer*>(m_chunks[4]);
        std::vector<IChunk*> regionChunks = m_chunks;
        m_region = std::make_unique<WorldGenRegion>(0, 0, 1, std::move(regionChunks));
    }

    std::vector<IChunk*> m_chunks;
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
    ChunkPrimer* m_centerChunk = nullptr;
};

TEST_F(NetherSurfaceParityTest, BedrockUsesConfiguredRoofAndFloorAnchors)
{
    ASSERT_NE(m_region, nullptr);

    ASSERT_NE(m_centerChunk, nullptr);

    NetherChunkGenerator generator(246813579ULL);
    generator.buildSurface(*m_region, *m_centerChunk);

    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            const BlockState* floor = m_centerChunk->getBlockState(x, 0, z);
            ASSERT_NE(floor, nullptr);
            EXPECT_TRUE(floor->is(VanillaBlocks::BEDROCK));

            const BlockState* roof = m_centerChunk->getBlockState(x, 127, z);
            ASSERT_NE(roof, nullptr);
            EXPECT_TRUE(roof->is(VanillaBlocks::BEDROCK));

            const BlockState* lava = m_centerChunk->getBlockState(x, generator.lavaLevel(), z);
            ASSERT_NE(lava, nullptr);
            EXPECT_TRUE(lava->is(VanillaBlocks::LAVA));
        }
    }
}

} // namespace
} // namespace mc
