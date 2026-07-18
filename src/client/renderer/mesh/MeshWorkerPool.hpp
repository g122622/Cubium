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

#pragma once

#include "client/renderer/MeshTypes.hpp"
#include "common/core/Types.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mc::client {

/**
 * @brief 网格执行任务
 *
 * MeshWorkerPool 只负责执行，不再承担优先级调度。
 * 任务排序、重排和取消策略由独立调度器负责。
 */
struct MeshWorkerTask {
    ChunkId chunkId;
    u64 taskId = 0;
    std::shared_ptr<const ChunkData> chunkData;
    std::array<std::shared_ptr<const ChunkData>, 6> neighbors;
    std::shared_ptr<std::atomic<bool>> abortSignal;
};

/**
 * @brief 网格执行结果
 */
struct MeshWorkerResult {
    ChunkId chunkId;
    u64 taskId = 0;
    i32 workerId = -1; // 构建该结果的 worker 索引，主线程归还 MeshData 时按此分桶
    MeshData solidMesh;
    MeshData transparentMesh;
    bool success = false;
    bool cancelled = false;
};

/**
 * @brief 网格执行线程池
 *
 * 该类是纯执行器：
 * - 只按提交顺序消费任务
 * - 不感知视锥、不感知优先级
 * - 通过 abortSignal 配合 ChunkMesher 实现协作取消
 */
class MeshWorkerPool {
public:
    explicit MeshWorkerPool(i32 threadCount);
    ~MeshWorkerPool();

    MeshWorkerPool(const MeshWorkerPool&) = delete;
    MeshWorkerPool& operator=(const MeshWorkerPool&) = delete;

    void start();
    void shutdown();

    [[nodiscard]] bool isRunning() const;

    void submit(MeshWorkerTask task);

    void drainCompleted(const std::function<void(MeshWorkerResult&&)>& callback, u32 maxCount);

    /**
     * @brief 归还已上传完毕的 MeshData，供对应 worker 下次构建复用其 capacity。
     *
     * 主线程在上传 GPU 后调用，按 workerId 分桶入池；池满或 workerId 无效时直接析构释放。
     * 归还前会对异常膨胀的 capacity 做 shrink_to_fit，防止峰值 capacity 永久驻留。
     *
     * @param workerId 构建该 MeshData 的 worker 索引（来自 MeshWorkerResult::workerId）
     * @param solid 实心层网格（已 clear，capacity 可复用）
     * @param transparent 透明层网格（已 clear，capacity 可复用）
     */
    void recycle(i32 workerId, MeshData&& solid, MeshData&& transparent);

    [[nodiscard]] size_t queuedTaskCount() const;
    [[nodiscard]] size_t runningTaskCount() const;
    [[nodiscard]] size_t completedTaskCount() const;
    [[nodiscard]] i32 threadCount() const { return m_threadCount; }

private:
    struct InternalTask {
        MeshWorkerTask task;
    };

    /// 每桶缓存的 MeshData 上限（solid/transparent 各算一个槽位）。
    /// worker 数通常 2-8，每桶 2 个足够吸收稳态归还抖动，超出直接析构释放。
    static constexpr size_t kRecycleSlotsPerBucket = 2;

    /// 容量膨胀防护阈值（顶点数）。任务 5 将 Solid reserve 上限降为 8192 faces = 32768 顶点，
    /// 取其 1.5 倍作防护：超过此 capacity 且远大于实际 size 时触发 shrink_to_fit，
    /// 防止某次峰值后超大 capacity 永久驻留池里。
    static constexpr size_t kMaxReuseVertexCapacity = 49152;
    static constexpr size_t kMaxReuseIndexCapacity = kMaxReuseVertexCapacity * 6 / 4;

    /// 每 worker 一个回收桶，主线程按 workerId 归还、worker 自取，桶内一把锁。
    /// 桶含 std::mutex（不可拷贝/移动），故用 unique_ptr 持有：vector 本身可重分配，
    /// 而 bucket 地址稳定（worker 自取时只索引不持有指针，安全）。
    struct RecycleBucket {
        std::vector<MeshData> solidSlots;
        std::vector<MeshData> transparentSlots;
        std::mutex mutex;
    };

    void _workerLoop(i32 workerId);
    void _executeTask(i32 workerId, const MeshWorkerTask& task);

    [[nodiscard]] static bool _isCancelled(const MeshWorkerTask& task);
    [[nodiscard]] static i32 _getOptimalThreadCount();

    /// 从指定 worker 桶取出一个带历史 capacity 的 MeshData；桶空则默认构造（capacity=0）。
    [[nodiscard]] MeshData _acquireRecycled(i32 workerId, bool transparent);
    /// 归还单个 MeshData 入桶前做膨胀防护，桶满则不入池（右值参析构释放）。
    void _recycleOne(i32 workerId, bool transparent, MeshData&& data);
    /// capacity 异常膨胀时 shrink_to_fit，其余情况保留 capacity 供复用。
    static void _shrinkIfBloated(MeshData& data);

    std::vector<std::thread> m_workers;
    i32 m_threadCount = 0;

    std::queue<InternalTask> m_taskQueue;
    mutable std::mutex m_queueMutex;
    std::condition_variable m_condition;

    std::queue<MeshWorkerResult> m_completedQueue;
    mutable std::mutex m_completedMutex;

    std::vector<std::unique_ptr<RecycleBucket>> m_recycleBuckets; // size == m_threadCount

    std::atomic<size_t> m_runningTaskCount{0};
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stop{false};
};

} // namespace mc::client
