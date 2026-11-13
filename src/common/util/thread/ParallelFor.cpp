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

#include "ParallelFor.hpp"

#include "common/util/assert/AssertAll.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

namespace mc::util {

void parallelFor(i32 count, const std::function<void(i32)>& body)
{
    MC_ASSERT_RELEASE(count >= 0);
    if (count == 0) {
        return;
    }

    const i32 hardware = static_cast<i32>(std::max(1u, std::thread::hardware_concurrency()));
    const i32 workerCount = std::min(count, hardware);

    // 单个元素或单核机器：串行执行，省掉线程创建与原子领取的开销。
    if (workerCount <= 1) {
        for (i32 index = 0; index < count; ++index) {
            body(index);
        }
        return;
    }

    // 动态领取：relaxed 足够，nextIndex 只用来发号，不与其它数据建立先后关系。
    std::atomic<i32> nextIndex{0};
    const auto run = [&nextIndex, count, &body]() {
        while (true) {
            const i32 index = nextIndex.fetch_add(1, std::memory_order::relaxed);
            if (index >= count) {
                return;
            }
            body(index);
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(workerCount - 1));
    for (i32 i = 0; i < workerCount - 1; ++i) {
        workers.emplace_back(run);
    }

    // 调用线程也参与，少建一条线程。
    run();

    for (auto& worker : workers) {
        worker.join();
    }
}

} // namespace mc::util
