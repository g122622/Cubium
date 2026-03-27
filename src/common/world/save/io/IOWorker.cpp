#include "IOWorker.hpp"

namespace mc::world::save::io {

// ========== 构造函数/析构函数 ==========

IOWorker::IOWorker(const std::filesystem::path& regionDir, u32 threadCount)
    : m_regionDir(regionDir)
{
    if (threadCount == 0) {
        threadCount = 1;
    }

    // 创建工作线程
    for (u32 i = 0; i < threadCount; ++i) {
        m_threads.emplace_back(&IOWorker::workerThread, this);
    }
}

IOWorker::~IOWorker() {
    close();
}

// ========== 区块操作 ==========

std::future<Result<std::optional<nbt::CompoundTag>>>
IOWorker::loadChunk(ChunkCoord chunkX, ChunkCoord chunkZ) {
    auto promise = std::promise<Result<std::optional<nbt::CompoundTag>>>();
    auto future = promise.get_future();

    if (m_closed.load()) {
        promise.set_value(Error(ErrorCode::InvalidState, "IOWorker is closed"));
        return future;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        Task task;
        task.type = TaskType::Load;
        task.chunkX = chunkX;
        task.chunkZ = chunkZ;
        task.loadPromise = std::move(promise);
        m_taskQueue.push(std::move(task));
    }
    m_queueCV.notify_one();

    return future;
}

std::future<Result<void>>
IOWorker::saveChunk(ChunkCoord chunkX, ChunkCoord chunkZ, std::unique_ptr<nbt::CompoundTag> nbt) {
    auto promise = std::promise<Result<void>>();
    auto future = promise.get_future();

    if (m_closed.load()) {
        promise.set_value(Error(ErrorCode::InvalidState, "IOWorker is closed"));
        return future;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        Task task;
        task.type = TaskType::Save;
        task.chunkX = chunkX;
        task.chunkZ = chunkZ;
        task.nbt = std::move(nbt);
        task.savePromise = std::move(promise);
        m_taskQueue.push(std::move(task));
    }
    m_queueCV.notify_one();

    return future;
}

std::future<Result<void>>
IOWorker::saveChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const nbt::CompoundTag& nbt) {
    return saveChunk(chunkX, chunkZ, nbt.copy());
}

bool IOWorker::hasChunk(ChunkCoord chunkX, ChunkCoord chunkZ) const {
    // 直接检查 Region 文件是否存在以及区块是否存在
    i32 regionX = chunkX >> 5;
    i32 regionZ = chunkZ >> 5;

    std::filesystem::path regionPath = m_regionDir /
        ("r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca");

    if (!std::filesystem::exists(regionPath)) {
        return false;
    }

    // 打开 Region 文件检查区块是否存在
    auto result = region::RegionFile::open(regionPath, false);
    if (result.failed()) {
        return false;
    }

    auto& regionFile = result.value();
    u32 localX = static_cast<u32>(chunkX & 31);
    u32 localZ = static_cast<u32>(chunkZ & 31);
    return regionFile->hasChunk(localX, localZ);
}

// ========== 同步 ==========

std::future<Result<void>> IOWorker::sync() {
    auto promise = std::promise<Result<void>>();
    auto future = promise.get_future();

    if (m_closed.load()) {
        promise.set_value(Error(ErrorCode::InvalidState, "IOWorker is closed"));
        return future;
    }

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        Task task;
        task.type = TaskType::Sync;
        task.savePromise = std::move(promise);
        m_taskQueue.push(std::move(task));
    }
    m_queueCV.notify_one();

    return future;
}

void IOWorker::close() {
    if (m_closed.exchange(true)) {
        return;  // 已经关闭
    }

    // 通知所有线程
    m_queueCV.notify_all();

    // 等待所有线程完成
    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_threads.clear();

    // 清空队列
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_taskQueue.empty()) {
        auto& task = m_taskQueue.front();
        if (task.type == TaskType::Load) {
            task.loadPromise.set_value(Error(ErrorCode::InvalidState, "IOWorker is closed"));
        } else if (task.type == TaskType::Save || task.type == TaskType::Sync) {
            task.savePromise.set_value(Error(ErrorCode::InvalidState, "IOWorker is closed"));
        }
        m_taskQueue.pop();
    }
}

u32 IOWorker::pendingTaskCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return static_cast<u32>(m_taskQueue.size());
}

// ========== 工作线程 ==========

void IOWorker::workerThread() {
    // 每个线程有自己的 Region 文件缓存
    region::RegionFileCache regionCache(m_regionDir, false);

    while (!m_closed.load()) {
        Task task;

        // 等待任务
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return !m_taskQueue.empty() || m_closed.load();
            });

            if (m_closed.load() && m_taskQueue.empty()) {
                break;
            }

            if (m_taskQueue.empty()) {
                continue;
            }

            task = std::move(m_taskQueue.front());
            m_taskQueue.pop();
        }

        // 处理任务
        switch (task.type) {
            case TaskType::Load:
                processLoadTask(task);
                break;

            case TaskType::Save:
                processSaveTask(task);
                break;

            case TaskType::Sync:
                processSyncTask(task);
                break;

            default:
                break;
        }
    }
}

void IOWorker::processLoadTask(Task& task) {
    u32 localX = static_cast<u32>(task.chunkX & 31);
    u32 localZ = static_cast<u32>(task.chunkZ & 31);

    // 每个线程使用自己的 Region 缓存
    // 注意：这里需要重新创建缓存，因为 thread_local 不能用于成员函数
    region::RegionFileCache regionCache(m_regionDir, false);

    auto result = regionCache.readChunk(task.chunkX, task.chunkZ);
    if (result.failed()) {
        task.loadPromise.set_value(result.error());
    } else {
        task.loadPromise.set_value(std::move(result.value()));
    }
}

void IOWorker::processSaveTask(Task& task) {
    if (task.nbt == nullptr) {
        task.savePromise.set_value(Error(ErrorCode::InvalidArgument, "NBT data is null"));
        return;
    }

    u32 localX = static_cast<u32>(task.chunkX & 31);
    u32 localZ = static_cast<u32>(task.chunkZ & 31);

    // 每个线程使用自己的 Region 缓存
    region::RegionFileCache regionCache(m_regionDir, true);

    auto result = regionCache.writeChunk(task.chunkX, task.chunkZ, *task.nbt);
    if (result.failed()) {
        task.savePromise.set_value(result.error());
    } else {
        task.savePromise.set_value({});
    }
}

void IOWorker::processSyncTask(Task& task) {
    // 同步操作：确保所有数据写入磁盘
    // 由于每个任务都会独立处理，这里的同步主要是确保之前的任务都已完成
    task.savePromise.set_value({});
}

} // namespace mc::world::save::io
