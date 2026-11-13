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
 * @file ParallelForTest.cpp
 * @brief parallelFor 的契约测试：每个下标恰好执行一次、大 count 不重不漏
 */

#include "common/util/thread/ParallelFor.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

namespace mc::util {
namespace {

/**
 * @brief 每个下标恰好被执行一次，且覆盖 [0, count)
 *
 * 用 set + mutex 而不是写 vector[index]，这样"重复执行"能被检测出来，而不是被覆盖掩盖。
 */
TEST(ParallelForTest, ExecutesEveryIndexExactlyOnce)
{
    constexpr i32 COUNT = 5000;

    std::mutex seenMutex;
    std::set<i32> seen;
    parallelFor(COUNT, [&seenMutex, &seen](i32 index) {
        std::lock_guard<std::mutex> lock(seenMutex);
        seen.insert(index);
    });

    ASSERT_EQ(static_cast<i32>(seen.size()), COUNT);
    EXPECT_EQ(*seen.begin(), 0);
    EXPECT_EQ(*seen.rbegin(), COUNT - 1);
}

/** @brief count 为 0 时不得调用 body，也不得崩溃。 */
TEST(ParallelForTest, ZeroCountDoesNothing)
{
    std::atomic<i32> calls{0};
    parallelFor(0, [&calls](i32) { calls.fetch_add(1, std::memory_order::relaxed); });
    EXPECT_EQ(calls.load(std::memory_order::acquire), 0);
}

/**
 * @brief 单个元素时串行执行（走不与多线程竞争的退化路径）
 *
 * 断言 body 在调用线程上执行：parallelFor 的契约是"调用线程自己也参与"，
 * 因此 count==1 时不会额外建线程。
 */
TEST(ParallelForTest, SingleItemRunsOnCallerThread)
{
    const std::thread::id callerId = std::this_thread::get_id();
    std::thread::id bodyId;
    parallelFor(1, [&bodyId](i32 index) {
        bodyId = std::this_thread::get_id();
        EXPECT_EQ(index, 0);
    });
    EXPECT_EQ(bodyId, callerId);
}

/**
 * @brief 负 count 属于调用方违约，必须断言崩溃而不是静默跳过
 *
 * 生产构建关闭了异常，违约一律走 MC_ASSERT_RELEASE（abort），故用 EXPECT_DEATH 验证。
 */
TEST(ParallelForTest, NegativeCountAborts)
{
    EXPECT_DEATH(parallelFor(-1, [](i32) {}), "count >= 0");
}

} // namespace
} // namespace mc::util
