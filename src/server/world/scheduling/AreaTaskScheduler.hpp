#pragma once

#include "common/core/Types.hpp"
#include "common/util/thread/ITask.hpp"
#include "common/world/chunk/base/ChunkPos.hpp"
#include <atomic>
#include <cstddef>
#include <memory>

namespace mc::util {
class UniversalWorkerPool;
}

namespace mc::server {

/**
 * @brief 单个世界的区域任务调度器，复用外部计算线程池，不创建线程。
 *
 * 重叠区域按优先级和提交顺序串行；等待区域的任务不占 worker。
 * 生成、光照和区块序列化必须使用同一世界的实例，跨世界的坐标互不干扰。
 * 宿主必须先 shutdown 本调度器并排空计算池，再销毁任务可能访问的世界数据。
 */
class AreaTaskScheduler {
public:
    /** @brief 注入外部执行器；nullptr 表示尚未装配。 */
    explicit AreaTaskScheduler(util::UniversalWorkerPool* executor);
    ~AreaTaskScheduler();

    AreaTaskScheduler(const AreaTaskScheduler&) = delete;
    AreaTaskScheduler& operator=(const AreaTaskScheduler&) = delete;

    /** @brief 空闲时更新执行器；执行器须存活至调度器关闭且计算池排空。 */
    void setExecutor(util::UniversalWorkerPool* executor);

    /**
     * @brief 接管任务并登记 [center±radius] 区域，radius 小于零时按零处理。
     *
     * 回调包含在区域互斥与完成语义内；任务提交后不得通过裸指针访问。
     * 返回世界内唯一任务 ID；无法接纳时执行取消收尾、通知失败并返回零。
     */
    u64 submit(std::unique_ptr<util::ITask> task,
        util::TaskCallback callback,
        ChunkPos center,
        i32 radius,
        util::TaskPriority priority,
        std::shared_ptr<std::atomic<bool>> abortSignal);

    /** @brief 取消尚未执行的任务，立即移除区域依赖并在锁外执行收尾。 */
    bool cancel(u64 taskId);

    /** @brief 移除令牌已取消的等待任务，不等待其区域释放。 */
    void pruneCancelledTasks();

    /** @brief 等待本世界的区域任务及其回调全部完成。 */
    void waitForCompletion();

    /** @brief 停止接纳任务、取消排队任务并等待运行任务；不得在任务回调中调用。 */
    void shutdown();

    /** @brief 返回尚未开始执行的任务数，包括区域等待和已提交到计算池的任务。 */
    [[nodiscard]] size_t pendingTaskCount() const;

    /** @brief 返回仍等待区域的任务数，不重复计算已进入计算池的任务。 */
    [[nodiscard]] size_t waitingTaskCount() const;

private:
    struct State;
    std::shared_ptr<State> m_state;
};

} // namespace mc::server
