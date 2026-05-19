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

#include "physics/CollisionCache.hpp"
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::physics;

namespace {

AxisAlignedBB makeBox(f32 minX, f32 minY, f32 minZ, f32 maxX, f32 maxY, f32 maxZ)
{
    return AxisAlignedBB(minX, minY, minZ, maxX, maxY, maxZ);
}

} // namespace

/**
 * @brief CollisionCache 单元测试
 *
 * 测试碰撞箱缓存系统的基本功能：
 * - 缓存存储和检索
 * - 缓存失效
 * - 统计信息
 */
class CollisionCacheTest : public ::testing::Test {
protected:
    void SetUp() override { cache = std::make_unique<CollisionCache>(); }

    void TearDown() override { cache.reset(); }

    std::unique_ptr<CollisionCache> cache;
};

// ========== 基本缓存操作测试 ==========

TEST_F(CollisionCacheTest, CacheAndRetrieve)
{
    // 创建测试碰撞箱
    std::vector<AxisAlignedBB> boxes = {
        makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f), makeBox(5.0f, 0.0f, 5.0f, 6.0f, 1.0f, 6.0f)};

    // 缓存碰撞箱
    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes));

    // 检索缓存
    const auto* cached = cache->getChunkCollisionBoxes(0, 0);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 2);
    EXPECT_EQ((*cached)[0], makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f));
    EXPECT_EQ((*cached)[1], makeBox(5.0f, 0.0f, 5.0f, 6.0f, 1.0f, 6.0f));
}

TEST_F(CollisionCacheTest, CacheCopy)
{
    std::vector<AxisAlignedBB> boxes = {makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};

    // 使用拷贝版本
    cache->cacheChunkCollisionBoxes(1, 2, boxes);

    // 验证原始数据仍然存在
    EXPECT_EQ(boxes.size(), 1);

    // 检索缓存
    const auto* cached = cache->getChunkCollisionBoxes(1, 2);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 1);
}

TEST_F(CollisionCacheTest, RetrieveNonExistent)
{
    const auto* cached = cache->getChunkCollisionBoxes(999, 999);
    EXPECT_EQ(cached, nullptr);

    // 检查统计
    EXPECT_EQ(cache->missCount(), 1);
    EXPECT_EQ(cache->hitCount(), 0);
}

TEST_F(CollisionCacheTest, MultipleChunks)
{
    // 缓存多个区块
    for (int i = 0; i < 5; ++i) {
        std::vector<AxisAlignedBB> boxes = {AxisAlignedBB(static_cast<f32>(i * 16),
            0.0f,
            static_cast<f32>(i * 16),
            static_cast<f32>(i * 16 + 16),
            256.0f,
            static_cast<f32>(i * 16 + 16))};
        cache->cacheChunkCollisionBoxes(i, i, std::move(boxes));
    }

    // 验证所有区块都能检索
    for (int i = 0; i < 5; ++i) {
        const auto* cached = cache->getChunkCollisionBoxes(i, i);
        ASSERT_NE(cached, nullptr);
        EXPECT_EQ(cached->size(), 1);
    }

    EXPECT_EQ(cache->size(), 5);
}

// ========== 缓存失效测试 ==========

TEST_F(CollisionCacheTest, InvalidateSingle)
{
    std::vector<AxisAlignedBB> boxes = {makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes));

    EXPECT_EQ(cache->size(), 1);

    // 使缓存失效
    bool result = cache->invalidateChunk(0, 0);
    EXPECT_TRUE(result);
    EXPECT_EQ(cache->size(), 0);

    // 再次检索应该返回 nullptr
    const auto* cached = cache->getChunkCollisionBoxes(0, 0);
    EXPECT_EQ(cached, nullptr);
}

TEST_F(CollisionCacheTest, InvalidateNonExistent)
{
    bool result = cache->invalidateChunk(999, 999);
    EXPECT_FALSE(result);
}

TEST_F(CollisionCacheTest, InvalidateAndNeighbors)
{
    // 缓存中心区块及其邻居
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dz = -2; dz <= 2; ++dz) {
            std::vector<AxisAlignedBB> boxes = {makeBox(static_cast<f32>(dx),
                0.0f,
                static_cast<f32>(dz),
                static_cast<f32>(dx + 1),
                1.0f,
                static_cast<f32>(dz + 1))};
            cache->cacheChunkCollisionBoxes(dx, dz, std::move(boxes));
        }
    }

    EXPECT_EQ(cache->size(), 25);

    // 使 (0, 0) 及其邻居失效（半径 1）
    cache->invalidateChunkAndNeighbors(0, 0, 1);

    // 应该失效 9 个区块：(-1,-1) 到 (1,1)
    EXPECT_EQ(cache->size(), 25 - 9);

    // 验证失效的区块
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dz = -1; dz <= 1; ++dz) {
            const auto* cached = cache->getChunkCollisionBoxes(dx, dz);
            EXPECT_EQ(cached, nullptr) << "Chunk (" << dx << ", " << dz << ") should be invalidated";
        }
    }

    // 验证未失效的区块
    const auto* cached = cache->getChunkCollisionBoxes(2, 2);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 1);
}

TEST_F(CollisionCacheTest, ClearAll)
{
    for (int i = 0; i < 10; ++i) {
        std::vector<AxisAlignedBB> boxes = {makeBox(
            static_cast<f32>(i), 0.0f, static_cast<f32>(i), static_cast<f32>(i + 1), 1.0f, static_cast<f32>(i + 1))};
        cache->cacheChunkCollisionBoxes(i, i, std::move(boxes));
    }

    EXPECT_EQ(cache->size(), 10);

    cache->clear();

    EXPECT_EQ(cache->size(), 0);
    EXPECT_TRUE(cache->empty());
}

// ========== 统计测试 ==========

TEST_F(CollisionCacheTest, HitMissStats)
{
    std::vector<AxisAlignedBB> boxes = {makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes));

    // 命中
    cache->getChunkCollisionBoxes(0, 0);
    cache->getChunkCollisionBoxes(0, 0);
    EXPECT_EQ(cache->hitCount(), 2);

    // 未命中
    cache->getChunkCollisionBoxes(1, 1);
    cache->getChunkCollisionBoxes(2, 2);
    EXPECT_EQ(cache->missCount(), 2);

    // 重置统计
    cache->resetStats();
    EXPECT_EQ(cache->hitCount(), 0);
    EXPECT_EQ(cache->missCount(), 0);
}

// ========== 版本号测试 ==========

TEST_F(CollisionCacheTest, VersionNumber)
{
    std::vector<AxisAlignedBB> boxes = {makeBox(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f)};
    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes), 42);

    const auto* cacheEntry = cache->getChunkCache(0, 0);
    ASSERT_NE(cacheEntry, nullptr);
    EXPECT_EQ(cacheEntry->version, 42);
    EXPECT_EQ(cacheEntry->boxes.size(), 1);
}

// ========== 负坐标测试 ==========

TEST_F(CollisionCacheTest, NegativeCoordinates)
{
    std::vector<AxisAlignedBB> boxes = {makeBox(-16.0f, 0.0f, -16.0f, -15.0f, 1.0f, -15.0f)};
    cache->cacheChunkCollisionBoxes(-1, -1, std::move(boxes));

    const auto* cached = cache->getChunkCollisionBoxes(-1, -1);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 1);
}

// ========== 空碰撞箱列表测试 ==========

TEST_F(CollisionCacheTest, EmptyBoxList)
{
    std::vector<AxisAlignedBB> boxes; // 空列表
    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes));

    const auto* cached = cache->getChunkCollisionBoxes(0, 0);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 0);
}

// ========== 大量数据测试 ==========

TEST_F(CollisionCacheTest, LargeNumberOfBoxes)
{
    // 创建大量碰撞箱
    std::vector<AxisAlignedBB> boxes;
    for (int i = 0; i < 1000; ++i) {
        boxes.emplace_back(static_cast<f32>(i % 16),
            static_cast<f32>(i / 256),
            static_cast<f32>((i / 16) % 16),
            static_cast<f32>((i % 16) + 1),
            static_cast<f32>((i / 256) + 1),
            static_cast<f32>(((i / 16) % 16) + 1));
    }

    cache->cacheChunkCollisionBoxes(0, 0, std::move(boxes));

    const auto* cached = cache->getChunkCollisionBoxes(0, 0);
    ASSERT_NE(cached, nullptr);
    EXPECT_EQ(cached->size(), 1000);
}
