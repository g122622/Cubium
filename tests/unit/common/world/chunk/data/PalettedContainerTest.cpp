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
 * LIABILITY, ARISING FROM AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "world/chunk/data/PalettedContainer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <set>
#include <utility>
#include <vector>

namespace mc::world::chunk {
namespace {

// ============================================================================
// 辅助函数
// ============================================================================

// 生成随机 stateId，控制唯一值数量
std::vector<u32> makeRandomData(u32 uniqueCount, u32 seed)
{
    std::mt19937 rng(seed);
    std::vector<u32> values;
    values.reserve(uniqueCount);
    for (u32 i = 0; i < uniqueCount; ++i) {
        values.push_back(i * 37 + 1); // 确保唯一且非连续
    }
    std::vector<u32> data(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        data[static_cast<size_t>(i)] = values[static_cast<size_t>(rng()) % uniqueCount];
    }
    return data;
}

// 验证容器内容与扁平数组完全一致
void expectContainerEquals(const PalettedContainer& container, const std::vector<u32>& expected)
{
    ASSERT_EQ(static_cast<i32>(expected.size()), PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), expected[static_cast<size_t>(i)]) << "索引 " << i << " 不匹配";
    }
}

// ============================================================================
// 均匀态测试（唯一值 == 1：bits = 0 且不分配 storage / 哈希表）
// ============================================================================

TEST(PalettedContainerTest, DefaultConstructorIsUniformAir)
{
    PalettedContainer container;
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), 0u);
    }
}

TEST(PalettedContainerTest, FillResetsToUniform)
{
    PalettedContainer container;
    // 先填充一些不同的值
    container.set(0, 100);
    container.set(1, 200);
    container.set(2, 300);
    EXPECT_GT(container.paletteSize(), 1);

    // fill 应该重置为均匀态
    container.fill(42);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), 42u);
    }
}

TEST(PalettedContainerTest, UniformGetAndSetSameValue)
{
    PalettedContainer container;
    container.fill(7);
    // 设置相同值应该保持均匀态
    u32 old = container.getAndSet(100, 7);
    EXPECT_EQ(old, 7u);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    EXPECT_EQ(container.get(100), 7u);
}

// ============================================================================
// 位宽增长测试：bits = max(1, ceil(log2(唯一值个数)))，从 1 位起步
// ============================================================================

/// 断言"恰好 count 个唯一值"时的位宽与它们一一对应（覆盖 1~12 位全档）
TEST(PalettedContainerTest, BitWidthMatchesPaletteSize)
{
    // {唯一值个数, 期望位宽}
    const std::pair<i32, i32> cases[] = {
        {1, 0}, {2, 1}, {3, 2}, {4, 2}, {5, 3}, {8, 3}, {9, 4}, {16, 4}, {17, 5}, {32, 5}, {33, 6}, {256, 8}};

    for (const auto& [uniqueCount, expectedBits] : cases) {
        PalettedContainer container;
        // 逐格写入 uniqueCount 个不同值。取值集合包含默认值 0，否则 0 会作为孤立条目
        // 留在调色板里（调色板不回收孤立条目，与 MC 一致），paletteSize 会多 1。
        for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
            container.set(i, static_cast<u32>(i % uniqueCount) * 37);
        }
        EXPECT_EQ(container.paletteSize(), uniqueCount) << "uniqueCount=" << uniqueCount;
        EXPECT_EQ(container.bitsPerEntry(), expectedBits) << "uniqueCount=" << uniqueCount;
    }
}

TEST(PalettedContainerTest, TransitionUniformToTwoValues)
{
    PalettedContainer container;
    container.fill(0); // 均匀态

    // 引入第二个值：升到 1 位
    container.set(0, 1);
    EXPECT_EQ(container.paletteSize(), 2);
    EXPECT_EQ(container.bitsPerEntry(), 1);
    EXPECT_EQ(container.get(0), 1u);
    EXPECT_EQ(container.get(1), 0u); // 其他位置仍为 0
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 0u);
}

TEST(PalettedContainerTest, FewValues)
{
    PalettedContainer container;
    const u32 values[] = {0, 1, 2, 3, 5, 8, 13};
    const i32 valueCount = 7;

    std::vector<u32> expected(PalettedContainer::VOLUME, 0);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        u32 v = values[i % valueCount];
        container.set(i, v);
        expected[static_cast<size_t>(i)] = v;
    }

    EXPECT_EQ(container.paletteSize(), valueCount);
    EXPECT_EQ(container.bitsPerEntry(), 3); // ceil(log2(7)) = 3
    expectContainerEquals(container, expected);
}

TEST(PalettedContainerTest, SetOverwriteDoesNotShrinkPalette)
{
    PalettedContainer container;
    container.set(0, 1);
    container.set(0, 2);
    container.set(0, 3);
    EXPECT_EQ(container.get(0), 3u);
    // 调色板不回收孤立条目（与原版 MC 一致）：值 0/1/2/3 均被加入调色板，
    // 即使索引 0 最终只引用值 3。paletteSize 反映累计添加的唯一值数。
    EXPECT_EQ(container.paletteSize(), 4);
    EXPECT_EQ(container.bitsPerEntry(), 2);
    // 但其他索引仍为 0（默认值）
    EXPECT_EQ(container.get(1), 0u);
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 0u);
}

// ============================================================================
// 大调色板 / 大 stateId
// ============================================================================

TEST(PalettedContainerTest, PaletteGrowthRaisesBits)
{
    PalettedContainer container;
    // 16 种唯一值 → ceil(log2(16)) = 4 位
    for (u32 v = 0; v < 16; ++v) {
        container.set(static_cast<i32>(v), v);
    }
    EXPECT_EQ(container.paletteSize(), 16);
    EXPECT_EQ(container.bitsPerEntry(), 4);

    // 第 17 种 → 5 位
    container.set(16, 16);
    EXPECT_EQ(container.paletteSize(), 17);
    EXPECT_EQ(container.bitsPerEntry(), 5);

    // 验证所有值
    for (u32 v = 0; v <= 16; ++v) {
        EXPECT_EQ(container.get(static_cast<i32>(v)), v);
    }
}

TEST(PalettedContainerTest, ManyValues)
{
    PalettedContainer container;
    const u32 uniqueCount = 100;
    auto data = makeRandomData(uniqueCount, 42);

    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    expectContainerEquals(container, data);
    EXPECT_EQ(container.paletteSize(), uniqueCount);
    EXPECT_EQ(container.bitsPerEntry(), 7); // ceil(log2(100)) = 7
}

TEST(PalettedContainerTest, LargeStateIdsDoNotGrowBits)
{
    // 位宽只由唯一值个数决定，与 stateId 的数值大小无关（大 id 仍存调色板索引）
    PalettedContainer container;
    container.set(0, 70000);
    EXPECT_EQ(container.get(0), 70000u);
    EXPECT_EQ(container.get(1), 0u);
    EXPECT_EQ(container.paletteSize(), 2);
    EXPECT_EQ(container.bitsPerEntry(), PalettedContainer::MIN_BITS);
}

TEST(PalettedContainerTest, LargeStateIdsRoundTrip)
{
    PalettedContainer container;
    // 设置多个大 stateId
    std::vector<u32> expected(PalettedContainer::VOLUME, 0);
    container.set(0, 100000);
    container.set(1, 200000);
    container.set(2, 300000);
    expected[0] = 100000;
    expected[1] = 200000;
    expected[2] = 300000;

    expectContainerEquals(container, expected);
}

// ============================================================================
// toFlat / fromFlat 测试
// ============================================================================

TEST(PalettedContainerTest, ToFlatUniform)
{
    PalettedContainer container;
    container.fill(42);
    auto flat = container.toFlat();
    ASSERT_EQ(static_cast<i32>(flat.size()), PalettedContainer::VOLUME);
    for (u32 v : flat) {
        EXPECT_EQ(v, 42u);
    }
}

TEST(PalettedContainerTest, FromFlatUniform)
{
    std::vector<u32> data(PalettedContainer::VOLUME, 7);
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatFewValues)
{
    std::vector<u32> data(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        data[static_cast<size_t>(i)] = static_cast<u32>(i % 10);
    }
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.paletteSize(), 10);
    EXPECT_EQ(container.bitsPerEntry(), 4); // ceil(log2(10)) = 4
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatManyValues)
{
    auto data = makeRandomData(50, 123);
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.paletteSize(), 50);
    EXPECT_EQ(container.bitsPerEntry(), 6); // ceil(log2(50)) = 6
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatAllUnique)
{
    // 所有 4096 个值都不同 → 满位宽 12
    std::vector<u32> data(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        data[static_cast<size_t>(i)] = static_cast<u32>(i);
    }
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.paletteSize(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.bitsPerEntry(), 12);
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, RoundTripToFlatFromFlat)
{
    auto data = makeRandomData(30, 999);
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    auto flat = container.toFlat();
    ASSERT_EQ(flat.size(), data.size());
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(flat[i], data[i]);
    }
}

// ============================================================================
// getAndSet 返回旧值测试
// ============================================================================

TEST(PalettedContainerTest, GetAndSetReturnsOldValue)
{
    PalettedContainer container;
    container.fill(5);

    u32 old = container.getAndSet(100, 10);
    EXPECT_EQ(old, 5u);
    EXPECT_EQ(container.get(100), 10u);

    old = container.getAndSet(100, 20);
    EXPECT_EQ(old, 10u);
    EXPECT_EQ(container.get(100), 20u);
}

TEST(PalettedContainerTest, GetAndSetAcrossBitWidths)
{
    PalettedContainer container;
    container.fill(0);

    // 均匀态 → 1 位
    container.getAndSet(0, 1);
    EXPECT_EQ(container.get(0), 1u);
    EXPECT_EQ(container.get(1), 0u);

    // 引入第三个取值 → 2 位
    container.getAndSet(1, 2);
    EXPECT_EQ(container.get(1), 2u);
    EXPECT_EQ(container.bitsPerEntry(), 2);

    // 覆盖已有值
    container.getAndSet(2, 1);
    EXPECT_EQ(container.get(2), 1u);
    EXPECT_EQ(container.bitsPerEntry(), 2);
}

// ============================================================================
// 边界与一致性测试
// ============================================================================

TEST(PalettedContainerTest, RandomStressTest)
{
    // 随机写入并验证
    PalettedContainer container;
    std::vector<u32> expected(PalettedContainer::VOLUME, 0);
    std::mt19937 rng(2024);

    for (int iter = 0; iter < 10000; ++iter) {
        i32 index = static_cast<i32>(rng() % static_cast<u32>(PalettedContainer::VOLUME));
        u32 value = rng() % 200; // 限制在 200 以内，触发各种模式
        u32 old = container.getAndSet(index, value);
        EXPECT_EQ(old, expected[static_cast<size_t>(index)]) << "iter " << iter << " index " << index;
        expected[static_cast<size_t>(index)] = value;
    }

    expectContainerEquals(container, expected);
}

TEST(PalettedContainerTest, AllIndicesAccessible)
{
    PalettedContainer container;
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        container.set(i, static_cast<u32>(i % 256));
    }
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), static_cast<u32>(i % 256));
    }
}

TEST(PalettedContainerTest, IndexZeroAndLast)
{
    PalettedContainer container;
    container.set(0, 111);
    container.set(PalettedContainer::VOLUME - 1, 222);
    EXPECT_EQ(container.get(0), 111u);
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 222u);
}

// ============================================================================
// 内存占用测试
// ============================================================================

TEST(PalettedContainerTest, UniformMemoryIsMinimal)
{
    PalettedContainer container;
    container.fill(0);
    // 均匀态：无 storage、无哈希表，只有 Data 结构与 1 项调色板
    size_t mem = container.estimatedMemoryUsage();
    EXPECT_LT(mem, 256u);
}

TEST(PalettedContainerTest, FillReleasesStorageAndHashTable)
{
    PalettedContainer container;
    for (u32 v = 0; v < 100; ++v) {
        container.set(static_cast<i32>(v), v);
    }
    const size_t packedMemory = container.estimatedMemoryUsage();
    ASSERT_GT(packedMemory, 256u);

    // fill 回到均匀态：位存储与哈希表都应被释放（而不是只留 capacity）
    container.fill(77);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_LT(container.estimatedMemoryUsage(), 256u);
    EXPECT_EQ(container.get(0), 77u);
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 77u);
}

TEST(PalettedContainerTest, PackedMemoryGrowsWithBitWidth)
{
    // 位宽越窄，位存储越小：2 值(1 位) < 4 值(2 位) < 8 值(3 位) < 16 值(4 位)
    const auto memoryFor = [](i32 uniqueCount) {
        PalettedContainer container;
        for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
            container.set(i, static_cast<u32>(i % uniqueCount) * 37 + 1);
        }
        return container.estimatedMemoryUsage();
    };

    const size_t twoValues = memoryFor(2);
    const size_t fourValues = memoryFor(4);
    const size_t eightValues = memoryFor(8);
    const size_t sixteenValues = memoryFor(16);

    EXPECT_LT(twoValues, fourValues);
    EXPECT_LT(fourValues, eightValues);
    EXPECT_LT(eightValues, sixteenValues);
    // 16 值 = 4 位 = 2048B 位存储 + 调色板 + 哈希表；远小于 16 KB 扁平数组
    EXPECT_LT(sixteenValues, PalettedContainer::VOLUME * sizeof(u32));
}

// ============================================================================
// forEach 遍历测试
// ============================================================================

TEST(PalettedContainerTest, ForEachUniform)
{
    PalettedContainer container;
    container.fill(42);
    i32 count = 0;
    container.forEach([&count](i32 index, u32 value) {
        EXPECT_EQ(value, 42u);
        ++count;
    });
    EXPECT_EQ(count, PalettedContainer::VOLUME);
}

TEST(PalettedContainerTest, ForEachPacked)
{
    PalettedContainer container;
    std::vector<u32> expected(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        u32 v = static_cast<u32>(i % 5);
        container.set(i, v);
        expected[static_cast<size_t>(i)] = v;
    }
    container.forEach([&expected](i32 index, u32 value) { EXPECT_EQ(value, expected[static_cast<size_t>(index)]); });
}

TEST(PalettedContainerTest, ForEachPaletteValue)
{
    PalettedContainer container;
    std::set<u32> values = {10, 20, 30, 40, 50};
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        container.set(i, *std::next(values.begin(), i % values.size()));
    }
    // 调色板包含默认值 0（空气）+ 设置的值
    std::set<u32> expected = values;
    expected.insert(0u);
    std::set<u32> seen;
    container.forEachPaletteValue([&seen](i32, u32 value) { seen.insert(value); });
    EXPECT_EQ(seen, expected);
}

// ============================================================================
// 拷贝与移动语义测试
// ============================================================================

TEST(PalettedContainerTest, CopyConstructor)
{
    PalettedContainer original;
    for (u32 v = 0; v < 20; ++v) {
        original.set(static_cast<i32>(v), v * 10);
    }
    PalettedContainer copy(original);
    for (u32 v = 0; v < 20; ++v) {
        EXPECT_EQ(copy.get(static_cast<i32>(v)), v * 10);
    }
}

TEST(PalettedContainerTest, MoveConstructor)
{
    PalettedContainer original;
    original.fill(99);
    PalettedContainer moved(std::move(original));
    EXPECT_EQ(moved.get(0), 99u);
    EXPECT_EQ(moved.get(PalettedContainer::VOLUME - 1), 99u);
}

TEST(PalettedContainerTest, CopyAssignment)
{
    PalettedContainer original;
    for (u32 v = 0; v < 15; ++v) {
        original.set(static_cast<i32>(v), v + 1);
    }
    PalettedContainer copy;
    copy.fill(0);
    copy = original;
    for (u32 v = 0; v < 15; ++v) {
        EXPECT_EQ(copy.get(static_cast<i32>(v)), v + 1);
    }
}

// ============================================================================
// 反向哈希表行为测试
// ============================================================================

/// 反复写入"已存在的取值"不得让调色板增长（证明反向哈希表命中，而不是每次追加）
TEST(PalettedContainerTest, ExistingValuesDoNotGrowPalette)
{
    PalettedContainer container;
    for (u32 v = 0; v < 64; ++v) {
        container.set(static_cast<i32>(v), v);
    }
    const i32 paletteSizeAfterFill = container.paletteSize();
    ASSERT_EQ(paletteSizeAfterFill, 64);

    std::mt19937 rng(2024);
    for (i32 iter = 0; iter < 5000; ++iter) {
        const i32 index = static_cast<i32>(rng() % static_cast<u32>(PalettedContainer::VOLUME));
        const u32 value = static_cast<u32>(iter % 64);
        container.set(index, value);
    }

    EXPECT_EQ(container.paletteSize(), paletteSizeAfterFill);
    EXPECT_EQ(container.bitsPerEntry(), 6); // ceil(log2(64)) = 6
}

} // namespace
} // namespace mc::world::chunk
