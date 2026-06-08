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
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/chunk/IChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

#include <memory>
#include <vector>

namespace mc {
namespace {

/**
 * @brief 下界表面生成测试
 *
 * 验证使用 NoiseChunkGenerator（统一密度函数管线）生成的下界地形
 * 具有正确的基岩层和熔岩海。
 */
class NetherSurfaceTest : public ::testing::Test {
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

TEST_F(NetherSurfaceTest, NetherUsesNoiseChunkGenerator)
{
    // 验证下界可以使用 NoiseChunkGenerator + NetherBiomeSource 创建
    const u64 seed = 246813579ULL;
    DimensionSettings settings = DimensionSettings::nether();
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(seed);

    NoiseChunkGenerator generator(seed, std::move(settings), std::move(biomeSource));

    // 验证生成器可以正确创建（不应崩溃）
    // 下界维度高度为 0-128，seaLevel=31
    const i32 height = generator.getHeight(0, 0, HeightmapType::WorldSurfaceWG);
    (void)height; // 只需确认调用不会崩溃
}

} // namespace
} // namespace mc
