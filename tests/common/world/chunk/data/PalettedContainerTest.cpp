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
// SingleValue 模式测试
// ============================================================================

TEST(PalettedContainerTest, DefaultConstructorIsSingleValueZero)
{
    PalettedContainer container;
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), 0u);
    }
}

TEST(PalettedContainerTest, FillResetsToSingleValue)
{
    PalettedContainer container;
    // 先填充一些不同的值
    container.set(0, 100);
    container.set(1, 200);
    container.set(2, 300);
    EXPECT_GT(container.paletteSize(), 1);

    // fill 应该重置为 SingleValue
    container.fill(42);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.bitsPerEntry(), 0);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        EXPECT_EQ(container.get(i), 42u);
    }
}

TEST(PalettedContainerTest, SingleValueGetAndSetSameValue)
{
    PalettedContainer container;
    container.fill(7);
    // 设置相同值应该保持 SingleValue
    u32 old = container.getAndSet(100, 7);
    EXPECT_EQ(old, 7u);
    EXPECT_EQ(container.paletteSize(), 1);
    EXPECT_EQ(container.get(100), 7u);
}

// ============================================================================
// Linear 模式测试 (2-15 种唯一值)
// ============================================================================

TEST(PalettedContainerTest, TransitionSingleToLinear)
{
    PalettedContainer container;
    container.fill(0); // SingleValue

    // 引入第二个值，应转为 Linear
    container.set(0, 1);
    EXPECT_EQ(container.paletteSize(), 2);
    EXPECT_EQ(container.bitsPerEntry(), 4); // MIN_BITS
    EXPECT_EQ(container.get(0), 1u);
    EXPECT_EQ(container.get(1), 0u); // 其他位置仍为 0
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 0u);
}

TEST(PalettedContainerTest, LinearWithFewValues)
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
    expectContainerEquals(container, expected);
}

TEST(PalettedContainerTest, LinearSetOverwrite)
{
    PalettedContainer container;
    container.set(0, 1);
    container.set(0, 2);
    container.set(0, 3);
    EXPECT_EQ(container.get(0), 3u);
    // 调色板不回收孤立条目（与原版 MC 1.16.5 一致）：值 0/1/2/3 均被加入调色板，
    // 即使索引 0 最终只引用值 3。paletteSize 反映累计添加的唯一值数。
    EXPECT_EQ(container.paletteSize(), 4);
    // 但其他索引仍为 0（默认值）
    EXPECT_EQ(container.get(1), 0u);
    EXPECT_EQ(container.get(PalettedContainer::VOLUME - 1), 0u);
}

// ============================================================================
// HashMap 模式测试 (16+ 种唯一值)
// ============================================================================

TEST(PalettedContainerTest, TransitionToHashMap)
{
    PalettedContainer container;
    // 设置 16 种唯一值，应触发 HashMap 模式
    for (u32 v = 0; v < 16; ++v) {
        container.set(static_cast<i32>(v), v);
    }
    EXPECT_EQ(container.paletteSize(), 16);
    // 16 种值需要 4 位（2^4=16），但 _calculateBitsForCount(16) = 4，仍为 Linear
    // 实际上 16 种值 ceil(log2(16))=4，所以还是 Linear
    // 需要 17 种才触发 HashMap

    container.set(16, 16);
    EXPECT_EQ(container.paletteSize(), 17);
    // 17 种值需要 5 位，仍 < MIN_BITS_FOR_FLAT(16)，应为 HashMap
    EXPECT_GE(container.bitsPerEntry(), 5);

    // 验证所有值
    for (u32 v = 0; v <= 16; ++v) {
        EXPECT_EQ(container.get(static_cast<i32>(v)), v);
    }
}

TEST(PalettedContainerTest, HashMapWithManyValues)
{
    PalettedContainer container;
    const u32 uniqueCount = 100;
    auto data = makeRandomData(uniqueCount, 42);

    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    expectContainerEquals(container, data);
    EXPECT_GE(container.paletteSize(), 1);
}

// ============================================================================
// Flat 模式测试
// ============================================================================
// 注意：Flat 模式（bitsPerEntry >= 16）仅在唯一值数 >= 32768 时触发，
// 但 VOLUME=4096，调色板最多 4096 项，_calculateBitsForCount 最多返回 12。
// 因此 Flat 模式在运行时不可达，仅在 fromFlat 中显式判定 uniqueCount >= 65536
// （同样不可达，因 VOLUME=4096）。Flat 路径作为安全兜底保留。
// 大 stateId 值通过调色板索引存储，不需要更多位数。

TEST(PalettedContainerTest, TransitionToFlat)
{
    PalettedContainer container;
    // 大 stateId 不触发 Flat（位数由调色板条目数决定，非值大小）
    container.set(0, 70000);
    EXPECT_EQ(container.get(0), 70000u);
    EXPECT_EQ(container.get(1), 0u);
    // 应为 SingleValue->Linear，bits = MIN_BITS = 4
    EXPECT_EQ(container.bitsPerEntry(), PalettedContainer::MIN_BITS);
}

TEST(PalettedContainerTest, FlatWithLargeStateIds)
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

TEST(PalettedContainerTest, ToFlatSingleValue)
{
    PalettedContainer container;
    container.fill(42);
    auto flat = container.toFlat();
    ASSERT_EQ(static_cast<i32>(flat.size()), PalettedContainer::VOLUME);
    for (u32 v : flat) {
        EXPECT_EQ(v, 42u);
    }
}

TEST(PalettedContainerTest, FromFlatSingleValue)
{
    std::vector<u32> data(PalettedContainer::VOLUME, 7);
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_EQ(container.paletteSize(), 1);
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatLinear)
{
    std::vector<u32> data(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        data[static_cast<size_t>(i)] = static_cast<u32>(i % 10);
    }
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    EXPECT_LE(container.paletteSize(), 10);
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatHashMap)
{
    auto data = makeRandomData(50, 123);
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
    expectContainerEquals(container, data);
}

TEST(PalettedContainerTest, FromFlatAllUnique)
{
    // 所有 4096 个值都不同
    std::vector<u32> data(PalettedContainer::VOLUME);
    for (i32 i = 0; i < PalettedContainer::VOLUME; ++i) {
        data[static_cast<size_t>(i)] = static_cast<u32>(i);
    }
    PalettedContainer container;
    container.fromFlat(data.data(), PalettedContainer::VOLUME);
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

TEST(PalettedContainerTest, GetAndSetTransitionsModes)
{
    PalettedContainer container;
    container.fill(0);

    // SingleValue -> Linear
    container.getAndSet(0, 1);
    EXPECT_EQ(container.get(0), 1u);
    EXPECT_EQ(container.get(1), 0u);

    // Linear -> Linear (新值)
    container.getAndSet(1, 2);
    EXPECT_EQ(container.get(1), 2u);

    // 覆盖已有值
    container.getAndSet(2, 1);
    EXPECT_EQ(container.get(2), 1u);
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

TEST(PalettedContainerTest, SingleValueMemoryIsMinimal)
{
    PalettedContainer container;
    container.fill(0);
    // SingleValue 模式应该有非常小的内存占用
    // storage 为空，palette 容量 1（4B），Data 结构体 + unique_ptr 开销
    size_t mem = container.estimatedMemoryUsage();
    EXPECT_LT(mem, 256u);
}

TEST(PalettedContainerTest, LinearMemoryLessThanFlat)
{
    PalettedContainer linear;
    for (u32 v = 0; v < 10; ++v) {
        linear.set(static_cast<i32>(v), v);
    }
    // Linear 4 位：4096*4/8 = 2048 字节 + 调色板
    EXPECT_LT(linear.estimatedMemoryUsage(), 4096u * 4u); // 16 KB 扁平
}

// ============================================================================
// forEach 遍历测试
// ============================================================================

TEST(PalettedContainerTest, ForEachSingleValue)
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

TEST(PalettedContainerTest, ForEachLinear)
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
// rawPalette 测试
// ============================================================================

TEST(PalettedContainerTest, RawPaletteNonEmptyForNonFlat)
{
    PalettedContainer container;
    container.set(0, 10);
    container.set(1, 20);
    const u32* palette = container.rawPalette();
    EXPECT_NE(palette, nullptr);
    // 调色板应包含 10 和 20
    bool found10 = false;
    bool found20 = false;
    for (i32 i = 0; i < container.paletteSize(); ++i) {
        if (palette[static_cast<size_t>(i)] == 10) found10 = true;
        if (palette[static_cast<size_t>(i)] == 20) found20 = true;
    }
    EXPECT_TRUE(found10);
    EXPECT_TRUE(found20);
}

} // namespace
} // namespace mc::world::chunk
