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

#include "common/world/chunk/data/light/SWMRNibbleArray.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <utility>
#include <vector>

namespace mc {
namespace {

/**
 * SWMRNibbleArray 的单写多读（Single Writer Multi Reader）生命周期回归测试。
 *
 * 【背景】该类的可见侧是无锁读的：读者经 m_storageVisible 载入裸指针后直接读取，不参与
 * 引用计数。而更新侧在"未 dirty"时会与可见侧指向**同一块**数组（单一缓冲，用于省内存）。
 * 因此任何"替换或清空更新侧"的操作若顺带释放了那块数组，就会让并发读者命中
 * heap-use-after-free。Java 版（Moonrise SWMRNibbleArray）靠 GC 天然规避该问题。
 *
 * 【修复前的实测崩溃】nightly CI 的三个 GameTest 分片各自复现过：
 *   - it4 ：toByteArray() 读取 2048 字节时命中已释放内存（主线程，MinecraftServer 光照回调）；
 *   - it11：setFull() 写入 2048 字节时命中已释放内存（SkyStarLightEngine::initNibble）；
 *   - it14：getVisible() 读取 1 字节时命中已释放内存（SpreadableSnowyDirtBlock::randomTick）。
 *
 * 下面各用例覆盖修复后必须成立的生命周期不变式。ASAN 构建下这些用例本身就是有效的
 * 释放后使用探测器：一旦不变式被破坏，读取已归还对象池的缓冲会被立即捕获。
 */
class SWMRNibbleArrayTest : public ::testing::Test {
protected:
    static constexpr i32 INDEX_AT_ZERO = 0;
    // 线性索引空间为 ARRAY_SIZE * 2 个 nibble，末位索引即最后一个 nibble。
    static constexpr i32 INDEX_AT_END = static_cast<i32>(SWMRNibbleArray::ARRAY_SIZE) * 2 - 1;
};

/**
 * 未 dirty 时，写入必须触发写时复制，且不得影响已发布可见侧的内容。
 *
 * 这是 it14 的直接回归：修复前 _ensureWritable() 会在写时复制后立即释放旧数组，
 * 而旧数组正是可见侧发布的同一块内存。
 */
TEST_F(SWMRNibbleArrayTest, WriteAfterPublishCopiesInsteadOfAliasing)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    // 先写入并发布：可见侧从此持有一块独立数组。
    nibble.set(INDEX_AT_ZERO, 0x5);
    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x5);

    // 再写入另一个索引：必须走写时复制，且旧索引的可见值保持不变。
    nibble.set(INDEX_AT_END, 0xA);
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x5) << "写时复制不应改动已发布的可见侧数据";
    EXPECT_EQ(nibble.getVisible(INDEX_AT_END), 0x0) << "新值在发布前不应出现在可见侧";
    EXPECT_EQ(nibble.getUpdating(INDEX_AT_END), 0xA);

    // 发布后可见侧才应更新，且先前写入的值仍在。
    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x5);
    EXPECT_EQ(nibble.getVisible(INDEX_AT_END), 0xA);
}

/**
 * setNull() 之后仍可安全读取可见侧，且不释放已发布的数组。
 *
 * 这是 it11 的直接回归：修复前 setNull() 在 dirty 时会归还更新侧缓冲，而随后的
 * setFull() 又会向该缓冲写入（SkyStarLightEngine::initNibble 的路径）。
 * setNull() 只应影响"更新侧"，可见侧必须继续有效直到 updateVisible() 取消发布。
 */
TEST_F(SWMRNibbleArrayTest, SetNullKeepsVisibleArrayAliveUntilPublish)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    nibble.set(INDEX_AT_ZERO, 0x7);
    ASSERT_TRUE(nibble.updateVisible());
    ASSERT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x7);

    // setNull() 只清空更新侧；可见侧在本次发布前仍应可读且内容不变。
    nibble.setNull();
    EXPECT_TRUE(nibble.isNullUpdating());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x7) << "setNull() 不应影响已发布的可见侧";

    // 取消发布：此后可见侧读 0，且不得出现释放后使用。
    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_TRUE(nibble.isNullVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x0);
}

/**
 * setFull() 在已发布状态下必须分配新的私有缓冲，而不是原地覆写可见侧。
 *
 * 这是 it11 中 setFull() 崩溃点的正向覆盖：修复前该路径会把 m_storageVisible
 * 重定向到新缓冲（旧数组随即被释放），使此前载入旧指针的读者读到已释放内存。
 */
TEST_F(SWMRNibbleArrayTest, SetFullAfterPublishDoesNotReusePublishedBuffer)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    nibble.set(INDEX_AT_ZERO, 0x3);
    ASSERT_TRUE(nibble.updateVisible());
    ASSERT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x3);

    // 整块置满：可见侧在发布前必须保持旧值。
    nibble.setFull();
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x3) << "setFull() 不应在发布前改动可见侧";
    EXPECT_EQ(nibble.getUpdating(INDEX_AT_ZERO), 0xF);

    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0xF);
    EXPECT_EQ(nibble.getVisible(INDEX_AT_END), 0xF);
}

/**
 * 可见侧在"取消发布"后，其缓冲不得被立即回收：本对象必须继续持有它。
 *
 * 该类无法在语言层面表达"读者仍持有指针"，因此这里以行为方式验证：取消发布后
 * 再次发布，数据仍能正确往返（若中途被错误释放，ASAN 构建下会直接报错）。
 */
TEST_F(SWMRNibbleArrayTest, RepublishAfterUnpublishKeepsDataConsistent)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    nibble.set(INDEX_AT_ZERO, 0x9);
    ASSERT_TRUE(nibble.updateVisible());

    // 取消发布，再重新写入并发布，数据必须仍然自洽。
    nibble.setNull();
    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x0);

    nibble.set(INDEX_AT_ZERO, 0x4);
    ASSERT_TRUE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x4);
    EXPECT_EQ(nibble.getUpdating(INDEX_AT_ZERO), 0x4);
}

/**
 * toByteArray() 必须能安全读取已发布的可见侧。
 *
 * 这是 it4 的直接回归：修复前 toByteArray() 读取 2048 字节时命中已释放内存
 * （主线程经 MinecraftServer::setupWorldCallbacks 的光照回调调用）。
 */
TEST_F(SWMRNibbleArrayTest, ToByteArrayReadsPublishedData)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    nibble.set(INDEX_AT_ZERO, 0x6);
    ASSERT_TRUE(nibble.updateVisible());

    const std::vector<u8> bytes = nibble.toByteArray();
    ASSERT_EQ(bytes.size(), SWMRNibbleArray::ARRAY_SIZE);
    // 索引 0 位于第 0 字节的低 4 位。
    EXPECT_EQ(static_cast<u8>(bytes[0] & 0x0F), 0x6);
}

/**
 * 多线程并发读可见侧、单线程反复写入并发布，全程不得出现释放后使用。
 *
 * 复刻真实的单写多读使用方式：工作线程（光照引擎）持续写入并发布，主线程持续读取。
 * 修复前，写者一旦触发写时复制或 setNull()，读者就会读到已释放的缓冲。
 */
TEST_F(SWMRNibbleArrayTest, ConcurrentReadDuringRepeatedPublishIsSafe)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));
    nibble.set(INDEX_AT_ZERO, 0x1);
    ASSERT_TRUE(nibble.updateVisible());

    std::atomic<bool> stop{false};
    std::atomic<u64> reads{0};
    // 在读者线程内只做原子累加，不用 gtest 断言（多线程下断言宏并非线程安全）；
    // 读到非法 nibble 值时记录下来，回到主线程再断言。
    std::atomic<bool> sawOutOfRange{false};

    // 读者：模拟主线程随机刻读取可见光照值。
    std::vector<std::thread> readers;
    for (i32 t = 0; t < 4; ++t) {
        readers.emplace_back([&nibble, &stop, &reads, &sawOutOfRange]() {
            while (!stop.load(std::memory_order::relaxed)) {
                // 每次读取都必须返回合法 nibble 值（0-15）；读已释放内存会被 ASAN 直接捕获。
                const u8 v = nibble.getVisible(0);
                if (v > 0x0F) {
                    sawOutOfRange.store(true, std::memory_order::relaxed);
                }
                reads.fetch_add(1, std::memory_order::relaxed);
            }
        });
    }

    // 写者：模拟光照引擎反复"写入 + 发布"（it14 / it4 的路径）。
    // 该路径下已发布的可见数组只会被分配一次，此后更新侧反复走写时复制并回拷，
    // 可见侧指针始终不变——这正是 SWMR 双缓冲的预期行为，读者因此绝不会读到已释放内存。
    for (i32 i = 0; i < 200; ++i) {
        nibble.set(INDEX_AT_ZERO, static_cast<u8>(i & 0x0F));
        nibble.updateVisible();
    }

    stop.store(true, std::memory_order::relaxed);
    for (auto& t : readers) {
        t.join();
    }

    EXPECT_FALSE(sawOutOfRange.load()) << "可见侧读到了非法的 nibble 值，疑似读到已回收的缓冲";
    EXPECT_GT(reads.load(), 0U) << "读者线程应至少完成一次读取";
    // 循环最后一次写入为 i=199，其低 4 位为 7，且已随之发布。
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x7);
}

/**
 * 移动构造与移动赋值之后，源对象必须处于可安全析构的状态，且目标对象数据完好。
 *
 * 该类由 ChunkData 按值存放于 std::array 中，容器搬迁会触发移动操作；
 * 若移动后源对象仍残留指向已转出缓冲的指针，析构时即会二次释放。
 */
TEST_F(SWMRNibbleArrayTest, MoveTransfersOwnershipWithoutDoubleFree)
{
    SWMRNibbleArray source = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));
    source.set(INDEX_AT_ZERO, 0xB);
    ASSERT_TRUE(source.updateVisible());

    SWMRNibbleArray moved(std::move(source));
    EXPECT_EQ(moved.getVisible(INDEX_AT_ZERO), 0xB);
    // 源对象已置空，读取应返回 0 而非解引用残留指针。
    EXPECT_EQ(source.getVisible(INDEX_AT_ZERO), 0x0);

    SWMRNibbleArray assigned = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));
    assigned = std::move(moved);
    EXPECT_EQ(assigned.getVisible(INDEX_AT_ZERO), 0xB);
    EXPECT_EQ(moved.getVisible(INDEX_AT_ZERO), 0x0);

    // 三个对象在此依次析构，任何重复释放都会被 ASAN 捕获。
}

/**
 * 未 dirty 时发布不应产生新数组（isDirty()==false ⇒ updateVisible() 返回 false）。
 */
TEST_F(SWMRNibbleArrayTest, UpdateVisibleIsNoOpWhenNotDirty)
{
    SWMRNibbleArray nibble = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));

    nibble.set(INDEX_AT_ZERO, 0x2);
    ASSERT_TRUE(nibble.updateVisible());
    // 已同步：再次发布应为无操作，否则会无谓地替换可见数组。
    EXPECT_FALSE(nibble.updateVisible());
    EXPECT_EQ(nibble.getVisible(INDEX_AT_ZERO), 0x2);
}

/**
 * extrudeLower() 在已发布状态下不得原地覆写可见侧数组。
 */
TEST_F(SWMRNibbleArrayTest, ExtrudeLowerAfterPublishDoesNotClobberVisible)
{
    SWMRNibbleArray target = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));
    target.set(INDEX_AT_ZERO, 0xC);
    ASSERT_TRUE(target.updateVisible());

    SWMRNibbleArray other = SWMRNibbleArray::fromData(std::vector<u8>(SWMRNibbleArray::ARRAY_SIZE, 0x00));
    other.set(INDEX_AT_ZERO, 0xD);
    ASSERT_TRUE(other.updateVisible());

    target.extrudeLower(other);

    // 挤压只作用于更新侧，可见侧在发布前应保持原值。
    EXPECT_EQ(target.getVisible(INDEX_AT_ZERO), 0xC) << "extrudeLower() 不应在发布前改动可见侧";
    EXPECT_EQ(target.getUpdating(INDEX_AT_ZERO), 0xD);

    ASSERT_TRUE(target.updateVisible());
    EXPECT_EQ(target.getVisible(INDEX_AT_ZERO), 0xD);
}

} // namespace
} // namespace mc
