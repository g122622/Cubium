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
// IChunkGenerator::clearStructureCache() 单元测试
//
// 测试覆盖：
// 1. IChunkGenerator 基类默认实现（空操作，不崩溃）
// 2. NoiseChunkGenerator::clearStructureCache() 正确清空缓存
// 3. FlatChunkGenerator::clearStructureCache() 正确清空缓存
// 4. 重复调用 clearStructureCache() 安全
// 5. clearStructureCache() 后 StructureCheck 可继续正常使用
// 6. IChunkGenerator 多态调用
// 7. 析构前显式清理的场景（模拟 ServerDimension::shutdown()）
// ============================================================================

#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/biome/source/FixedBiomeSource.hpp"
#include "common/world/biome/source/MultiNoiseBiomeSource.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/gen/chunk/DebugChunkGenerator.hpp"
#include "common/world/gen/chunk/FlatChunkGenerator.hpp"
#include "common/world/gen/chunk/NoiseChunkGenerator.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::world::gen::structure;

namespace {

// ============================================================================
// 辅助函数
// ============================================================================

/// 将区块坐标打包为 64 位 ID，复用项目中的 chunkPosToId
static u64 packChunkPos(i32 chunkX, i32 chunkZ)
{
    return mc::math::chunkPosToId(chunkX, chunkZ);
}

// ============================================================================
// 测试夹具
// ============================================================================

class ClearStructureCacheTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        VanillaBlocks::initialize();
        BiomeRegistry::instance().initialize();
        fluid::FluidRegistry::instance().initialize();
    }
};

// ============================================================================
// 1. IChunkGenerator 基类默认实现
// ============================================================================

TEST_F(ClearStructureCacheTest, BaseClass_DefaultImplementation_DoesNotCrash)
{
    // DebugChunkGenerator 不重写 clearStructureCache()，使用基类默认空操作
    // 调用应不崩溃、不抛异常
    DebugChunkGenerator generator;
    IChunkGenerator& base = generator;

    // 基类默认实现是空操作，调用不应崩溃
    base.clearStructureCache();
    SUCCEED();
}

TEST_F(ClearStructureCacheTest, BaseClass_DefaultStructureCheck_ReturnsNullptr)
{
    // DebugChunkGenerator 不重写 structureCheck()，返回 nullptr
    DebugChunkGenerator generator;
    IChunkGenerator& base = generator;

    EXPECT_EQ(base.structureCheck(), nullptr);
}

// ============================================================================
// 2. NoiseChunkGenerator::clearStructureCache()
// ============================================================================

TEST_F(ClearStructureCacheTest, NoiseGenerator_ClearStructureCache_EmptiesCache)
{
    // 创建 NoiseChunkGenerator 并填充结构缓存，然后验证 clearStructureCache() 清空缓存
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    NoiseChunkGenerator gen(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    // 获取 StructureCheck 指针
    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    // 填充缓存数据
    ResourceLocation villageId("minecraft:village_plains");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    const u64 chunkPos = packChunkPos(10, 20);
    check->onStructureLoad(chunkPos, refCounts);

    // 验证缓存非空
    EXPECT_GT(check->loadedChunkCount(), 0u);

    // 调用 clearStructureCache()
    gen.clearStructureCache();

    // 验证缓存已被清空
    EXPECT_EQ(check->loadedChunkCount(), 0u);

    // 验证查询返回 ChunkLoadNeeded（缓存未命中）
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check->checkStart(chunkPos, villageId));
}

TEST_F(ClearStructureCacheTest, NoiseGenerator_ClearStructureCache_ClearsFeatureChecks)
{
    // 验证 clearStructureCache() 同时清空精确缓存和近似缓存
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    NoiseChunkGenerator gen(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    ResourceLocation fortressId("minecraft:fortress");

    // 填充近似缓存
    const u64 chunkPos1 = packChunkPos(1, 1);
    const u64 chunkPos2 = packChunkPos(2, 2);
    check->setFeatureCheckResult(chunkPos2, false);

    // 验证近似缓存生效：setFeatureCheckResult(false) 返回 StartNotPresent
    EXPECT_EQ(StructureCheckResult::StartNotPresent, check->checkStart(chunkPos2, fortressId));

    // 调用 clearStructureCache()
    gen.clearStructureCache();

    // 验证近似缓存也被清空：查询应返回 ChunkLoadNeeded
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check->checkStart(chunkPos2, fortressId));
}

TEST_F(ClearStructureCacheTest, NoiseGenerator_ClearStructureCache_ReloadAfterClear)
{
    // 验证清空缓存后可正常重新加载数据
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    NoiseChunkGenerator gen(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    ResourceLocation villageId("minecraft:village_plains");
    const u64 chunkPos = packChunkPos(5, 5);

    // 加载、清空、重新加载
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check->onStructureLoad(chunkPos, refCounts);
    gen.clearStructureCache();
    EXPECT_EQ(0u, check->loadedChunkCount());

    // 重新加载
    refCounts[villageId] = 3;
    check->onStructureLoad(chunkPos, refCounts);
    EXPECT_EQ(1u, check->loadedChunkCount());
    EXPECT_EQ(StructureCheckResult::StartPresent, check->checkStart(chunkPos, villageId));
}

TEST_F(ClearStructureCacheTest, NoiseGenerator_ClearStructureCache_MultipleCallsSafe)
{
    // 验证重复调用 clearStructureCache() 安全
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    NoiseChunkGenerator gen(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    // 填充缓存
    ResourceLocation villageId("minecraft:village_plains");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check->onStructureLoad(packChunkPos(1, 1), refCounts);

    // 多次调用不崩溃
    gen.clearStructureCache();
    gen.clearStructureCache();
    gen.clearStructureCache();

    EXPECT_EQ(0u, check->loadedChunkCount());
}

// ============================================================================
// 3. FlatChunkGenerator::clearStructureCache()
// ============================================================================

TEST_F(ClearStructureCacheTest, FlatGenerator_ClearStructureCache_EmptiesCache)
{
    // 创建 FlatChunkGenerator 并验证 clearStructureCache() 清空缓存
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());

    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    // 填充缓存数据
    ResourceLocation villageId("minecraft:village_plains");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    const u64 chunkPos = packChunkPos(3, 3);
    check->onStructureLoad(chunkPos, refCounts);

    EXPECT_GT(check->loadedChunkCount(), 0u);

    // 调用 clearStructureCache()
    gen.clearStructureCache();

    EXPECT_EQ(check->loadedChunkCount(), 0u);
    EXPECT_EQ(StructureCheckResult::ChunkLoadNeeded, check->checkStart(chunkPos, villageId));
}

TEST_F(ClearStructureCacheTest, FlatGenerator_ClearStructureCache_MultipleCallsSafe)
{
    FlatChunkGenerator gen(12345LL, FlatLevelGeneratorSettings::createDefault());

    StructureCheck* check = gen.structureCheck();
    ASSERT_NE(check, nullptr);

    // 填充缓存
    ResourceLocation fortressId("minecraft:fortress");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[fortressId] = 2;
    check->onStructureLoad(packChunkPos(10, 10), refCounts);

    // 多次调用不崩溃
    gen.clearStructureCache();
    gen.clearStructureCache();

    EXPECT_EQ(0u, check->loadedChunkCount());
}

// ============================================================================
// 4. IChunkGenerator 多态调用
// ============================================================================

TEST_F(ClearStructureCacheTest, PolymorphicCall_NoiseGenerator)
{
    // 通过基类指针调用 clearStructureCache()，验证虚函数分派正确
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    auto gen = std::make_unique<NoiseChunkGenerator>(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    StructureCheck* check = gen->structureCheck();
    ASSERT_NE(check, nullptr);

    ResourceLocation villageId("minecraft:village_plains");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check->onStructureLoad(packChunkPos(7, 7), refCounts);
    EXPECT_GT(check->loadedChunkCount(), 0u);

    // 通过基类指针调用
    IChunkGenerator* base = gen.get();
    base->clearStructureCache();

    EXPECT_EQ(0u, check->loadedChunkCount());
}

TEST_F(ClearStructureCacheTest, PolymorphicCall_FlatGenerator)
{
    // 通过基类指针调用 clearStructureCache()，验证虚函数分派正确
    auto gen = std::make_unique<FlatChunkGenerator>(12345LL, FlatLevelGeneratorSettings::createDefault());

    StructureCheck* check = gen->structureCheck();
    ASSERT_NE(check, nullptr);

    ResourceLocation strongholdId("minecraft:stronghold");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[strongholdId] = 1;
    check->onStructureLoad(packChunkPos(0, 0), refCounts);
    EXPECT_GT(check->loadedChunkCount(), 0u);

    // 通过基类指针调用
    IChunkGenerator* base = gen.get();
    base->clearStructureCache();

    EXPECT_EQ(0u, check->loadedChunkCount());
}

TEST_F(ClearStructureCacheTest, PolymorphicCall_DebugGenerator_NoOp)
{
    // DebugChunkGenerator 不重写 clearStructureCache()，基类默认空操作
    auto gen = std::make_unique<DebugChunkGenerator>();
    IChunkGenerator* base = gen.get();

    // 多次调用不应崩溃
    base->clearStructureCache();
    base->clearStructureCache();
    SUCCEED();
}

// ============================================================================
// 5. 析构前显式清理的场景（模拟 ServerDimension::shutdown()）
// ============================================================================

TEST_F(ClearStructureCacheTest, NoiseGenerator_ExplicitClearBeforeDestruction)
{
    // 模拟 ServerDimension::shutdown() 的行为模式：
    // 先显式调用 clearStructureCache()，再销毁生成器
    auto biomeSource = world::biome::source::MultiNoiseBiomeSource::createOverworld(12345ULL, false);
    auto gen = std::make_unique<NoiseChunkGenerator>(12345ULL, DimensionSettings::overworld(), std::move(biomeSource));

    StructureCheck* check = gen->structureCheck();
    ASSERT_NE(check, nullptr);

    // 填充缓存
    ResourceLocation villageId("minecraft:village_plains");
    std::unordered_map<ResourceLocation, i32> refCounts;
    refCounts[villageId] = 0;
    check->onStructureLoad(packChunkPos(1, 1), refCounts);
    EXPECT_GT(check->loadedChunkCount(), 0u);

    // 模拟 ServerDimension::shutdown()：先显式清理
    gen->clearStructureCache();
    EXPECT_EQ(0u, check->loadedChunkCount());

    // 再销毁生成器（析构函数为 = default，不崩溃）
    gen.reset();
    SUCCEED();
}

} // namespace
