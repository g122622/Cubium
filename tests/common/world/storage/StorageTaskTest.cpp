#include <gtest/gtest.h>
#include "world/storage/task/StorageTask.hpp"
#include "world/storage/task/StorageTaskManager.hpp"
#include "common/util/thread/ServerWorkerPool.hpp"
#include "world/storage/db/SectionKey.hpp"
#include <atomic>

namespace mc::world::storage {
namespace {

TEST(StorageTaskTest, CreateAndExecuteLoadTask)
{
    SectionKey key(1, 2, 3, 0);
    std::atomic<bool> executed{false};

    auto task = StorageTask::createLoadTask(key, [&executed](const std::atomic<bool>& cancelSignal) {
        executed.store(!cancelSignal.load(std::memory_order_acquire), std::memory_order_release);
        return true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->type(), util::TaskType::ChunkLoad);
    EXPECT_FALSE(task->description().empty());
    EXPECT_STREQ(task->traceCategory(), "storage.task.load");

    std::atomic<bool> cancelSignal{false};
    EXPECT_TRUE(task->execute(cancelSignal));
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST(StorageTaskTest, CreateAndExecuteSaveTask)
{
    SectionKey key(-4, 5, 6, 1);
    std::atomic<bool> executed{false};

    auto task = StorageTask::createSaveTask(key, true, [&executed](const std::atomic<bool>& cancelSignal) {
        executed.store(!cancelSignal.load(std::memory_order_acquire), std::memory_order_release);
        return true;
    });

    ASSERT_NE(task, nullptr);
    EXPECT_EQ(task->type(), util::TaskType::ChunkSave);
    EXPECT_STREQ(task->traceCategory(), "storage.task.save");

    std::atomic<bool> cancelSignal{false};
    EXPECT_TRUE(task->execute(cancelSignal));
    EXPECT_TRUE(executed.load(std::memory_order_acquire));
}

TEST(StorageTaskTest, TaskManagerSubmitsToPool)
{
    util::ServerWorkerPool pool(1, "StorageTaskTest");
    pool.start();

    StorageTaskManager manager(pool);
    std::atomic<bool> completed{false};
    std::atomic<bool> success{false};

    auto task = StorageTask::createFlushTask(0, 1, [&completed](const std::atomic<bool>& cancelSignal) {
        completed.store(!cancelSignal.load(std::memory_order_acquire), std::memory_order_release);
        return true;
    });

    auto taskId = manager.submit(std::move(task), util::TaskPriority::Normal,
        [&success](bool taskSuccess, util::ITask*) {
            success.store(taskSuccess, std::memory_order_release);
        }, std::make_shared<std::atomic<bool>>(false));

    ASSERT_NE(taskId, 0u);
    pool.waitForCompletion();

    EXPECT_TRUE(completed.load(std::memory_order_acquire));
    EXPECT_TRUE(success.load(std::memory_order_acquire));
    pool.shutdown();
}

} // namespace
} // namespace mc::world::storage
