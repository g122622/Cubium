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

/**
 * @file StructureBiomeSamplePointTest.cpp
 * @brief 结构生物群系校验的采样点契约
 *
 * 群系校验必须落在**结构自己算出的候选生成点**上，并且该点要按 QuartPos.fromBlock
 * 换算成四分格后取噪声群系；用区块中心/角落这类固定近似点、或用方块级 fuzzy 选格，
 * 都会在群系边界附近得出相反结论（合法结构被误杀、非法结构被放过）。
 *
 * 这里用记录型生成器把「采样点」这件事本身钉死，与具体某个结构的布局无关。
 */

#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeTags.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/structure/Structure.hpp"

#include <memory>
#include <vector>

using namespace mc;
using namespace mc::world::gen::structure;

namespace {

/// 采样记录：一次噪声群系查询的四分坐标
struct QuartSample {
    i32 x;
    i32 y;
    i32 z;
};

/**
 * @brief 只记录噪声群系查询的生成器桩
 *
 * 结构基类的群系校验只用到 getNoiseBiome / getBiomeAtBlock / getHeight，
 * 其余生成阶段接口留空实现（本测试不推进区块生成状态）。
 */
class RecordingChunkGenerator final : public IChunkGenerator {
public:
    explicit RecordingChunkGenerator(BiomeId biomeId)
        : m_biomeId(biomeId)
    {}

    void generateStructureStarts(WorldGenRegion&, ChunkPrimer&) override {}
    void generateStructureReferences(WorldGenRegion&, ChunkPrimer&) override {}
    void generateBiomes(WorldGenRegion&, ChunkPrimer&) override {}
    void generateNoise(WorldGenRegion&, ChunkPrimer&) override {}
    void buildSurface(WorldGenRegion&, ChunkPrimer&) override {}
    void applyCarvers(WorldGenRegion&, ChunkPrimer&) override {}
    void placeFeatures(WorldGenRegion&, ChunkPrimer&) override {}

    [[nodiscard]] i32 spawnInitialMobs(
        WorldGenRegion&, ChunkPrimer&, std::vector<SpawnedEntityData>&) override
    {
        return 0;
    }

    [[nodiscard]] BiomeId getBiome(i32, i32, i32) const override
    {
        return m_biomeId;
    }

    [[nodiscard]] BiomeId getNoiseBiome(i32 noiseX, i32 noiseY, i32 noiseZ) const override
    {
        m_samples.push_back({noiseX, noiseY, noiseZ});
        return m_biomeId;
    }

    [[nodiscard]] i32 getHeight(i32, i32, HeightmapType) const override
    {
        return m_height;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] const DimensionSettings& settings() const override
    {
        return m_settings;
    }

    [[nodiscard]] i32 seaLevel() const override
    {
        return m_settings.seaLevel;
    }

    void setHeight(i32 height)
    {
        m_height = height;
    }

    [[nodiscard]] const std::vector<QuartSample>& samples() const
    {
        return m_samples;
    }

private:
    BiomeId m_biomeId;
    i32 m_height = 64;
    DimensionSettings m_settings{};
    mutable std::vector<QuartSample> m_samples;
};

/// 无构件的空结构：只用来调用基类的群系校验辅助函数
class PlainStructure final : public Structure {
public:
    PlainStructure()
        : Structure(ResourceLocation("minecraft:plain_test_structure"))
    {}

    [[nodiscard]] const std::string& name() const override
    {
        static const std::string kName = "minecraft:plain_test_structure";
        return kName;
    }

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator&, math::IRandom&, i32 chunkX, i32 chunkZ) const override
    {
        return makeStructureStart(chunkX, chunkZ);
    }
};

} // namespace

class StructureBiomeSamplePointTest : public ::testing::Test {};

/**
 * @test 候选生成点按 QuartPos.fromBlock 采样
 *
 * 候选点为 (x=37, y=70, z=-13) 时，必须查询四分格 (9, 17, -4)：
 * 算术右移 2 位是 QuartPos.fromBlock 的定义（负数向下取整）。
 */
TEST_F(StructureBiomeSamplePointTest, CandidatePointIsSampledAtQuartResolution)
{
    PlainStructure structure;
    RecordingChunkGenerator generator(world::biome::Biomes::Plains);
    structure.setBiomeTag(&world::biome::BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS());

    MC_UNUSED(structure.isValidBiomeAt(generator, BlockPos(37, 70, -13)));

    ASSERT_EQ(generator.samples().size(), 1u) << "exactly one biome sample per validation";
    EXPECT_EQ(generator.samples().front().x, 9) << "37 >> 2";
    EXPECT_EQ(generator.samples().front().y, 17) << "70 >> 2";
    EXPECT_EQ(generator.samples().front().z, -4) << "-13 >> 2 rounds toward negative infinity";
}

/**
 * @test 校验落在调用方给定的点上，不塌缩到区块中心
 *
 * 若实现里改成"区块中心"（block + 8）这类固定近似点，传入的候选点就不会被查询。
 */
TEST_F(StructureBiomeSamplePointTest, SamplePointFollowsCallerNotChunkCenter)
{
    PlainStructure structure;
    RecordingChunkGenerator generator(world::biome::Biomes::Plains);
    structure.setBiomeTag(&world::biome::BiomeTags::HAS_STRUCTURE_VILLAGE_PLAINS());

    // 区块 0 的中心是 (8, ?, 8)；这里传一个明显不同的点。
    const BlockPos candidate(21, 100, 44);
    MC_UNUSED(structure.isValidBiomeAt(generator, candidate));

    ASSERT_EQ(generator.samples().size(), 1u);
    const QuartSample& sample = generator.samples().front();
    EXPECT_EQ(sample.x, candidate.x >> 2);
    EXPECT_EQ(sample.y, candidate.y >> 2);
    EXPECT_EQ(sample.z, candidate.z >> 2);
    EXPECT_NE(sample.x, 8 >> 2) << "sample must not collapse to the chunk center";
    EXPECT_NE(sample.z, 8 >> 2) << "sample must not collapse to the chunk center";
}

/**
 * @test onTopOfChunkCenter 的 X/Z 是区块中心、Y 取高度图
 *
 * 多处结构用它构造候选生成点，这三个分量都必须固定下来：
 * X/Z 是区块中心（而非最小角），Y 来自传入的高度图而不是固定常量。
 */
TEST_F(StructureBiomeSamplePointTest, OnTopOfChunkCenterUsesCenterAndHeightmap)
{
    PlainStructure structure;
    RecordingChunkGenerator generator(world::biome::Biomes::Plains);
    generator.setHeight(97);

    // 区块 (2, -1)：最小角是 (32, -16)，中心是 (40, -8)。
    const BlockPos pos = structure.onTopOfChunkCenter(generator, 2, -1, HeightmapType::WorldSurfaceWG);

    EXPECT_EQ(pos.x, 40);
    EXPECT_EQ(pos.z, -8);
    EXPECT_EQ(pos.y, 97) << "Y must come from the heightmap, not a fixed constant";
}