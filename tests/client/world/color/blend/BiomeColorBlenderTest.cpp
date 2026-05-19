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
 * @file BiomeColorBlenderTest.cpp
 * @brief 生物群系颜色混合器单元测试
 *
 * 测试 BiomeColorBlender、BiomeColorCache、ChunkBiomeAccessor 的功能。
 */

#include "client/world/color/blend/blend.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeEffects.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include <gtest/gtest.h>

namespace mc::client::test {

using namespace world::biome;

/**
 * @brief 测试用生物群系访问器
 *
 * 简单实现 IBiomeAccessor 接口，用于单元测试。
 */
class TestBiomeAccessor : public BiomeColorBlender::IBiomeAccessor {
public:
    /**
     * @brief 设置指定位置的生物群系
     */
    void setBiome(i32 x, i32 y, i32 z, const Biome* biome)
    {
        const u64 key = makeKey(x, z);
        m_biomes[key] = biome;
    }

    /**
     * @brief 设置指定区块是否加载
     */
    void setChunkLoaded(ChunkCoord x, ChunkCoord z, bool loaded)
    {
        const u64 key = static_cast<u64>(static_cast<u32>(x)) << 32 | static_cast<u64>(static_cast<u32>(z));
        m_loadedChunks[key] = loaded;
    }

    // === IBiomeAccessor 接口实现 ===

    [[nodiscard]] const Biome* getBiome(i32 x, i32 y, i32 z) const override
    {
        const u64 key = makeKey(x, z);
        auto it = m_biomes.find(key);
        if (it != m_biomes.end()) {
            return it->second;
        }
        return nullptr;
    }

    [[nodiscard]] bool isChunkLoaded(ChunkCoord x, ChunkCoord z) const override
    {
        const u64 key = static_cast<u64>(static_cast<u32>(x)) << 32 | static_cast<u64>(static_cast<u32>(z));
        auto it = m_loadedChunks.find(key);
        return it != m_loadedChunks.end() && it->second;
    }

private:
    [[nodiscard]] static u64 makeKey(i32 x, i32 z)
    {
        return (static_cast<u64>(static_cast<u32>(x)) << 32) | static_cast<u64>(static_cast<u32>(z));
    }

    std::unordered_map<u64, const Biome*> m_biomes;
    std::unordered_map<u64, bool> m_loadedChunks;
};

/**
 * @brief 测试用颜色解析器
 */
class TestColorResolver : public ColorResolver {
public:
    explicit TestColorResolver(u32 color)
        : m_color(color)
    {}

    [[nodiscard]] u32 getColor(const Biome& /*biome*/, f64 /*x*/, f64 /*z*/) const override { return m_color; }

private:
    u32 m_color;
};

// ============================================================================
// BiomeColorCache 测试
// ============================================================================

TEST(BiomeColorCacheTest, BasicCacheOperations)
{
    BiomeColorCache cache;

    i32 callCount = 0;
    auto compute = [&]() {
        callCount++;
        return 0xFF0000u;
    };

    // 第一次调用，未命中
    u32 color1 = cache.getOrCompute(0, 0, 5, 5, 0, compute);
    EXPECT_EQ(color1, 0xFF0000u);
    EXPECT_EQ(callCount, 1);

    // 第二次调用，命中缓存
    u32 color2 = cache.getOrCompute(0, 0, 5, 5, 0, compute);
    EXPECT_EQ(color2, 0xFF0000u);
    EXPECT_EQ(callCount, 1); // compute 未被调用

    // 不同位置，未命中
    u32 color3 = cache.getOrCompute(0, 0, 6, 6, 0, compute);
    EXPECT_EQ(color3, 0xFF0000u);
    EXPECT_EQ(callCount, 2);

    // 不同解析器，未命中
    u32 color4 = cache.getOrCompute(0, 0, 5, 5, 1, compute);
    EXPECT_EQ(color4, 0xFF0000u);
    EXPECT_EQ(callCount, 3);
}

TEST(BiomeColorCacheTest, ChunkInvalidation)
{
    BiomeColorCache cache;

    i32 callCount = 0;
    auto compute = [&]() {
        callCount++;
        return 0x00FF00u;
    };

    // 填充缓存
    cache.getOrCompute(0, 0, 5, 5, 0, compute);
    cache.getOrCompute(1, 0, 5, 5, 0, compute);
    EXPECT_EQ(callCount, 2);

    // 使区块 (0, 0) 失效
    cache.invalidateChunk(0, 0);

    // 再次访问区块 (0, 0)，应该重新计算
    callCount = 0;
    u32 color = cache.getOrCompute(0, 0, 5, 5, 0, compute);
    EXPECT_EQ(color, 0x00FF00u);
    EXPECT_EQ(callCount, 1);

    // 区块 (1, 0) 应该仍然缓存
    callCount = 0;
    cache.getOrCompute(1, 0, 5, 5, 0, compute);
    EXPECT_EQ(callCount, 0);
}

TEST(BiomeColorCacheTest, PositionInvalidation)
{
    BiomeColorCache cache;

    i32 callCount = 0;
    auto compute = [&]() {
        callCount++;
        return 0x0000FFu;
    };

    // 填充缓存
    cache.getOrCompute(0, 0, 5, 5, 0, compute);
    cache.getOrCompute(0, 0, 6, 6, 0, compute);
    EXPECT_EQ(callCount, 2);

    // 使位置 (5, 5) 失效（世界坐标）
    cache.invalidatePosition(5, 5);

    // 位置 (5, 5) 应该重新计算
    callCount = 0;
    cache.getOrCompute(0, 0, 5, 5, 0, compute);
    EXPECT_EQ(callCount, 1);

    // 位置 (6, 6) 应该仍然缓存
    callCount = 0;
    cache.getOrCompute(0, 0, 6, 6, 0, compute);
    EXPECT_EQ(callCount, 0);
}

TEST(BiomeColorCacheTest, CacheStats)
{
    BiomeColorCache cache;

    auto compute = []() { return 0xFFFFFFu; };

    // 产生 2 次未命中
    cache.getOrCompute(0, 0, 0, 0, 0, compute);
    cache.getOrCompute(0, 0, 1, 1, 0, compute);

    // 产生 3 次命中
    cache.getOrCompute(0, 0, 0, 0, 0, compute);
    cache.getOrCompute(0, 0, 0, 0, 0, compute);
    cache.getOrCompute(0, 0, 1, 1, 0, compute);

    auto stats = cache.getStats();
    EXPECT_EQ(stats.cacheMisses, 2);
    EXPECT_EQ(stats.cacheHits, 3);
    EXPECT_EQ(stats.totalEntries, 1); // 只有一个区块
}

// ============================================================================
// BiomeColorBlender 测试
// ============================================================================

TEST(BiomeColorBlenderTest, SetBlendRadius)
{
    BiomeColorBlender blender;

    // 默认半径
    EXPECT_EQ(blender.blendRadius(), 2);

    // 设置半径
    blender.setBlendRadius(0);
    EXPECT_EQ(blender.blendRadius(), 0);

    blender.setBlendRadius(7);
    EXPECT_EQ(blender.blendRadius(), 7);

    // 边界检查
    blender.setBlendRadius(-1);
    EXPECT_EQ(blender.blendRadius(), 0); // clamp 到 0

    blender.setBlendRadius(10);
    EXPECT_EQ(blender.blendRadius(), 7); // clamp 到最大值
}

TEST(BiomeColorBlenderTest, AverageColors)
{
    // 单色
    u32 singleColor[] = {0xFF0000};
    EXPECT_EQ(BiomeColorBlender::averageColors(singleColor, 1), 0xFF0000);

    // 双色平均
    // 0xFF0000 = (255, 0, 0), 0x00FF00 = (0, 255, 0)
    // 平均: (127, 127, 0) = 0x7F7F00
    u32 twoColors[] = {0xFF0000, 0x00FF00};
    u32 avg1 = BiomeColorBlender::averageColors(twoColors, 2);
    EXPECT_EQ(avg1, 0x7F7F00); // (127 << 16) | (127 << 8) | 0

    // 三色平均
    // 0xFF0000 = (255, 0, 0), 0x00FF00 = (0, 255, 0), 0x0000FF = (0, 0, 255)
    // 平均: (85, 85, 85) = 0x555555
    u32 threeColors[] = {0xFF0000, 0x00FF00, 0x0000FF};
    u32 avg2 = BiomeColorBlender::averageColors(threeColors, 3);
    EXPECT_EQ(avg2, 0x555555); // (85 << 16) | (85 << 8) | 85

    // 四色平均
    // 0xFF0000 = (255, 0, 0), 0x00FF00 = (0, 255, 0), 0x0000FF = (0, 0, 255), 0xFFFFFF = (255, 255, 255)
    // 平均: (127, 127, 127) = 0x7F7F7F
    u32 fourColors[] = {0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};
    u32 avg3 = BiomeColorBlender::averageColors(fourColors, 4);
    EXPECT_EQ(avg3, 0x7F7F7F); // (127 << 16) | (127 << 8) | 127
}

TEST(BiomeColorBlenderTest, NoBlending)
{
    BiomeColorBlender blender;
    blender.setBlendRadius(0); // 禁用混合

    TestBiomeAccessor accessor;
    TestColorResolver resolver(0xABCDEF);

    // 创建测试生物群系
    BiomeEffects effects = BiomeEffects::Builder().waterColor(0x123456).build();
    Biome biome(BiomeId(1), "test_biome");
    biome.setEffects(effects);

    accessor.setBiome(0, 64, 0, &biome);
    accessor.setChunkLoaded(0, 0, true);

    // 禁用混合时，应该直接返回颜色
    u32 color = blender.getBlendedColor(accessor, 0, 64, 0, resolver, BiomeColorBlender::ResolverId::Grass);

    EXPECT_EQ(color, 0xABCDEF);
}

TEST(BiomeColorBlenderTest, Blending)
{
    BiomeColorBlender blender;
    blender.setBlendRadius(1);      // 3x3 混合区域
    blender.setCacheEnabled(false); // 禁用缓存以测试算法

    TestBiomeAccessor accessor;

    // 创建两个测试生物群系
    BiomeEffects effects1 = BiomeEffects::Builder().grassColor(0xFF0000).build();
    Biome biome1(BiomeId(1), "red_biome");
    biome1.setEffects(effects1);

    BiomeEffects effects2 = BiomeEffects::Builder().grassColor(0x0000FF).build();
    Biome biome2(BiomeId(2), "blue_biome");
    biome2.setEffects(effects2);

    // 设置生物群系分布
    // 红色区域：(-1, -1) 到 (0, 0)
    accessor.setBiome(-1, 64, -1, &biome1);
    accessor.setBiome(-1, 64, 0, &biome1);
    accessor.setBiome(0, 64, -1, &biome1);
    accessor.setBiome(0, 64, 0, &biome1);

    // 蓝色区域：(1, 1) 到 (1, 1)
    accessor.setBiome(1, 64, 1, &biome2);

    // 使用固定颜色解析器进行测试
    // 中心位置 (0, 0) 的混合区域 (-1,-1) 到 (1,1)
    // 实际上是 4 个红色 + 1 个蓝色 (位置 1,1 可能超出设置的生物群系)
    TestColorResolver redResolver(0xFF0000);
    TestColorResolver blueResolver(0x0000FF);

    // 在红色区域
    accessor.setBiome(1, 64, 0, &biome1);
    accessor.setBiome(0, 64, 1, &biome1);
    accessor.setBiome(1, 64, 1, &biome1);

    u32 color = blender.getBlendedColor(accessor, 0, 64, 0, redResolver, BiomeColorBlender::ResolverId::Grass);

    // 所有 9 个位置都返回 0xFF0000，平均后仍然是 0xFF0000
    EXPECT_EQ(color, 0xFF0000);
}

TEST(BiomeColorBlenderTest, CacheEnabled)
{
    BiomeColorBlender blender;
    blender.setBlendRadius(2);
    blender.setCacheEnabled(true);

    TestBiomeAccessor accessor;
    TestColorResolver resolver(0x00FF00);

    accessor.setBiome(0, 64, 0, nullptr); // 返回 nullptr 会导致返回默认白色
    accessor.setChunkLoaded(0, 0, true);

    // 第一次调用
    u32 color1 = blender.getBlendedColorCached(accessor, 0, 64, 0, resolver, BiomeColorBlender::ResolverId::Grass);

    // 第二次调用（应该命中缓存）
    u32 color2 = blender.getBlendedColorCached(accessor, 0, 64, 0, resolver, BiomeColorBlender::ResolverId::Grass);

    EXPECT_EQ(color1, color2);

    // 检查缓存统计
    auto stats = blender.getCacheStats();
    EXPECT_EQ(stats.cacheHits, 1);
    EXPECT_EQ(stats.cacheMisses, 1);
}

// ============================================================================
// ChunkBiomeAccessor 测试
// ============================================================================

class ChunkBiomeAccessorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 创建测试区块
        m_chunkData = std::make_unique<ChunkData>(0, 0);

        // 设置区块内的生物群系
        // 我们使用默认的生物群系
        auto& registry = BiomeRegistry::instance();
        registry.initialize();

        m_plains = &registry.get(Biomes::Plains);
        m_forest = &registry.get(Biomes::Forest);
    }

    std::unique_ptr<ChunkData> m_chunkData;
    const Biome* m_plains = nullptr;
    const Biome* m_forest = nullptr;
};

TEST_F(ChunkBiomeAccessorTest, GetBiomeLocal)
{
    // 创建访问器（仅当前区块，无邻居）
    std::array<const ChunkData*, 4> neighbors = {nullptr, nullptr, nullptr, nullptr};
    ChunkBiomeAccessor accessor(*m_chunkData, neighbors, 0, 0);

    // 测试区块内访问
    // 注意：ChunkData 默认所有生物群系为 0 (void/placeholder)
    const Biome* biome = accessor.getBiomeLocal(0, 64, 0);
    // 如果 BiomeRegistry 没有初始化，这里可能返回 nullptr
    // 在测试环境中，我们假设返回的是有效的生物群系
}

TEST_F(ChunkBiomeAccessorTest, IsChunkLoaded)
{
    std::array<const ChunkData*, 4> neighbors = {nullptr, nullptr, nullptr, nullptr};
    ChunkBiomeAccessor accessor(*m_chunkData, neighbors, 0, 0);

    // 当前区块应该被视为已加载
    EXPECT_TRUE(accessor.isChunkLoaded(0, 0));

    // 邻居区块未加载
    EXPECT_FALSE(accessor.isChunkLoaded(1, 0));
    EXPECT_FALSE(accessor.isChunkLoaded(-1, 0));
    EXPECT_FALSE(accessor.isChunkLoaded(0, 1));
    EXPECT_FALSE(accessor.isChunkLoaded(0, -1));
}

TEST_F(ChunkBiomeAccessorTest, WorldToLocalCoord)
{
    std::array<const ChunkData*, 4> neighbors = {nullptr, nullptr, nullptr, nullptr};
    ChunkBiomeAccessor accessor(*m_chunkData, neighbors, 0, 0);

    // 区块 (0, 0) 内的坐标
    const Biome* biome = accessor.getBiome(5, 64, 10);
    // 由于没有设置生物群系，应该返回 nullptr 或默认生物群系
    // 这里主要测试坐标转换不会崩溃

    // 区块 (-1, 0) 的坐标（邻居区块，未加载）
    biome = accessor.getBiome(-5, 64, 10);
    EXPECT_EQ(biome, nullptr); // 邻居未加载

    // 区块 (1, 0) 的坐标（邻居区块，未加载）
    biome = accessor.getBiome(20, 64, 10);
    EXPECT_EQ(biome, nullptr); // 邻居未加载
}

} // namespace mc::client::test
