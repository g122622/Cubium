#include <gtest/gtest.h>
#include "common/world/gen/surface/SurfaceBuilder.hpp"
#include "common/world/gen/surface/SurfaceBuilders.hpp"
#include "common/world/chunk/ChunkPrimer.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/core/Constants.hpp"

using namespace mc;

namespace {

[[nodiscard]] bool isTerracottaState(const BlockState* state) {
    if (state == nullptr) {
        return false;
    }

    return state->is(VanillaBlocks::TERRACOTTA) ||
           state->is(VanillaBlocks::WHITE_TERRACOTTA) ||
           state->is(VanillaBlocks::ORANGE_TERRACOTTA) ||
           state->is(VanillaBlocks::MAGENTA_TERRACOTTA) ||
           state->is(VanillaBlocks::LIGHT_BLUE_TERRACOTTA) ||
           state->is(VanillaBlocks::YELLOW_TERRACOTTA) ||
           state->is(VanillaBlocks::LIME_TERRACOTTA) ||
           state->is(VanillaBlocks::PINK_TERRACOTTA) ||
           state->is(VanillaBlocks::GRAY_TERRACOTTA) ||
           state->is(VanillaBlocks::LIGHT_GRAY_TERRACOTTA) ||
           state->is(VanillaBlocks::CYAN_TERRACOTTA) ||
           state->is(VanillaBlocks::PURPLE_TERRACOTTA) ||
           state->is(VanillaBlocks::BLUE_TERRACOTTA) ||
           state->is(VanillaBlocks::BROWN_TERRACOTTA) ||
           state->is(VanillaBlocks::GREEN_TERRACOTTA) ||
           state->is(VanillaBlocks::RED_TERRACOTTA) ||
           state->is(VanillaBlocks::BLACK_TERRACOTTA);
}

} // namespace

// ============================================================================
// SurfaceBuilderConfig 测试
// ============================================================================

class SurfaceBuilderConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
    }
};

TEST_F(SurfaceBuilderConfigTest, DefaultValues) {
    SurfaceBuilderConfig config;
    EXPECT_EQ(config.topBlock, nullptr);
    EXPECT_EQ(config.underBlock, nullptr);
    EXPECT_EQ(config.underWaterBlock, nullptr);
}

TEST_F(SurfaceBuilderConfigTest, CustomValues) {
    SurfaceBuilderConfig config(
        &VanillaBlocks::SAND->defaultState(),
        &VanillaBlocks::SAND->defaultState(),
        &VanillaBlocks::GRAVEL->defaultState()
    );
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, GrassPreset) {
    auto config = SurfaceBuilderConfig::grass();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRASS_BLOCK));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::DIRT));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, SandPreset) {
    auto config = SurfaceBuilderConfig::sand();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::SAND));
}

TEST_F(SurfaceBuilderConfigTest, StonePreset) {
    auto config = SurfaceBuilderConfig::stone();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::STONE));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::STONE));
}

TEST_F(SurfaceBuilderConfigTest, GravelPreset) {
    auto config = SurfaceBuilderConfig::gravel();
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::GRAVEL));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::GRAVEL));
}

TEST_F(SurfaceBuilderConfigTest, RedSandPreset) {
    auto config = SurfaceBuilderConfig::redSand();
    ASSERT_NE(config.topBlock, nullptr);
    ASSERT_NE(config.underBlock, nullptr);
    ASSERT_NE(config.underWaterBlock, nullptr);
    EXPECT_TRUE(config.topBlock->is(VanillaBlocks::RED_SAND));
    EXPECT_TRUE(config.underBlock->is(VanillaBlocks::RED_SAND));
    EXPECT_TRUE(config.underWaterBlock->is(VanillaBlocks::RED_SAND));
}

// ============================================================================
// Random 类测试
// ============================================================================

class RandomTest : public ::testing::Test {
protected:
    void SetUp() override {
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<math::Random> random;
};

TEST_F(RandomTest, NextIntRange) {
    for (int i = 0; i < 100; ++i) {
        i32 val = random->nextInt(0, 9);  // [0, 9] = [0, 10)
        EXPECT_GE(val, 0);
        EXPECT_LT(val, 10);
    }
}

TEST_F(RandomTest, NextFloatRange) {
    for (int i = 0; i < 100; ++i) {
        f32 val = random->nextFloat();
        EXPECT_GE(val, 0.0f);
        EXPECT_LT(val, 1.0f);
    }
}

TEST_F(RandomTest, NextDoubleRange) {
    for (int i = 0; i < 100; ++i) {
        f64 val = random->nextDouble();
        EXPECT_GE(val, 0.0);
        EXPECT_LT(val, 1.0);
    }
}

TEST_F(RandomTest, Reproducibility) {
    random->setSeed(54321);
    i32 val1 = random->nextInt(0, 99);  // [0, 99]
    i32 val2 = random->nextInt(0, 99);
    i32 val3 = random->nextInt(0, 99);

    random->setSeed(54321);
    EXPECT_EQ(val1, random->nextInt(0, 99));
    EXPECT_EQ(val2, random->nextInt(0, 99));
    EXPECT_EQ(val3, random->nextInt(0, 99));
}

// ============================================================================
// DefaultSurfaceBuilder 测试
// ============================================================================

class DefaultSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<DefaultSurfaceBuilder>();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<DefaultSurfaceBuilder> builder;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<math::Random> random;
};

TEST_F(DefaultSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "default");
}

TEST_F(DefaultSurfaceBuilderTest, BuildSurfaceBasic) {
    // 填充区块为石头
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);

    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                chunk->setBlock(x, y, z, stone);
            }
        }
    }

    // 填充空气（地表以上）
    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    for (int y = 64; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
        for (int x = 0; x < 16; ++x) {
            for (int z = 0; z < 16; ++z) {
                chunk->setBlock(x, y, z, air);
            }
        }
    }

    // 构建地表
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    auto config = SurfaceBuilderConfig::grass();

    builder->buildSurface(
        *random,
        *chunk,
        biome,
        8, 8,           // x, z
        63,             // startHeight
        0.5,            // surfaceNoise
        stone,          // defaultBlock
        &VanillaBlocks::WATER->defaultState(),  // defaultFluid
        63,             // seaLevel
        config
    );

    // 检查地表层是否被设置为草方块
    const BlockState* topBlock = chunk->getBlock(8, 63, 8);
    ASSERT_NE(topBlock, nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::GRASS_BLOCK));

    // 检查次表层是否为泥土
    const BlockState* underBlock = chunk->getBlock(8, 62, 8);
    ASSERT_NE(underBlock, nullptr);
    EXPECT_TRUE(underBlock->is(VanillaBlocks::DIRT));
}

// 注意：calculateDepth 是 protected 方法，不能直接测试
// 我们通过 BuildSurfaceBasic 测试间接验证其功能

// ============================================================================
// MountainSurfaceBuilder 测试
// ============================================================================

class MountainSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<MountainSurfaceBuilder>();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<MountainSurfaceBuilder> builder;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<math::Random> random;
};

TEST_F(MountainSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "mountain");
}

// 注意：shouldPlaceSnow 是 private 方法，不能直接测试
// 我们通过 BuildSurface 测试间接验证其功能

// ============================================================================
// DesertSurfaceBuilder 测试
// ============================================================================

class DesertSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<DesertSurfaceBuilder>();
        chunk = std::make_unique<ChunkPrimer>(0, 0);
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<DesertSurfaceBuilder> builder;
    std::unique_ptr<ChunkPrimer> chunk;
    std::unique_ptr<math::Random> random;
};

TEST_F(DesertSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "desert");
}

TEST_F(DesertSurfaceBuilderTest, BuildSurface) {
    // 填充区块为石头
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    ASSERT_NE(stone, nullptr);

    for (int y = 0; y < 64; ++y) {
        chunk->setBlock(8, y, 8, stone);
    }

    const BlockState* air = &VanillaBlocks::AIR->defaultState();
    for (int y = 64; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
        chunk->setBlock(8, y, 8, air);
    }

    const Biome& biome = BiomeRegistry::instance().get(Biomes::Desert);
    auto config = SurfaceBuilderConfig::sand();

    builder->buildSurface(
        *random,
        *chunk,
        biome,
        8, 8,
        63,
        0.5,
        stone,
        &VanillaBlocks::WATER->defaultState(),
        63,
        config
    );

    // 沙漠地表应该使用沙子
    const BlockState* topBlock = chunk->getBlock(8, 63, 8);
    ASSERT_NE(topBlock, nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::SAND));
}

// ============================================================================
// SwampSurfaceBuilder 测试
// ============================================================================

class SwampSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<SwampSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<SwampSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(SwampSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "swamp");
}

// ============================================================================
// FrozenOceanSurfaceBuilder 测试
// ============================================================================

class FrozenOceanSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<FrozenOceanSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<FrozenOceanSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(FrozenOceanSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "frozen_ocean");
}

// ============================================================================
// BadlandsSurfaceBuilder 测试
// ============================================================================

class BadlandsSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<BadlandsSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<BadlandsSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(BadlandsSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "badlands");
}

TEST_F(BadlandsSurfaceBuilderTest, BuildSurfaceUsesRedSandAndTerracotta) {
    ChunkPrimer chunk(0, 0);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    const BlockState* air = &VanillaBlocks::AIR->defaultState();

    for (int y = 0; y < 64; ++y) {
        chunk.setBlock(8, y, 8, stone);
    }

    for (int y = 64; y < mc::world::MAX_BUILD_HEIGHT; ++y) {
        chunk.setBlock(8, y, 8, air);
    }

    const Biome& biome = BiomeRegistry::instance().get(Biomes::Badlands);
    auto config = SurfaceBuilderConfig::redSand();

    builder->buildSurface(
        *random,
        chunk,
        biome,
        8,
        8,
        63,
        0.5f,
        stone,
        &VanillaBlocks::WATER->defaultState(),
        63,
        config
    );

    const BlockState* topBlock = chunk.getBlock(8, 63, 8);
    ASSERT_NE(topBlock, nullptr);
    EXPECT_TRUE(topBlock->is(VanillaBlocks::RED_SAND));

    const BlockState* underBlock = chunk.getBlock(8, 62, 8);
    ASSERT_NE(underBlock, nullptr);
    EXPECT_TRUE(isTerracottaState(underBlock));
}

// 注意：BadlandsSurfaceBuilder 不再提供 getTerracottaLayer 方法
// 陶瓦层生成逻辑已整合到 buildSurface 内部

// ============================================================================
// BeachSurfaceBuilder 测试
// ============================================================================

class BeachSurfaceBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        builder = std::make_unique<BeachSurfaceBuilder>();
        random = std::make_unique<math::Random>(12345);
    }

    std::unique_ptr<BeachSurfaceBuilder> builder;
    std::unique_ptr<math::Random> random;
};

TEST_F(BeachSurfaceBuilderTest, Name) {
    EXPECT_STREQ(builder->name(), "beach");
}

// ============================================================================
// SurfaceBuilder 多态性测试
// ============================================================================

class SurfaceBuilderPolymorphismTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
    }
};

TEST_F(SurfaceBuilderPolymorphismTest, AllBuildersAreValid) {
    std::vector<std::unique_ptr<SurfaceBuilder>> builders;
    builders.push_back(std::make_unique<DefaultSurfaceBuilder>());
    builders.push_back(std::make_unique<MountainSurfaceBuilder>());
    builders.push_back(std::make_unique<DesertSurfaceBuilder>());
    builders.push_back(std::make_unique<SwampSurfaceBuilder>());
    builders.push_back(std::make_unique<FrozenOceanSurfaceBuilder>());
    builders.push_back(std::make_unique<BadlandsSurfaceBuilder>());
    builders.push_back(std::make_unique<BeachSurfaceBuilder>());

    for (const auto& builder : builders) {
        EXPECT_NE(builder->name(), nullptr);
        EXPECT_GT(strlen(builder->name()), 0u);
    }
}

TEST_F(SurfaceBuilderPolymorphismTest, BuildSurfaceWithDifferentBuilders) {
    ChunkPrimer chunk(0, 0);
    math::Random random(12345);
    const Biome& biome = BiomeRegistry::instance().get(Biomes::Plains);
    const BlockState* stone = &VanillaBlocks::STONE->defaultState();
    auto config = SurfaceBuilderConfig::grass();

    // 填充区块
    for (int y = 0; y < 64; ++y) {
        chunk.setBlock(0, y, 0, stone);
    }

    std::vector<std::unique_ptr<SurfaceBuilder>> builders;
    builders.push_back(std::make_unique<DefaultSurfaceBuilder>());
    builders.push_back(std::make_unique<MountainSurfaceBuilder>());
    builders.push_back(std::make_unique<DesertSurfaceBuilder>());

    // 每个构建器都应该能成功构建地表
    for (auto& builder : builders) {
        // 重新填充区块
        for (int y = 0; y < 64; ++y) {
            chunk.setBlock(0, y, 0, stone);
        }

        // 不应该抛出异常
        EXPECT_NO_THROW(
            builder->buildSurface(
                random,
                chunk,
                biome,
                0, 0,
                63,
                0.5,
                stone,
                &VanillaBlocks::WATER->defaultState(),
                63,
                config
            )
        );
    }
}
