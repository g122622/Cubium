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

#include "world/storage/task/StorageTask.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include "world/storage/db/SectionKey.hpp"
#include "world/storage/task/StorageTaskManager.hpp"
#include <atomic>
#include <gtest/gtest.h>

namespace mc::world::storage {
namespace {

TEST(StorageTaskTest, CreateAndExecuteLoadTask)
{
    SectionKey key(1, 2, 3, 0);
    std::atomic<bool> executed{false};

    auto task = StorageTask::createLoadTask(key, [&executed](const std::atomic<bool>& abortSignal) {
        executed.store(!abortSignal.load(std::memory_order::acquire), std::memory_order::release);
        return true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->type(), util::TaskType::ChunkLoad);
    EXPECT_FALSE(task->description().empty());
    EXPECT_STREQ(task->traceCategory(), "storage.task.load");

    std::atomic<bool> abortSignal{false};
    EXPECT_TRUE(task->execute(abortSignal));
    EXPECT_TRUE(executed.load(std::memory_order::acquire));
}

TEST(StorageTaskTest, CreateAndExecuteSaveTask)
{
    SectionKey key(-4, 5, 6, 1);
    std::atomic<bool> executed{false};

    auto task = StorageTask::createSaveTask(key, true, [&executed](const std::atomic<bool>& abortSignal) {
        executed.store(!abortSignal.load(std::memory_order::acquire), std::memory_order::release);
        return true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->type(), util::TaskType::ChunkSave);
    EXPECT_STREQ(task->traceCategory(), "storage.task.save");

    std::atomic<bool> abortSignal{false};
    EXPECT_TRUE(task->execute(abortSignal));
    EXPECT_TRUE(executed.load(std::memory_order::acquire));
}

TEST(StorageTaskTest, TaskManagerSubmitsToPool)
{
    util::UniversalWorkerPool pool(1, "StorageTaskTest", 900);
    pool.start();

    StorageTaskManager manager(pool);
    std::atomic<bool> completed{false};
    std::atomic<bool> success{false};

    auto task = StorageTask::createFlushTask(0, 1, [&completed](const std::atomic<bool>& abortSignal) {
        completed.store(!abortSignal.load(std::memory_order::acquire), std::memory_order::release);
        return true;
    });

    auto taskId = manager.submit(
        std::move(task),
        util::TaskPriority::Normal,
        [&success](bool taskSuccess, util::ITask*) { success.store(taskSuccess, std::memory_order::release); },
        std::make_shared<std::atomic<bool>>(false));

    ASSERT_NE(taskId, 0u);
    pool.waitForCompletion();

    EXPECT_TRUE(completed.load(std::memory_order::acquire));
    EXPECT_TRUE(success.load(std::memory_order::acquire));
    pool.shutdown();
}

} // namespace
} // namespace mc::world::storage
