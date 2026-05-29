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

#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include <cmath>
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief 调试区块生成器测试夹具
 *
 * 确保 VanillaBlocks 已初始化
 */
class DebugChunkGeneratorTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // 初始化所有方块
        VanillaBlocks::initialize();
    }

    static void TearDownTestSuite()
    {
        // 清理（如果需要）
    }
};

// 测试生成器创建
TEST_F(DebugChunkGeneratorTest, CreateGenerator)
{
    DebugChunkGenerator generator;

    EXPECT_EQ(generator.seed(), 0);
    EXPECT_EQ(generator.seaLevel(), 0);
    EXPECT_EQ(generator.getGroundHeight(), 70);
}

// 测试初始化所有方块状态
TEST_F(DebugChunkGeneratorTest, InitializeValidStates)
{
    // 初始化之前状态列表可能为空
    DebugChunkGenerator::initializeValidStates();

    EXPECT_TRUE(DebugChunkGenerator::isInitialized());
    EXPECT_GT(DebugChunkGenerator::getGridWidth(), 0);
    EXPECT_GT(DebugChunkGenerator::getGridHeight(), 0);

    const auto& states = DebugChunkGenerator::getAllValidStates();
    EXPECT_FALSE(states.empty());

    // 状态数量应该大于方块数量
    auto& registry = BlockRegistry::instance();
    EXPECT_GT(states.size(), registry.blockCount());
}

// 测试方块位置映射
TEST_F(DebugChunkGeneratorTest, GetBlockStateFor)
{
    DebugChunkGenerator::initializeValidStates();

    // 原点应该返回空气
    const BlockState* state = DebugChunkGenerator::getBlockStateFor(0, 0);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());

    // 负坐标应该返回空气
    state = DebugChunkGenerator::getBlockStateFor(-1, -1);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());

    // 偶数坐标应该返回空气（因为只在奇数坐标放置方块）
    state = DebugChunkGenerator::getBlockStateFor(2, 2);
    ASSERT_NE(state, nullptr);
    EXPECT_TRUE(state->isAir());

    // 奇数坐标应该返回有效方块状态
    state = DebugChunkGenerator::getBlockStateFor(1, 1);
    ASSERT_NE(state, nullptr);
    // 第一个方块不应该是空气（除非空气是第一个注册的状态）

    // 另一个奇数坐标
    state = DebugChunkGenerator::getBlockStateFor(3, 1);
    ASSERT_NE(state, nullptr);
}

// 测试方块网格索引计算
TEST_F(DebugChunkGeneratorTest, GridIndexCalculation)
{
    DebugChunkGenerator::initializeValidStates();

    i32 gridWidth = DebugChunkGenerator::getGridWidth();
    const auto& states = DebugChunkGenerator::getAllValidStates();

    // 测试多个位置的方块状态
    for (i32 gx = 0; gx < std::min(gridWidth, 5); ++gx) {
        for (i32 gz = 0; gz < std::min(DebugChunkGenerator::getGridHeight(), 5); ++gz) {
            i32 worldX = gx * 2 + 1; // 转换为世界坐标
            i32 worldZ = gz * 2 + 1;

            const BlockState* state = DebugChunkGenerator::getBlockStateFor(worldX, worldZ);
            ASSERT_NE(state, nullptr) << "State should not be null at (" << worldX << ", " << worldZ << ")";

            // 计算预期索引
            i32 expectedIndex = std::abs(gx * gridWidth + gz);
            if (expectedIndex < static_cast<i32>(states.size())) {
                EXPECT_EQ(state, states[expectedIndex]) << "State mismatch at grid (" << gx << ", " << gz << ")";
            }
        }
    }
}

// 测试生物群系始终返回平原
TEST_F(DebugChunkGeneratorTest, BiomeAlwaysPlains)
{
    DebugChunkGenerator generator;

    EXPECT_EQ(generator.getBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(generator.getBiome(100, 50, 100), Biomes::Plains);
    EXPECT_EQ(generator.getBiome(-100, 0, -100), Biomes::Plains);

    EXPECT_EQ(generator.getNoiseBiome(0, 0, 0), Biomes::Plains);
    EXPECT_EQ(generator.getNoiseBiome(100, 50, 100), Biomes::Plains);
}

// 测试高度返回
TEST_F(DebugChunkGeneratorTest, GetHeight)
{
    DebugChunkGenerator generator;
    DebugChunkGenerator::initializeValidStates();

    // 高度应该返回 60 或 70
    i32 height = generator.getHeight(0, 0, HeightmapType::WorldSurface);
    EXPECT_GE(height, 60);
    EXPECT_LE(height, 70);

    // 奇数坐标应该有方块，高度为 70
    height = generator.getHeight(1, 1, HeightmapType::WorldSurface);
    EXPECT_EQ(height, 70);
}

// 测试屏障方块基座层
TEST_F(DebugChunkGeneratorTest, BarrierBaseLayer)
{
    // 初始化
    DebugChunkGenerator::initializeValidStates();

    // Y=60 应该是屏障层
    // 这个测试需要实际的区块生成来验证
    // 这里我们只验证生成器可以创建
    DebugChunkGenerator generator;
    EXPECT_NO_THROW(generator.getHeight(0, 0, HeightmapType::WorldSurface));
}

// 测试空操作方法
TEST_F(DebugChunkGeneratorTest, NoOpMethods)
{
    DebugChunkGenerator generator;

    // 这些方法应该是空操作，不应该崩溃
    EXPECT_NO_THROW(
        generator.generateStructureStarts(*static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr)));
    EXPECT_NO_THROW(generator.generateStructureReferences(
        *static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr)));
    EXPECT_NO_THROW(
        generator.buildSurface(*static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr)));
    EXPECT_NO_THROW(
        generator.applyCarvers(*static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr), true));
    EXPECT_NO_THROW(
        generator.placeFeatures(*static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr)));

    std::vector<SpawnedEntityData> entities;
    EXPECT_EQ(generator.spawnInitialMobs(
                  *static_cast<WorldGenRegion*>(nullptr), *static_cast<ChunkPrimer*>(nullptr), entities),
        0);
}

// 测试网格尺寸一致性
TEST_F(DebugChunkGeneratorTest, GridSizeConsistency)
{
    DebugChunkGenerator::initializeValidStates();

    i32 gridWidth = DebugChunkGenerator::getGridWidth();
    i32 gridHeight = DebugChunkGenerator::getGridHeight();
    const auto& states = DebugChunkGenerator::getAllValidStates();

    // 网格应该能容纳所有状态
    i32 gridSize = gridWidth * gridHeight;
    EXPECT_GE(gridSize, static_cast<i32>(states.size()));
}

// 测试 WorldConfig 枚举
TEST(DebugWorldConfigTest, WorldTypeEnum)
{
    EXPECT_EQ(worldTypeName(WorldType::Default), "default");
    EXPECT_EQ(worldTypeName(WorldType::Flat), "flat");
    EXPECT_EQ(worldTypeName(WorldType::LargeBiomes), "largeBiomes");
    EXPECT_EQ(worldTypeName(WorldType::Amplified), "amplified");
    EXPECT_EQ(worldTypeName(WorldType::Debug), "debug_all_block_states");
}

TEST(DebugWorldConfigTest, ParseWorldType)
{
    EXPECT_EQ(parseWorldType("flat"), WorldType::Flat);
    EXPECT_EQ(parseWorldType("largeBiomes"), WorldType::LargeBiomes);
    EXPECT_EQ(parseWorldType("large_biomes"), WorldType::LargeBiomes);
    EXPECT_EQ(parseWorldType("amplified"), WorldType::Amplified);
    EXPECT_EQ(parseWorldType("debug"), WorldType::Debug);
    EXPECT_EQ(parseWorldType("debug_all_block_states"), WorldType::Debug);
    EXPECT_EQ(parseWorldType("unknown"), WorldType::Default);
    EXPECT_EQ(parseWorldType("default"), WorldType::Default);
}

TEST(DebugWorldConfigTest, WorldConfigDebugCheck)
{
    WorldConfig config;
    EXPECT_FALSE(config.isDebugWorld()); // 默认不是调试模式

    config.worldType = WorldType::Debug;
    EXPECT_TRUE(config.isDebugWorld());

    config.worldType = WorldType::Default;
    EXPECT_FALSE(config.isDebugWorld());
}

// ============================================================================
// isDebugGenerator() 测试
// ============================================================================

TEST_F(DebugChunkGeneratorTest, IsDebugGenerator_ReturnsTrue)
{
    DebugChunkGenerator generator;
    EXPECT_TRUE(generator.isDebugGenerator());
}

TEST_F(DebugChunkGeneratorTest, IsDebugGenerator_VirtualDispatch)
{
    // 通过基类指针调用，验证虚函数分派
    DebugChunkGenerator debugGen;
    IChunkGenerator* basePtr = &debugGen;
    EXPECT_TRUE(basePtr->isDebugGenerator());
}

// ============================================================================
// NoiseChunkGenerator::isDebugGenerator() 测试
// ============================================================================

TEST(NoiseChunkGeneratorIsDebugTest, IsDebugGenerator_ReturnsFalse)
{
    NoiseChunkGenerator generator(12345ULL, DimensionSettings::overworld());
    EXPECT_FALSE(generator.isDebugGenerator());
}

TEST(NoiseChunkGeneratorIsDebugTest, IsDebugGenerator_VirtualDispatch)
{
    // 通过基类指针调用，验证虚函数分派
    NoiseChunkGenerator noiseGen(12345ULL, DimensionSettings::overworld());
    IChunkGenerator* basePtr = &noiseGen;
    EXPECT_FALSE(basePtr->isDebugGenerator());
}

TEST(NoiseChunkGeneratorIsDebugTest, IsDebugGenerator_FlatSettings)
{
    // 使用 flat 设置也应该返回 false
    NoiseChunkGenerator generator(12345ULL, DimensionSettings::flat());
    EXPECT_FALSE(generator.isDebugGenerator());
}

TEST(NoiseChunkGeneratorIsDebugTest, IsDebugGenerator_AmplifiedSettings)
{
    // 使用 amplified 设置也应该返回 false
    NoiseChunkGenerator generator(12345ULL, DimensionSettings::amplified());
    EXPECT_FALSE(generator.isDebugGenerator());
}

} // namespace
} // namespace mc
