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
 * @file CacheBenchmark.cpp
 * @brief 缓存性能对比测试
 *
 * 对比 Long2IntLRUCache（原有实现）和 OpenAddressingLRUCache（新实现）的性能。
 */

#include "util/cache/Long2IntLRUCache.hpp"
#include "util/cache/OpenAddressingLRUCache.hpp"
#include <chrono>
#include <random>
#include <vector>
#include <gtest/gtest.h>

namespace mc {
namespace {

/**
 * @brief 生成随机坐标对
 */
std::vector<std::pair<i32, i32>> generateRandomCoords(i32 count, i32 range, u64 seed = 12345)
{
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<i32> dist(-range, range);

    std::vector<std::pair<i32, i32>> coords;
    coords.reserve(static_cast<size_t>(count));
    for (i32 i = 0; i < count; ++i) {
        coords.emplace_back(dist(rng), dist(rng));
    }
    return coords;
}

// ============================================================================
// Long2IntLRUCache 测试
// ============================================================================

TEST(Long2IntLRUCacheTest, BasicOperations)
{
    Long2IntLRUCache cache(100);

    // 测试 put 和 get
    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);

    i32 value;
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_EQ(value, 200);
    EXPECT_TRUE(cache.get(3, value));
    EXPECT_EQ(value, 300);

    // 测试未命中
    EXPECT_FALSE(cache.get(999, value));
}

TEST(Long2IntLRUCacheTest, PackCoords)
{
    // 测试坐标打包
    i64 key1 = Long2IntLRUCache::packCoords(100, 200);
    i64 key2 = Long2IntLRUCache::packCoords(100, 200);
    EXPECT_EQ(key1, key2);

    // 不同坐标应该产生不同的键
    i64 key3 = Long2IntLRUCache::packCoords(200, 100);
    EXPECT_NE(key1, key3);

    // 负坐标
    i64 key4 = Long2IntLRUCache::packCoords(-100, -200);
    i64 key5 = Long2IntLRUCache::packCoords(-100, -200);
    EXPECT_EQ(key4, key5);
}

TEST(Long2IntLRUCacheTest, LRUEviction)
{
    Long2IntLRUCache cache(3);

    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);
    EXPECT_EQ(cache.size(), 3);

    // 添加第 4 个，应该淘汰最旧的
    cache.put(4, 400);
    EXPECT_EQ(cache.size(), 3);

    i32 value;
    // 键 1 应该被淘汰
    EXPECT_FALSE(cache.get(1, value));
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_TRUE(cache.get(3, value));
    EXPECT_TRUE(cache.get(4, value));
}

TEST(Long2IntLRUCacheTest, UpdateValue)
{
    Long2IntLRUCache cache(10);

    cache.put(1, 100);
    cache.put(1, 200); // 更新

    i32 value;
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, 200);
    EXPECT_EQ(cache.size(), 1);
}

// ============================================================================
// OpenAddressingLRUCache 测试
// ============================================================================

TEST(OpenAddressingLRUCacheTest, BasicOperations)
{
    OpenAddressingLRUCache cache(100);

    // 测试 put 和 get
    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);

    i32 value;
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(cache.get(2, value));
    EXPECT_EQ(value, 200);
    EXPECT_TRUE(cache.get(3, value));
    EXPECT_EQ(value, 300);

    // 测试未命中
    EXPECT_FALSE(cache.get(999, value));
}

TEST(OpenAddressingLRUCacheTest, PackCoords)
{
    // 测试坐标打包
    i64 key1 = OpenAddressingLRUCache::packCoords(100, 200);
    i64 key2 = OpenAddressingLRUCache::packCoords(100, 200);
    EXPECT_EQ(key1, key2);

    // 不同坐标应该产生不同的键
    i64 key3 = OpenAddressingLRUCache::packCoords(200, 100);
    EXPECT_NE(key1, key3);

    // 负坐标
    i64 key4 = OpenAddressingLRUCache::packCoords(-100, -200);
    i64 key5 = OpenAddressingLRUCache::packCoords(-100, -200);
    EXPECT_EQ(key4, key5);
}

TEST(OpenAddressingLRUCacheTest, LRUEviction)
{
    OpenAddressingLRUCache cache(3);

    cache.put(1, 100);
    cache.put(2, 200);
    cache.put(3, 300);
    EXPECT_EQ(cache.size(), 3);

    // 添加更多条目，应该触发批量淘汰
    cache.put(4, 400);
    cache.put(5, 500);
    cache.put(6, 600);

    // 容量应该保持或低于最大值
    EXPECT_LE(cache.size(), 3);
}

TEST(OpenAddressingLRUCacheTest, UpdateValue)
{
    OpenAddressingLRUCache cache(10);

    cache.put(1, 100);
    cache.put(1, 200); // 更新

    i32 value;
    EXPECT_TRUE(cache.get(1, value));
    EXPECT_EQ(value, 200);
    EXPECT_EQ(cache.size(), 1);
}

TEST(OpenAddressingLRUCacheTest, LockedOperations)
{
    OpenAddressingLRUCache cache(100);

    // 批量操作（持有锁）
    std::lock_guard<std::mutex> lock(cache.getMutex());

    cache.putLocked(1, 100);
    cache.putLocked(2, 200);
    cache.putLocked(3, 300);

    i32 value;
    EXPECT_TRUE(cache.getLocked(1, value));
    EXPECT_EQ(value, 100);
    EXPECT_TRUE(cache.getLocked(2, value));
    EXPECT_EQ(value, 200);
}

TEST(OpenAddressingLRUCacheTest, Statistics)
{
    OpenAddressingLRUCache cache(100);

    cache.put(1, 100);
    cache.put(2, 200);

    i32 value;
    cache.get(1, value); // 命中
    cache.get(2, value); // 命中
    cache.get(3, value); // 未命中

    EXPECT_EQ(cache.hitCount(), 2);
    EXPECT_EQ(cache.missCount(), 1);

    cache.resetStats();
    EXPECT_EQ(cache.hitCount(), 0);
    EXPECT_EQ(cache.missCount(), 0);
}

// ============================================================================
// 性能对比测试
// ============================================================================

class CacheBenchmark : public ::testing::Test {
protected:
    static constexpr i32 CACHE_SIZE = 1024;
    static constexpr i32 NUM_OPERATIONS = 100000;
    static constexpr i32 COORD_RANGE = 10000;
};

TEST_F(CacheBenchmark, Long2IntLRUCache_WritePerformance)
{
    Long2IntLRUCache cache(CACHE_SIZE);
    auto coords = generateRandomCoords(NUM_OPERATIONS, COORD_RANGE);

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& [x, z] : coords) {
        i64 key = Long2IntLRUCache::packCoords(x, z);
        cache.put(key, x * z);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 记录结果用于比较（不是断言）
    std::cout << "[Long2IntLRUCache] Write " << NUM_OPERATIONS << " operations: " << duration.count() << " us"
              << std::endl;
}

TEST_F(CacheBenchmark, OpenAddressingLRUCache_WritePerformance)
{
    OpenAddressingLRUCache cache(CACHE_SIZE);
    auto coords = generateRandomCoords(NUM_OPERATIONS, COORD_RANGE);

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& [x, z] : coords) {
        i64 key = OpenAddressingLRUCache::packCoords(x, z);
        cache.put(key, x * z);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "[OpenAddressingLRUCache] Write " << NUM_OPERATIONS << " operations: " << duration.count() << " us"
              << std::endl;
}

TEST_F(CacheBenchmark, Long2IntLRUCache_ReadPerformance)
{
    Long2IntLRUCache cache(CACHE_SIZE);
    auto coords = generateRandomCoords(NUM_OPERATIONS, COORD_RANGE);

    // 预填充缓存
    for (const auto& [x, z] : coords) {
        i64 key = Long2IntLRUCache::packCoords(x, z);
        cache.put(key, x * z);
    }

    cache.clear();
    for (i32 i = 0; i < CACHE_SIZE; ++i) {
        cache.put(i, i * 2);
    }

    auto start = std::chrono::high_resolution_clock::now();

    i32 value;
    i32 hitCount = 0;
    for (const auto& [x, z] : coords) {
        i64 key = Long2IntLRUCache::packCoords(x, z);
        if (cache.get(key, value)) {
            ++hitCount;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "[Long2IntLRUCache] Read " << NUM_OPERATIONS << " operations: " << duration.count() << " us"
              << ", hits: " << hitCount << std::endl;
}

TEST_F(CacheBenchmark, OpenAddressingLRUCache_ReadPerformance)
{
    OpenAddressingLRUCache cache(CACHE_SIZE);
    auto coords = generateRandomCoords(NUM_OPERATIONS, COORD_RANGE);

    // 预填充缓存
    for (const auto& [x, z] : coords) {
        i64 key = OpenAddressingLRUCache::packCoords(x, z);
        cache.put(key, x * z);
    }

    cache.clear();
    for (i32 i = 0; i < CACHE_SIZE; ++i) {
        cache.put(i, i * 2);
    }

    auto start = std::chrono::high_resolution_clock::now();

    i32 value;
    i32 hitCount = 0;
    for (const auto& [x, z] : coords) {
        i64 key = OpenAddressingLRUCache::packCoords(x, z);
        if (cache.get(key, value)) {
            ++hitCount;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "[OpenAddressingLRUCache] Read " << NUM_OPERATIONS << " operations: " << duration.count() << " us"
              << ", hits: " << hitCount << std::endl;
}

TEST_F(CacheBenchmark, OpenAddressingLRUCache_BatchPerformance)
{
    OpenAddressingLRUCache cache(CACHE_SIZE);
    constexpr i32 BATCH_SIZE = 64;
    auto coords = generateRandomCoords(BATCH_SIZE, COORD_RANGE);

    // 测试批量操作
    auto start = std::chrono::high_resolution_clock::now();

    for (i32 iter = 0; iter < NUM_OPERATIONS / BATCH_SIZE; ++iter) {
        std::lock_guard<std::mutex> lock(cache.getMutex());
        for (const auto& [x, z] : coords) {
            i64 key = OpenAddressingLRUCache::packCoords(x + iter * 100, z);
            i32 value;
            if (!cache.getLocked(key, value)) {
                cache.putLocked(key, x * z);
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "[OpenAddressingLRUCache] Batch read/write " << (NUM_OPERATIONS / BATCH_SIZE) << " batches (size "
              << BATCH_SIZE << "): " << duration.count() << " us" << std::endl;
}

// ============================================================================
// 缓存一致性测试
// ============================================================================

TEST(CacheConsistencyTest, SameCoordinates_SameValue)
{
    // 使用足够大的容量以容纳所有条目 (21 x 21 = 441)
    Long2IntLRUCache cache1(1000);
    OpenAddressingLRUCache cache2(1000);

    // 相同坐标应该产生相同结果
    // 注意：两个缓存使用不同的 packCoords 方法，但逻辑一致
    for (i32 x = -10; x <= 10; ++x) {
        for (i32 z = -10; z <= 10; ++z) {
            i64 key1 = Long2IntLRUCache::packCoords(x, z);
            i64 key2 = OpenAddressingLRUCache::packCoords(x, z);

            cache1.put(key1, x * 100 + z);
            cache2.put(key2, x * 100 + z);
        }
    }

    i32 value1, value2;
    for (i32 x = -10; x <= 10; ++x) {
        for (i32 z = -10; z <= 10; ++z) {
            i64 key1 = Long2IntLRUCache::packCoords(x, z);
            i64 key2 = OpenAddressingLRUCache::packCoords(x, z);

            EXPECT_TRUE(cache1.get(key1, value1)) << "Failed at (" << x << ", " << z << ")";
            EXPECT_TRUE(cache2.get(key2, value2)) << "Failed at (" << x << ", " << z << ")";
            EXPECT_EQ(value1, value2) << "Values differ at (" << x << ", " << z << ")";
        }
    }
}

TEST(CacheConsistencyTest, ClearAndRefill)
{
    Long2IntLRUCache cache1(2000);
    OpenAddressingLRUCache cache2(2000);

    // 填充缓存
    for (i32 x = -20; x <= 20; ++x) {
        for (i32 z = -20; z <= 20; ++z) {
            i64 key1 = Long2IntLRUCache::packCoords(x, z);
            i64 key2 = OpenAddressingLRUCache::packCoords(x, z);
            cache1.put(key1, x * 100 + z);
            cache2.put(key2, x * 100 + z);
        }
    }

    // 清空缓存
    cache1.clear();
    cache2.clear();

    EXPECT_EQ(cache1.size(), 0);
    EXPECT_EQ(cache2.size(), 0);

    // 重新填充
    for (i32 x = -5; x <= 5; ++x) {
        for (i32 z = -5; z <= 5; ++z) {
            i64 key1 = Long2IntLRUCache::packCoords(x, z);
            i64 key2 = OpenAddressingLRUCache::packCoords(x, z);
            cache1.put(key1, x * 100 + z);
            cache2.put(key2, x * 100 + z);
        }
    }

    // 验证重新填充的值
    i32 value1, value2;
    for (i32 x = -5; x <= 5; ++x) {
        for (i32 z = -5; z <= 5; ++z) {
            i64 key1 = Long2IntLRUCache::packCoords(x, z);
            i64 key2 = OpenAddressingLRUCache::packCoords(x, z);
            EXPECT_TRUE(cache1.get(key1, value1)) << "Failed at (" << x << ", " << z << ")";
            EXPECT_TRUE(cache2.get(key2, value2)) << "Failed at (" << x << ", " << z << ")";
            EXPECT_EQ(value1, value2);
        }
    }
}

} // namespace
} // namespace mc
