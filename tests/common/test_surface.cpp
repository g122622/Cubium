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

#include "common/core/Constants.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/gen/surface/SurfaceBuilder.hpp"
#include "common/world/gen/surface/SurfaceBuilders.hpp"
#include <gtest/gtest.h>

using namespace mc;

namespace {

[[nodiscard]] bool isTerracottaState(const BlockState* state)
{
    if (state == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::TERRACOTTA) || state->is(VanillaBlocks::WHITE_TERRACOTTA) ||
        state->is(VanillaBlocks::ORANGE_TERRACOTTA) || state->is(VanillaBlocks::MAGENTA_TERRACOTTA) ||
        state->is(VanillaBlocks::LIGHT_BLUE_TERRACOTTA) || state->is(VanillaBlocks::YELLOW_TERRACOTTA) ||
        state->is(VanillaBlocks::LIME_TERRACOTTA) || state->is(VanillaBlocks::PINK_TERRACOTTA) ||
        state->is(VanillaBlocks::GRAY_TERRACOTTA) || state->is(VanillaBlocks::LIGHT_GRAY_TERRACOTTA) ||
        state->is(VanillaBlocks::CYAN_TERRACOTTA) || state->is(VanillaBlocks::PURPLE_TERRACOTTA) ||
        state->is(VanillaBlocks::BLUE_TERRACOTTA) || state->is(VanillaBlocks::BROWN_TERRACOTTA) ||
        state->is(VanillaBlocks::GREEN_TERRACOTTA) || state->is(VanillaBlocks::RED_TERRACOTTA) ||
        state->is(VanillaBlocks::BLACK_TERRACOTTA);
}

} // namespace

// ============================================================================
// SurfaceBuilderConfig 测试
// ============================================================================

class SurfaceBuilderConfigTest : public ::testing::Test {
protected:
    void SetUp() override { VanillaBlocks::initialize(); }
};

TEST_F(SurfaceBuilderConfigTest, DefaultValues)
{
    SurfaceBuilderConfig config;
    EXPECT_EQ(config.topBlock, nullptr);
    EXPECT_EQ(config.underBlock, nullptr);
    EXPECT_EQ(config.underWaterBlock, nullptr);
}

TEST_F(SurfaceBuilderConfigTest, CustomValues)
{
    SurfaceBuilderConfig config(&VanillaBlocks::SAND->defaultState(),
        &VanillaBlocks::SAND->defaultState(),
        &VanillaBlocks::GRAVEL->defaultState());
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, GrassPreset)
{
    auto config = SurfaceBuilderConfig::grass();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, SandPreset)
{
    auto config = SurfaceBuilderConfig::sand();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::SAND));
}

TEST_F(SurfaceBuilderConfigTest, StonePreset)
{
    auto config = SurfaceBuilderConfig::stone();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::STONE));
}

TEST_F(SurfaceBuilderConfigTest, GravelPreset)
{
    auto config = SurfaceBuilderConfig::gravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, RedSandPreset)
{
    auto config = SurfaceBuilderConfig::redSand();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::RED_SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::RED_SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::RED_SAND));
}

TEST_F(SurfaceBuilderConfigTest, PodzolDirtGravelPreset)
{
    auto config = SurfaceBuilderConfig::podzolDirtGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::PODZOL));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, GravelOnlyPreset)
{
    auto config = SurfaceBuilderConfig::gravelOnly();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, GrassDirtGravelPreset)
{
    auto config = SurfaceBuilderConfig::grassDirtGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, StoneStoneGravelPreset)
{
    auto config = SurfaceBuilderConfig::stoneStoneGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, CoarseDirtDirtGravelPreset)
{
    auto config = SurfaceBuilderConfig::coarseDirtDirtGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::COARSE_DIRT));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, SandSandGravelPreset)
{
    auto config = SurfaceBuilderConfig::sandSandGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, GrassDirtSandPreset)
{
    auto config = SurfaceBuilderConfig::grassDirtSand();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::SAND));
}

TEST_F(SurfaceBuilderConfigTest, RedSandWhiteTerracottaGravelPreset)
{
    auto config = SurfaceBuilderConfig::redSandWhiteTerracottaGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::RED_SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::WHITE_TERRACOTTA));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, MyceliumDirtGravelPreset)
{
    auto config = SurfaceBuilderConfig::myceliumDirtGravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::MYCELIUM));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, NetherrackPreset)
{
    auto config = SurfaceBuilderConfig::netherrack();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::NETHERRACK));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::NETHERRACK));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::NETHERRACK));
}

// ============================================================================
// DefaultSurfaceBuilder 测试
// ============================================================================

class DefaultSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<DefaultSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<DefaultSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(DefaultSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "default");
}

TEST_F(DefaultSurfaceBuilderTest, BuildBasicSurface)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::grass();

    // 填充区块
    for (int y = 0; y < 64; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    // 构建地表
    builder->buildSurface(*random,
        chunk,
        biome,
        0,
        0,
        63,
        0.5, // surfaceNoise
        stone,
        water,
        63,    // seaLevel
        12345, // worldSeed
        config);

    // 验证地表已放置
    const BlockState* topBlock = chunk.getBlockState(0, 63, 0);
    EXPECT_TRUE(topBlock != nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::GRASS_BLOCK));
}

TEST_F(DefaultSurfaceBuilderTest, UnderwaterSurface)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::grass();

    // 填充区块到海平面以下
    for (int y = 0; y < 60; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    // 构建地表
    builder->buildSurface(*random, chunk, biome, 0, 0, 59, 0.5, stone, water, 63, 12345, config);

    // 水下应该使用gravel作为底板
    const BlockState* bottomBlock = chunk.getBlockState(0, 52, 0);
    if (bottomBlock != nullptr) {
        EXPECT_TRUE(bottomBlock->is(VanillaBlocks::GRAVEL) || bottomBlock->is(VanillaBlocks::STONE));
    }
}

// ============================================================================
// MountainSurfaceBuilder 测试
// ============================================================================

class MountainSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<MountainSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<MountainSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(MountainSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "mountain");
}

TEST_F(MountainSurfaceBuilderTest, HighNoiseUsesStone)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::grassDirtGravel();

    for (int y = 0; y < 100; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    // 高噪声应使用石头配置
    builder->buildSurface(*random,
        chunk,
        biome,
        0,
        0,
        99,
        2.0, // 高噪声
        stone,
        water,
        63,
        12345,
        config);

    // 表层应该是石头
    const BlockState* topBlock = chunk.getBlockState(0, 99, 0);
    EXPECT_TRUE(topBlock != nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::STONE));
}

TEST_F(MountainSurfaceBuilderTest, LowNoiseUsesGrass)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::grassDirtGravel();

    for (int y = 0; y < 100; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    // 低噪声应使用草地配置
    builder->buildSurface(*random,
        chunk,
        biome,
        0,
        0,
        99,
        0.5, // 低噪声
        stone,
        water,
        63,
        12345,
        config);

    const BlockState* topBlock = chunk.getBlockState(0, 99, 0);
    EXPECT_TRUE(topBlock != nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::GRASS_BLOCK));
}

// ============================================================================
// GravellyMountainSurfaceBuilder 测试
// ============================================================================

class GravellyMountainSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<GravellyMountainSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<GravellyMountainSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(GravellyMountainSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "gravelly_mountain");
}

// ============================================================================
// GiantTreeTaigaSurfaceBuilder 测试
// ============================================================================

class GiantTreeTaigaSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<GiantTreeTaigaSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<GiantTreeTaigaSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(GiantTreeTaigaSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "giant_tree_taiga");
}

// ============================================================================
// ShatteredSavannaSurfaceBuilder 测试
// ============================================================================

class ShatteredSavannaSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<ShatteredSavannaSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<ShatteredSavannaSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(ShatteredSavannaSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "shattered_savanna");
}

// ============================================================================
// SwampSurfaceBuilder 测试
// ============================================================================

class SwampSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<SwampSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<SwampSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(SwampSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "swamp");
}

// ============================================================================
// FrozenOceanSurfaceBuilder 测试
// ============================================================================

class FrozenOceanSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<FrozenOceanSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<FrozenOceanSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(FrozenOceanSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "frozen_ocean");
}

// ============================================================================
// BadlandsSurfaceBuilder 测试
// ============================================================================

class BadlandsSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<BadlandsSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<BadlandsSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(BadlandsSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "badlands");
}

TEST_F(BadlandsSurfaceBuilderTest, SetSeedInitializes)
{
    // 设置种子不应抛出异常
    EXPECT_NO_THROW(builder->setSeed(12345));
}

TEST_F(BadlandsSurfaceBuilderTest, BuildSurface)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Badlands);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::redSand();

    for (int y = 0; y < 80; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    builder->setSeed(12345);
    builder->buildSurface(*random, chunk, biome, 0, 0, 79, 0.5, stone, water, 63, 12345, config);

    // 表层应该是红沙或陶瓦
    const BlockState* topBlock = chunk.getBlockState(0, 79, 0);
    EXPECT_TRUE(topBlock != nullptr);
    bool isValidTop = topBlock->is(VanillaBlocks::RED_SAND) || isTerracottaState(topBlock);
    EXPECT_TRUE(isValidTop);
}

// ============================================================================
// ErodedBadlandsSurfaceBuilder 测试
// ============================================================================

class ErodedBadlandsSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<ErodedBadlandsSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<ErodedBadlandsSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(ErodedBadlandsSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "eroded_badlands");
}

// ============================================================================
// WoodedBadlandsSurfaceBuilder 测试
// ============================================================================

class WoodedBadlandsSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<WoodedBadlandsSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<WoodedBadlandsSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(WoodedBadlandsSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "wooded_badlands");
}

// ============================================================================
// NetherSurfaceBuilder 测试
// ============================================================================

class NetherSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<NetherSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<NetherSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(NetherSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "nether");
}

// ============================================================================
// NetherForestsSurfaceBuilder 测试
// ============================================================================

class NetherForestsSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<NetherForestsSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<NetherForestsSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(NetherForestsSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "nether_forests");
}

TEST_F(NetherForestsSurfaceBuilderTest, SetSeedInitializes)
{
    EXPECT_NO_THROW(builder->setSeed(12345));
}

// ============================================================================
// SoulSandValleySurfaceBuilder 测试
// ============================================================================

class SoulSandValleySurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<SoulSandValleySurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<SoulSandValleySurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(SoulSandValleySurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "soul_sand_valley");
}

// ============================================================================
// BasaltDeltasSurfaceBuilder 测试
// ============================================================================

class BasaltDeltasSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<BasaltDeltasSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<BasaltDeltasSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(BasaltDeltasSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "basalt_deltas");
}

// ============================================================================
// NoopSurfaceBuilder 测试
// ============================================================================

class NoopSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<NoopSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<NoopSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(NoopSurfaceBuilderTest, Name)
{
    EXPECT_STREQ(builder->name(), "noop");
}

TEST_F(NoopSurfaceBuilderTest, DoesNotModifyChunk)
{
    ChunkPrimer chunk(0, 0);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* water = &VanillaBlocks::WATER->defaultState();
    auto config = SurfaceBuilderConfig::grass();

    // 填充区块
    for (int y = 0; y < 64; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    // NoopSurfaceBuilder不应修改区块
    builder->buildSurface(*random, chunk, biome, 0, 0, 63, 0.5, stone, water, 63, 12345, config);

    // 验证区块仍然全是石头
    for (int y = 0; y < 64; ++y) {
        const BlockState* block = chunk.getBlockState(0, y, 0);
        EXPECT_TRUE(block != nullptr);
        EXPECT_TRUE(block->is(VanillaBlocks::STONE));
    }
}

// ============================================================================
// SurfaceBuilder 多态性测试
// ============================================================================

class SurfaceBuilderPolymorphismTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(SurfaceBuilderPolymorphismTest, AllBuildersAreValid)
{
    std::vector<std::unique_ptr<SurfaceBuilder>> builders;
    builders.push_back(std::make_unique<DefaultSurfaceBuilder>());
    builders.push_back(std::make_unique<MountainSurfaceBuilder>());
    builders.push_back(std::make_unique<GravellyMountainSurfaceBuilder>());
    builders.push_back(std::make_unique<SwampSurfaceBuilder>());
    builders.push_back(std::make_unique<FrozenOceanSurfaceBuilder>());
    builders.push_back(std::make_unique<BadlandsSurfaceBuilder>());
    builders.push_back(std::make_unique<ErodedBadlandsSurfaceBuilder>());
    builders.push_back(std::make_unique<WoodedBadlandsSurfaceBuilder>());
    builders.push_back(std::make_unique<GiantTreeTaigaSurfaceBuilder>());
    builders.push_back(std::make_unique<ShatteredSavannaSurfaceBuilder>());
    builders.push_back(std::make_unique<NetherSurfaceBuilder>());
    builders.push_back(std::make_unique<NetherForestsSurfaceBuilder>());
    builders.push_back(std::make_unique<SoulSandValleySurfaceBuilder>());
    builders.push_back(std::make_unique<BasaltDeltasSurfaceBuilder>());
    builders.push_back(std::make_unique<NoopSurfaceBuilder>());

    for (const auto& builder : builders) {
        EXPECT_NE(builder->name(), nullptr);
        EXPECT_GT(strlen(builder->name()), 0u);
    }
}

TEST_F(SurfaceBuilderPolymorphismTest, BuildSurfaceWithDifferentBuilders)
{
    ChunkPrimer chunk(0, 0);
    math::Random random(12345);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    auto config = SurfaceBuilderConfig::grass();

    // 填充区块
    for (int y = 0; y < 64; ++y) {
        chunk.setBlockState(0, y, 0, stone);
    }

    std::vector<std::unique_ptr<SurfaceBuilder>> builders;
    builders.push_back(std::make_unique<DefaultSurfaceBuilder>());
    builders.push_back(std::make_unique<MountainSurfaceBuilder>());
    builders.push_back(std::make_unique<GravellyMountainSurfaceBuilder>());

    // 每个构建器都应该能成功构建地表
    for (auto& builder : builders) {
        // 重新填充区块
        for (int y = 0; y < 64; ++y) {
            chunk.setBlockState(0, y, 0, stone);
        }

        // 不应该抛出异常
        EXPECT_NO_THROW(builder->buildSurface(
            random, chunk, biome, 0, 0, 63, 0.5, stone, &VanillaBlocks::WATER->defaultState(), 63, 12345, config));
    }
}

// ============================================================================
// SurfaceBuilderConfig 预设配置组合测试
// ============================================================================

TEST_F(SurfaceBuilderConfigTest, AllPresetsAreValid)
{
    std::vector<std::function<SurfaceBuilderConfig()>> presets = {
        SurfaceBuilderConfig::grass,
        SurfaceBuilderConfig::sand,
        SurfaceBuilderConfig::stone,
        SurfaceBuilderConfig::gravel,
        SurfaceBuilderConfig::redSand,
        SurfaceBuilderConfig::podzolDirtGravel,
        SurfaceBuilderConfig::gravelOnly,
        SurfaceBuilderConfig::grassDirtGravel,
        SurfaceBuilderConfig::stoneStoneGravel,
        SurfaceBuilderConfig::coarseDirtDirtGravel,
        SurfaceBuilderConfig::sandSandGravel,
        SurfaceBuilderConfig::grassDirtSand,
        SurfaceBuilderConfig::redSandWhiteTerracottaGravel,
        SurfaceBuilderConfig::myceliumDirtGravel,
        SurfaceBuilderConfig::netherrack,
    };

    for (const auto& preset : presets) {
        auto config = preset();
        EXPECT_NE(config.topBlock, nullptr);
        EXPECT_NE(config.underBlock, nullptr);
        EXPECT_NE(config.underWaterBlock, nullptr);
    }
}
