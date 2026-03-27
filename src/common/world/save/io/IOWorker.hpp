#pragma once

#include "../region/RegionFileCache.hpp"
#include "../serializer/ChunkSerializer.hpp"
#include "../../../core/Types.hpp"
#include "../../../core/Result.hpp"
#include "../../../util/nbt/Nbt.hpp"
#include "../../../world/chunk/ChunkPos.hpp"
#include <filesystem>
#include <memory>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <atomic>

namespace mc::world::save::io {

/**
 * @brief 异步 I/O 工作线程
 *
 * 使用线程池处理区块读写请求，避免阻塞主线程。
 * 参考 MC 1.16.5 IOWorker.java
 *
 * ## 使用示例
 * ```cpp
 * IOWorker worker("saves/MyWorld/region", 1);
 *
 * // 异步加载区块
 * auto loadFuture = worker.loadChunk(0, 0);
 * // ... 做其他事情 ...
 * auto loadResult = loadFuture.get();
 * if (loadResult.success() && loadResult.value().has_value()) {
 *     auto& nbt = loadResult.value().value();
 * }
 *
 * // 异步保存区块
 * auto saveFuture = worker.saveChunk(0, 0, std::move(nbt));
 * saveFuture.get();
 *
 * // 同步所有写入
 * worker.sync().get();
 *
 * // 关闭
 * worker.close();
 * ```
 *
 * ## 线程安全
 *
 * 所有公共方法都是线程安全的。
 */
class IOWorker {
public:
    /// 任务类型枚举
    enum class TaskType : u8 {
        Load,       ///< 加载任务
        Save,       ///< 保存任务
        Sync,       ///< 同步任务
        Close       ///< 关闭任务
    };

    /// 任务结果
    struct TaskResult {
        bool success = false;
        std::optional<nbt::CompoundTag> data;
        String error;
    };

    /// 任务回调类型
    using TaskCallback = std::function<void(TaskResult)>;

    /**
     * @brief 构造 I/O 工作线程
     *
     * @param regionDir Region 文件目录
     * @param threadCount 工作线程数量（默认 1）
     */
    explicit IOWorker(const std::filesystem::path& regionDir, u32 threadCount = 1);

    ~IOWorker();

    // 禁止拷贝和移动
    IOWorker(const IOWorker&) = delete;
    IOWorker& operator=(const IOWorker&) = delete;
    IOWorker(IOWorker&&) = delete;
    IOWorker& operator=(IOWorker&&) = delete;

    // ========== 区块操作 ==========

    /**
     * @brief 异步加载区块
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @return 返回 future，结果为 optional<CompoundTag>
     *         （区块不存在返回 nullopt）
     */
    [[nodiscard]] std::future<Result<std::optional<nbt::CompoundTag>>>
    loadChunk(ChunkCoord chunkX, ChunkCoord chunkZ);

    /**
     * @brief 异步保存区块
     *
     * @param chunkX 区块 X 坐标（世界坐标）
     * @param chunkZ 区块 Z 坐标（世界坐标）
     * @param nbt 区块 NBT 数据
     * @return 返回 future
     */
    std::future<Result<void>>
    saveChunk(ChunkCoord chunkX, ChunkCoord chunkZ, std::unique_ptr<nbt::CompoundTag> nbt);

    /**
     * @brief 异步保存区块（拷贝版本）
     */
    std::future<Result<void>>
    saveChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const nbt::CompoundTag& nbt);

    /**
     * @brief 检查区块是否存在
     *
     * @param chunkX 区块 X 坐标
     * @param chunkZ 区块 Z 坐标
     * @return 如果区块存在返回 true
     *
     * @note 此方法是同步的，直接检查文件
     */
    [[nodiscard]] bool hasChunk(ChunkCoord chunkX, ChunkCoord chunkZ) const;

    // ========== 同步 ==========

    /**
     * @brief 同步所有待写入操作
     *
     * @return 返回 future
     */
    std::future<Result<void>> sync();

    /**
     * @brief 关闭工作线程
     *
     * 等待所有任务完成后关闭。
     */
    void close();

    /**
     * @brief 检查是否已关闭
     */
    [[nodiscard]] bool isClosed() const { return m_closed.load(); }

    /**
     * @brief 获取待处理任务数
     */
    [[nodiscard]] u32 pendingTaskCount() const;

private:
    /// 任务结构
    struct Task {
        TaskType type;
        ChunkCoord chunkX;
        ChunkCoord chunkZ;
        std::unique_ptr<nbt::CompoundTag> nbt;
        std::promise<Result<std::optional<nbt::CompoundTag>>> loadPromise;
        std::promise<Result<void>> savePromise;
    };

    /// 工作线程函数
    void workerThread();

    /// 处理加载任务
    void processLoadTask(Task& task);

    /// 处理保存任务
    void processSaveTask(Task& task);

    /// 处理同步任务
    void processSyncTask(Task& task);

    // ========== 成员变量 ==========

    std::filesystem::path m_regionDir;     ///< Region 文件目录
    std::vector<std::thread> m_threads;     ///< 工作线程
    std::queue<Task> m_taskQueue;           ///< 任务队列
    mutable std::mutex m_queueMutex;        ///< 队列锁
    std::condition_variable m_queueCV;      ///< 队列条件变量
    std::atomic<bool> m_closed{false};      ///< 是否已关闭

    /// 每个线程的 Region 文件缓存
    /// 使用 thread_local 在线程内部存储
};

} // namespace mc::world::save::io
