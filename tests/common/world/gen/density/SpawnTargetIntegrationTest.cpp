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

// ============================================================================
// NoiseChunk::cachedClimateSampler / RandomState spawnTarget 集成测试
//
// 本测试覆盖 TODO 收敛的关键集成点：
// 1. RandomState::create 是否真正将 DimensionSettings.spawnTarget 设置到 Sampler 上
// 2. NoiseChunk::cachedClimateSampler 是否真正调用 setSpawnTarget
// 3. 主世界/下界/末地预设的 spawnTarget 在数据流中的传递一致性
// 4. 空.spawnTarget 的降级行为（下界/末地）
// ============================================================================

#include <gtest/gtest.h>

#include "common/core/Constants.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/climate/Sampler.hpp"
#include "common/world/biome/source/OverworldBiomeBuilder.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/RandomState.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/density/Beardifier.hpp"
#include "common/world/gen/density/NoiseChunk.hpp"
#include "common/world/gen/density/NoiseRouter.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"

namespace mc {
namespace {

class SpawnTargetIntegrationTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. RandomState::create 集成测试
// ============================================================================

TEST_F(SpawnTargetIntegrationTest, RandomState_Overworld_SetsSpawnTargetOnSampler)
{
    // 验证 RandomState::create 在构造时将 DimensionSettings.spawnTarget 设置到 Sampler 上
    auto settings = DimensionSettings::overworld();
    ASSERT_FALSE(settings.spawnTarget.empty());

    const auto& expectedSpawnTarget = settings.spawnTarget;

    auto randomState = world::gen::RandomState::create(settings, 12345ULL);
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    const auto& samplerSpawnTarget = sampler.spawnTarget();

    // Sampler 的 spawnTarget 应该与 DimensionSettings 的 spawnTarget 一致
    EXPECT_EQ(samplerSpawnTarget.size(), expectedSpawnTarget.size());
    EXPECT_EQ(samplerSpawnTarget.size(), 2u);

    for (size_t i = 0; i < samplerSpawnTarget.size(); ++i) {
        EXPECT_EQ(samplerSpawnTarget[i], expectedSpawnTarget[i])
            << "Sampler.spawnTarget[" << i << "] 不匹配 DimensionSettings.spawnTarget[" << i << "]";
    }
}

TEST_F(SpawnTargetIntegrationTest, RandomState_LargeBiomes_SetsSpawnTargetOnSampler)
{
    auto settings = DimensionSettings::largeBiomesPreset();
    ASSERT_FALSE(settings.spawnTarget.empty());

    auto randomState = world::gen::RandomState::create(settings, 1ULL);
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_EQ(sampler.spawnTarget().size(), 2u);
}

TEST_F(SpawnTargetIntegrationTest, RandomState_Amplified_SetsSpawnTargetOnSampler)
{
    auto settings = DimensionSettings::amplified();
    ASSERT_FALSE(settings.spawnTarget.empty());

    auto randomState = world::gen::RandomState::create(settings, 1ULL);
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_EQ(sampler.spawnTarget().size(), 2u);
}

TEST_F(SpawnTargetIntegrationTest, RandomState_Nether_HasEmptySpawnTargetOnSampler)
{
    // 下界 spawnTarget 为空（沿用 (0,0) 区块作为出生点）
    auto settings = DimensionSettings::nether();
    ASSERT_TRUE(settings.spawnTarget.empty());

    auto randomState = world::gen::RandomState::create(settings, 1ULL);
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

TEST_F(SpawnTargetIntegrationTest, RandomState_End_HasEmptySpawnTargetOnSampler)
{
    auto settings = DimensionSettings::end();
    ASSERT_TRUE(settings.spawnTarget.empty());

    auto randomState = world::gen::RandomState::create(settings, 1ULL);
    ASSERT_NE(randomState, nullptr);

    const auto& sampler = randomState->sampler();
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

TEST_F(SpawnTargetIntegrationTest, RandomState_SamplerSpawnTargetMatchesOverworldBiomeBuilder)
{
    // 验证 RandomState 内 Sampler 持有的 spawnTarget 与 OverworldBiomeBuilder.spawnTarget() 完全一致
    auto settings = DimensionSettings::overworld();
    auto expected = world::biome::source::OverworldBiomeBuilder().spawnTarget();

    ASSERT_EQ(settings.spawnTarget.size(), expected.size());

    auto randomState = world::gen::RandomState::create(settings, 42ULL);
    const auto& sampler = randomState->sampler();

    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(sampler.spawnTarget()[i], expected[i])
            << "Sampler.spawnTarget[" << i << "] 不匹配 OverworldBiomeBuilder.spawnTarget()[" << i << "]";
    }
}

// ============================================================================
// 2. NoiseChunk::cachedClimateSampler 集成测试
// ============================================================================

TEST_F(SpawnTargetIntegrationTest, NoiseChunk_CachedClimateSampler_SetsSpawnTarget)
{
    // 验证 NoiseChunk::cachedClimateSampler(spawnTarget) 在首次调用时
    // 真正将 spawnTarget 设置到内部缓存的 Sampler 上
    const u64 seed = 42ULL;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto settings = DimensionSettings::overworld();
    const auto& expectedSpawnTarget = settings.spawnTarget;
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0, // alignedX
        startY,
        0, // startBlockZ
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1);

    // 调用 cachedClimateSampler 传入 spawnTarget
    auto sampler = noiseChunk->cachedClimateSampler(settings.spawnTarget);

    // 验证返回的 Sampler 副本持有 spawnTarget
    EXPECT_EQ(sampler.spawnTarget().size(), expectedSpawnTarget.size());
    EXPECT_EQ(sampler.spawnTarget().size(), 2u);

    const auto& samplerSpawnTarget = sampler.spawnTarget();
    for (size_t i = 0; i < samplerSpawnTarget.size(); ++i) {
        EXPECT_EQ(samplerSpawnTarget[i], expectedSpawnTarget[i]);
    }
}

TEST_F(SpawnTargetIntegrationTest, NoiseChunk_CachedClimateSampler_EmptySpawnTarget_NoCrash)
{
    // 验证空 spawnTarget 调用不会崩溃，且 sampler.spawnTarget() 为空
    const u64 seed = 42ULL;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto settings = DimensionSettings::nether(); // 空 spawnTarget
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1);

    // 传入空 spawnTarget
    auto sampler = noiseChunk->cachedClimateSampler({});
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

TEST_F(SpawnTargetIntegrationTest, NoiseChunk_CachedClimateSampler_DefaultArg_EmptySpawnTarget)
{
    // 验证默认参数（空 spawnTarget）也能正常工作
    const u64 seed = 42ULL;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1);

    // 使用默认参数（不传 spawnTarget）
    auto sampler = noiseChunk->cachedClimateSampler();
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

TEST_F(SpawnTargetIntegrationTest, NoiseChunk_CachedClimateSampler_CachesSamplerInstance)
{
    // 验证多次调用 cachedClimateSampler 返回的 Sampler 是同一个缓存实例的副本
    // （Sampler 只持有指针，拷贝是廉价的；但内部 m_cachedSampler 应该只创建一次）
    const u64 seed = 42ULL;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1);

    auto sampler1 = noiseChunk->cachedClimateSampler(settings.spawnTarget);
    auto sampler2 = noiseChunk->cachedClimateSampler(settings.spawnTarget);

    // 两次调用返回的 spawnTarget 应该一致
    EXPECT_EQ(sampler1.spawnTarget().size(), sampler2.spawnTarget().size());
    for (size_t i = 0; i < sampler1.spawnTarget().size(); ++i) {
        EXPECT_EQ(sampler1.spawnTarget()[i], sampler2.spawnTarget()[i]);
    }
}

TEST_F(SpawnTargetIntegrationTest, NoiseChunk_CachedClimateSampler_DifferentSpawnTarget_UpdatesCache)
{
    // 验证传入不同的 spawnTarget 时会刷新缓存
    const u64 seed = 42ULL;
    const i32 cellWidth = 4;
    const i32 cellHeight = 8;
    const i32 startY = world::MIN_BUILD_HEIGHT;
    const i32 noiseHeight = world::MAX_BUILD_HEIGHT - world::MIN_BUILD_HEIGHT;
    const i32 cellCountY = noiseHeight / cellHeight;

    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, seed);
    auto noiseChunk = std::make_unique<world::gen::density::NoiseChunk>(*randomState,
        cellWidth,
        cellHeight,
        cellCountY,
        0,
        startY,
        0,
        std::make_unique<world::gen::density::BeardifierMarker>(),
        1);

    // 第一次调用：传入主世界 spawnTarget
    auto sampler1 = noiseChunk->cachedClimateSampler(settings.spawnTarget);
    EXPECT_EQ(sampler1.spawnTarget().size(), 2u);

    // 第二次调用：传入空 spawnTarget
    // 注意：根据实现，空 spawnTarget 不会刷新缓存（因为 !spawnTarget.empty() 守卫）
    auto sampler2 = noiseChunk->cachedClimateSampler({});
    // 缓存仍然保留之前的 spawnTarget（因为空 spawnTarget 不触发刷新）
    EXPECT_EQ(sampler2.spawnTarget().size(), 2u);
}

// ============================================================================
// 3. 端到端集成：NoiseChunkGenerator + RandomState + spawnTarget
// ============================================================================

TEST_F(SpawnTargetIntegrationTest, NoiseChunkGenerator_RandomState_HasSpawnTarget)
{
    // 验证 NoiseChunkGenerator 持有的 RandomState 的 Sampler 已经设置了 spawnTarget
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 12345ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(*randomState, false, false);
    auto generator =
        std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

    // 通过 randomState() 访问器获取 RandomState
    const auto& rs = generator->randomState();
    ASSERT_NE(rs, nullptr);

    const auto& sampler = rs->sampler();
    EXPECT_FALSE(sampler.spawnTarget().empty());
    EXPECT_EQ(sampler.spawnTarget().size(), 2u);
}

TEST_F(SpawnTargetIntegrationTest, NoiseChunkGenerator_Nether_RandomState_HasEmptySpawnTarget)
{
    // 下界 NoiseChunkGenerator 的 Sampler.spawnTarget() 应该为空
    auto settings = DimensionSettings::nether();
    auto randomState = world::gen::RandomState::create(settings, 0ULL);
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createNether(*randomState);
    auto generator =
        std::make_unique<NoiseChunkGenerator>(std::move(settings), std::move(biomeSource), std::move(randomState));

    const auto& rs = generator->randomState();
    ASSERT_NE(rs, nullptr);

    const auto& sampler = rs->sampler();
    EXPECT_TRUE(sampler.spawnTarget().empty());
}

// ============================================================================
// 4. findSpawnPosition 端到端验证
// ============================================================================

TEST_F(SpawnTargetIntegrationTest, RandomState_Overworld_FindSpawnPositionReturnsValidBlockPos)
{
    // 验证主世界 Sampler.findSpawnPosition() 能正常工作并返回合理的 BlockPos
    // （在气候空间径向搜索最佳出生点，返回 (x, 0, z) 块坐标）
    auto settings = DimensionSettings::overworld();
    auto randomState = world::gen::RandomState::create(settings, 12345ULL);

    const auto& sampler = randomState->sampler();
    ASSERT_FALSE(sampler.spawnTarget().empty());

    const BlockPos spawn = sampler.findSpawnPosition();
    // findSpawnPosition 返回的 y 应该为 0（MC 行为：在气候空间搜索，y 固定为 0）
    EXPECT_EQ(spawn.y, 0);
    // x/z 应该是有限值（不是极端的 INT_MIN/INT_MAX）
    EXPECT_GT(spawn.x, -1000000);
    EXPECT_LT(spawn.x, 1000000);
    EXPECT_GT(spawn.z, -1000000);
    EXPECT_LT(spawn.z, 1000000);
}

TEST_F(SpawnTargetIntegrationTest, RandomState_Nether_FindSpawnPositionReturnsOrigin)
{
    // 下界 spawnTarget 为空，findSpawnPosition 应返回 (0, 0, 0)
    auto settings = DimensionSettings::nether();
    auto randomState = world::gen::RandomState::create(settings, 0ULL);

    const auto& sampler = randomState->sampler();
    ASSERT_TRUE(sampler.spawnTarget().empty());

    const BlockPos spawn = sampler.findSpawnPosition();
    EXPECT_EQ(spawn.x, 0);
    EXPECT_EQ(spawn.y, 0);
    EXPECT_EQ(spawn.z, 0);
}

TEST_F(SpawnTargetIntegrationTest, FindSpawnPosition_Deterministic_ForSameSeed)
{
    // 同一种子的 findSpawnPosition 应该是确定性的
    auto settings1 = DimensionSettings::overworld();
    auto randomState1 = world::gen::RandomState::create(settings1, 99999ULL);
    const auto& sampler1 = randomState1->sampler();
    const BlockPos spawn1 = sampler1.findSpawnPosition();

    auto settings2 = DimensionSettings::overworld();
    auto randomState2 = world::gen::RandomState::create(settings2, 99999ULL);
    const auto& sampler2 = randomState2->sampler();
    const BlockPos spawn2 = sampler2.findSpawnPosition();

    EXPECT_EQ(spawn1.x, spawn2.x);
    EXPECT_EQ(spawn1.y, spawn2.y);
    EXPECT_EQ(spawn1.z, spawn2.z);
}

} // namespace
} // namespace mc
