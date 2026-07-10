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

#include "common/world/storage/section/SectionManager.hpp"
#include "common/core/Constants.hpp"
#include "common/profiler/TraceEvents.hpp"
#include <mutex>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

SectionManager::SectionManager(RocksDBDatabase& db, DimensionId dimension, const Config& config)
    : m_db(db)
    , m_dimension(dimension)
    , m_cfName(cf::getSectionCF(dimension))
    , m_config(config)
    , m_cache(config.cacheCapacity)
{
    spdlog::info("SectionManager created for dimension {} (CF: {})", static_cast<i32>(m_dimension), m_cfName);
}

// ============================================================================
// Section加载
// ============================================================================

Result<std::shared_ptr<const SectionData>> SectionManager::loadSectionSync(const SectionKey& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::loadSectionSync",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    // 检查维度是否匹配
    if (key.dimension != m_dimension) {
        return Error(ErrorCode::InvalidArgument,
            fmt::format("Dimension mismatch: expected {}, got {}",
                static_cast<i32>(m_dimension),
                static_cast<i32>(key.dimension)));
    }

    // 先检查缓存
    auto cached = m_cache.get(key);
    if (cached) {
        MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadSectionSync.cacheHit");
        return std::static_pointer_cast<const SectionData>(cached);
    }

    // 从数据库加载
    return _loadFromDatabase(key);
}

std::future<Result<std::shared_ptr<const SectionData>>> SectionManager::loadSectionAsync(
    const SectionKey& key, util::TaskPriority priority, std::shared_ptr<std::atomic<bool>> abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::loadSectionAsync",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    auto promise = std::make_shared<std::promise<Result<std::shared_ptr<const SectionData>>>>();
    auto future = promise->get_future();

    auto executor = [this, key, promise](const std::atomic<bool>& abortSignal) {
        if (abortSignal.load(std::memory_order::acquire)) {
            promise->set_value(Error(ErrorCode::InvalidState, "Load section task cancelled"));
            return false;
        }

        promise->set_value(loadSectionSync(key));
        return true;
    };

    if (!m_taskManager) {
        promise->set_value(loadSectionSync(key));
        return future;
    }

    auto task = StorageTask::createLoadTask(key, std::move(executor));
    // 使用外部 abortSignal（可取消）或内部不可取消令牌
    auto signal = abortSignal ? std::move(abortSignal) : std::make_shared<std::atomic<bool>>(false);
    m_taskManager->submit(std::move(task), priority, nullptr, std::move(signal));
    return future;
}

std::future<Result<std::vector<std::shared_ptr<const SectionData>>>> SectionManager::loadSectionsAsync(
    const std::vector<SectionKey>& keys, util::TaskPriority priority, std::shared_ptr<std::atomic<bool>> abortSignal)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadSectionsAsync", "count", keys.size());

    auto promise = std::make_shared<std::promise<Result<std::vector<std::shared_ptr<const SectionData>>>>>();
    auto future = promise->get_future();

    // 阶段1：缓存命中部分在调用线程同步取出（持 m_cache 短锁），收集未命中 key
    std::vector<std::shared_ptr<const SectionData>> results(keys.size());
    std::vector<SectionKey> missedKeys;
    std::vector<size_t> missedIndexes;
    missedKeys.reserve(keys.size());
    missedIndexes.reserve(keys.size());

    for (size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        if (key.dimension != m_dimension) {
            promise->set_value(Error(ErrorCode::InvalidArgument,
                fmt::format("Dimension mismatch: expected {}, got {}",
                    static_cast<i32>(m_dimension),
                    static_cast<i32>(key.dimension))));
            return future;
        }

        auto cached = m_cache.get(key);
        if (cached) {
            results[i] = std::static_pointer_cast<const SectionData>(cached);
            continue;
        }

        missedKeys.push_back(key);
        missedIndexes.push_back(i);
    }

    // 全部命中缓存：直接完成
    if (missedKeys.empty()) {
        promise->set_value(std::move(results));
        return future;
    }

    // 阶段2：未命中部分提交到 ServerIO 线程池批量读取，executor 内合并缓存命中与未命中结果
    auto _mergeExecutor = [this,
                              missedKeys = std::move(missedKeys),
                              missedIndexes = std::move(missedIndexes),
                              results = std::move(results),
                              promise](const std::atomic<bool>& abortSig) mutable {
        if (abortSig.load(std::memory_order::acquire)) {
            promise->set_value(Error(ErrorCode::InvalidState, "Load sections batch task cancelled"));
            return false;
        }
        auto batchResult = _loadFromDatabaseBatch(missedKeys);
        if (batchResult.failed()) {
            promise->set_value(batchResult.error());
            return true;
        }
        const auto& missedResults = batchResult.value();
        for (size_t i = 0; i < missedResults.size(); ++i) {
            results[missedIndexes[i]] = missedResults[i];
        }
        promise->set_value(std::move(results));
        return true;
    };

    if (!m_taskManager) {
        // 无线程池（测试/独立模式）：同步降级直接执行 executor
        std::atomic<bool> dummySig{false};
        _mergeExecutor(dummySig);
        return future;
    }

    // 提交到 ServerIO 线程池（descKey 仅用于 StorageTask 描述，取首个未命中 key）
    SectionKey descKey = missedKeys.front();
    auto mergeTask = StorageTask::createLoadTask(descKey, std::move(_mergeExecutor));
    auto signal = abortSignal ? std::move(abortSignal) : std::make_shared<std::atomic<bool>>(false);
    m_taskManager->submit(std::move(mergeTask), priority, nullptr, std::move(signal));
    return future;
}

Result<std::vector<std::shared_ptr<const SectionData>>> SectionManager::loadSectionsSync(
    const std::vector<SectionKey>& keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadSectionsSync", "count", keys.size());

    std::vector<std::shared_ptr<const SectionData>> results(keys.size());
    std::vector<SectionKey> missedKeys;
    std::vector<size_t> missedIndexes;
    missedKeys.reserve(keys.size());
    missedIndexes.reserve(keys.size());

    for (size_t i = 0; i < keys.size(); ++i) {
        const auto& key = keys[i];
        if (key.dimension != m_dimension) {
            return Error(ErrorCode::InvalidArgument,
                fmt::format("Dimension mismatch: expected {}, got {}",
                    static_cast<i32>(m_dimension),
                    static_cast<i32>(key.dimension)));
        }

        auto cached = m_cache.get(key);
        if (cached) {
            results[i] = std::static_pointer_cast<const SectionData>(cached);
            continue;
        }

        missedKeys.push_back(key);
        missedIndexes.push_back(i);
    }

    if (missedKeys.empty()) {
        return results;
    }

    auto batchResult = _loadFromDatabaseBatch(missedKeys);
    if (batchResult.failed()) {
        return batchResult.error();
    }

    const auto& missedResults = batchResult.value();
    MC_ASSERT_RELEASE(missedResults.size() == missedIndexes.size());
    for (size_t i = 0; i < missedResults.size(); ++i) {
        results[missedIndexes[i]] = missedResults[i];
    }

    return results;
}

// ============================================================================
// Section保存
// ============================================================================

Result<void> SectionManager::saveSectionSync(const SectionKey& key, const SectionData& data, bool immediate)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::saveSectionSync",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY),
        "immediate",
        immediate);

    // 检查维度是否匹配
    if (key.dimension != m_dimension) {
        return Error(ErrorCode::InvalidArgument,
            fmt::format("Dimension mismatch: expected {}, got {}",
                static_cast<i32>(m_dimension),
                static_cast<i32>(key.dimension)));
    }

    // 保存到数据库
    auto result = _saveToDatabase(key, data, immediate);
    if (!result.success()) {
        return result;
    }

    // 更新缓存
    auto dataCopy = std::make_shared<SectionData>(data);
    m_cache.put(key, dataCopy, false); // 保存后标记为干净

    // 从脏集合中移除
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.erase(key);
    }

    return {};
}

std::future<Result<void>> SectionManager::saveSectionAsync(
    const SectionKey& key, const SectionData& data, util::TaskPriority priority)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::saveSectionAsync",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    auto promise = std::make_shared<std::promise<Result<void>>>();
    auto future = promise->get_future();

    auto executor = [this, key, data, promise](const std::atomic<bool>& abortSignal) {
        if (abortSignal.load(std::memory_order::acquire)) {
            promise->set_value(Error(ErrorCode::InvalidState, "Save section task cancelled"));
            return false;
        }

        promise->set_value(saveSectionSync(key, data));
        return true;
    };

    if (!m_taskManager) {
        promise->set_value(saveSectionSync(key, data));
        return future;
    }

    auto task = StorageTask::createSaveTask(key, false, std::move(executor));
    m_taskManager->submit(std::move(task), priority, nullptr, std::make_shared<std::atomic<bool>>(false));
    return future;
}

Result<size_t> SectionManager::flushDirtySections()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::flushDirtySections");

    // 获取所有脏Section
    auto dirtySections = m_cache.getDirtySections();

    if (dirtySections.empty()) {
        return 0;
    }

    spdlog::info("Flushing {} dirty sections", dirtySections.size());

    // 批量保存
    rocksdb::WriteBatch batch;
    size_t savedCount = 0;
    const bool syncWrites = m_config.consistencyMode != ConsistencyMode::Eventual;

    for (const auto& [key, data] : dirtySections) {
        if (!data) {
            continue;
        }

        // 序列化Section
        auto serializeResult = data->serialize();
        if (!serializeResult.success()) {
            spdlog::error("Failed to serialize section at ({}, {}, {}): {}",
                key.chunkX,
                key.chunkZ,
                static_cast<i32>(key.sectionY),
                serializeResult.error().message());
            continue;
        }

        // 添加到批次
        auto keyBytes = key.toKey();
        const auto& valueBytes = serializeResult.value();

        rocksdb::Slice keySlice(reinterpret_cast<const char*>(keyBytes.data()), keyBytes.size());
        rocksdb::Slice valueSlice(reinterpret_cast<const char*>(valueBytes.data()), valueBytes.size());

        auto* cf = m_db.getCF(m_cfName);
        if (!cf) {
            return Error(ErrorCode::InvalidState, fmt::format("Column family not found: {}", m_cfName));
        }

        batch.Put(cf, keySlice, valueSlice);
        ++savedCount;
    }

    // 执行批量写入
    if (savedCount > 0) {
        auto writeResult = m_db.writeBatch(batch, syncWrites);
        if (!writeResult.success()) {
            return writeResult.error();
        }
    }

    // 标记所有脏Section为干净
    for (const auto& [key, data] : dirtySections) {
        m_cache.markClean(key);
    }

    // 只清理已成功写入的键，避免序列化失败的数据被错误丢失。
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        for (const auto& [key, data] : dirtySections) {
            if (data) {
                m_dirtySet.erase(key);
            }
        }
    }

    spdlog::info("Flushed {} dirty sections", savedCount);

    return savedCount;
}

Result<size_t> SectionManager::saveAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::saveAll");

    // 获取所有缓存中的Section，确保 saveAll 真正保存全部内容。
    auto allSections = m_cache.getAllSections();
    size_t savedCount = 0;

    for (const auto& [key, data] : allSections) {
        if (!data) {
            continue;
        }

        auto result = saveSectionSync(key, *data, false);
        if (result.success()) {
            ++savedCount;
        }
    }

    return savedCount;
}

// ============================================================================
// Section卸载
// ============================================================================

Result<void> SectionManager::unloadSection(const SectionKey& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::unloadSection",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    // 检查是否为脏
    bool isDirty = m_cache.isDirty(key);

    // 如果为脏，先保存
    if (isDirty) {
        auto data = m_cache.get(key);
        if (data) {
            auto saveResult = saveSectionSync(key, *data, false);
            if (!saveResult.success()) {
                return saveResult;
            }
        }
    }

    // 从缓存中移除
    m_cache.evict(key);

    // 从脏集合中移除
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.erase(key);
    }

    return {};
}

Result<void> SectionManager::unloadAll()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::unloadAll");

    // 保存所有脏Section
    auto flushResult = flushDirtySections();
    if (!flushResult.success()) {
        return flushResult.error();
    }

    // 清空缓存
    m_cache.clear();

    // 清空脏集合
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.clear();
    }

    return {};
}

// ============================================================================
// Section删除
// ============================================================================

Result<void> SectionManager::deleteSection(const SectionKey& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::deleteSection",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    // 从数据库删除
    auto keyBytes = key.toKey();
    auto result = m_db.del(m_cfName, keyBytes);
    if (!result.success() && result.error().code() != ErrorCode::NotFound) {
        return result;
    }

    // 从缓存中移除
    m_cache.evict(key);

    // 从脏集合中移除
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.erase(key);
    }

    return {};
}

Result<size_t> SectionManager::deleteChunkSections(i32 chunkX, i32 chunkZ)
{
    MC_TRACE_SCOPED_EVENT(
        TraceEvents.Storage.Section, "SectionManager::deleteChunkSections", "chunkX", chunkX, "chunkZ", chunkZ);

    // 注意：sectionY 使用有符号字节直接序列化时，字节序排序并不适合做范围删除。
    // 这里逐个删除，避免删除范围在 RocksDB 字典序下失效。
    size_t removedCount = 0;
    for (i8 sectionY = world::MIN_SECTION_Y; sectionY <= world::MAX_SECTION_Y; ++sectionY) {
        SectionKey key(chunkX, chunkZ, sectionY, m_dimension);

        auto removeResult = deleteSection(key);
        if (removeResult.success()) {
            ++removedCount;
        }

        m_cache.evict(key);
    }

    return removedCount;
}

// ============================================================================
// 缓存管理
// ============================================================================

void SectionManager::setCacheCapacity(size_t capacity)
{
    m_cache.setCapacity(capacity);
    m_config.cacheCapacity = capacity;
}

SectionCache::CacheStats SectionManager::getCacheStats() const
{
    return m_cache.getStats();
}

void SectionManager::clearCache()
{
    m_cache.clear();

    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    m_dirtySet.clear();
}

bool SectionManager::isCached(const SectionKey& key) const
{
    return m_cache.contains(key);
}

// ============================================================================
// 脏标记追踪
// ============================================================================

bool SectionManager::markDirty(const SectionKey& key)
{
    bool success = m_cache.markDirty(key);
    if (success) {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.insert(key);
    }
    return success;
}

size_t SectionManager::getDirtyCount() const
{
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_dirtySet.size();
}

std::vector<SectionKey> SectionManager::getDirtyKeys() const
{
    return m_cache.getDirtyKeys();
}

// ============================================================================
// 内部方法
// ============================================================================

Result<std::shared_ptr<const SectionData>> SectionManager::_loadFromDatabase(const SectionKey& key)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::loadFromDatabase",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY));

    // 从数据库读取
    auto keyBytes = key.toKey();
    auto result = m_db.get(m_cfName, keyBytes);

    if (!result.success()) {
        if (result.error().code() == ErrorCode::NotFound) {
            // Section不存在，返回nullptr
            return std::shared_ptr<const SectionData>{};
        }
        return result.error();
    }

    // 反序列化
    const auto& value = result.value();
    auto deserializeResult = SectionData::deserialize(value.data(), value.size());

    if (!deserializeResult.success()) {
        return deserializeResult.error();
    }

    auto data = std::make_shared<SectionData>(std::move(deserializeResult.value()));

    // 从RocksDB键恢复key（key不存储在序列化数据中）
    data->key = key;

    // 放入缓存
    m_cache.put(key, data, false);

    return std::static_pointer_cast<const SectionData>(data);
}

Result<std::vector<std::shared_ptr<const SectionData>>> SectionManager::_loadFromDatabaseBatch(
    const std::vector<SectionKey>& keys)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section, "SectionManager::loadFromDatabaseBatch", "count", keys.size());

    std::vector<std::vector<u8>> keyBytesList;
    keyBytesList.reserve(keys.size());
    for (const auto& key : keys) {
        keyBytesList.push_back(key.toKey());
    }

    auto multiGetResult = m_db.multiGet(m_cfName, keyBytesList);
    if (multiGetResult.failed()) {
        return multiGetResult.error();
    }

    const auto& rawResults = multiGetResult.value();
    MC_ASSERT_RELEASE(rawResults.size() == keys.size());

    std::vector<std::shared_ptr<const SectionData>> results;
    results.reserve(keys.size());

    for (size_t i = 0; i < rawResults.size(); ++i) {
        const auto& rawResult = rawResults[i];
        const auto& key = keys[i];

        if (rawResult.failed()) {
            if (rawResult.error().code() == ErrorCode::NotFound) {
                results.emplace_back(nullptr);
                continue;
            }
            return rawResult.error();
        }

        const auto& value = rawResult.value();
        auto deserializeResult = SectionData::deserialize(value.data(), value.size());
        if (deserializeResult.failed()) {
            return deserializeResult.error();
        }

        auto data = std::make_shared<SectionData>(std::move(deserializeResult.value()));
        data->key = key;
        m_cache.put(key, data, false);
        results.push_back(std::static_pointer_cast<const SectionData>(data));
    }

    return results;
}

Result<void> SectionManager::_saveToDatabase(const SectionKey& key, const SectionData& data, bool sync)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Storage.Section,
        "SectionManager::saveToDatabase",
        "chunkX",
        key.chunkX,
        "chunkZ",
        key.chunkZ,
        "sectionY",
        static_cast<i32>(key.sectionY),
        "sync",
        sync);

    // 计算哈希时使用本地副本，避免原地修改共享缓存对象
    if (m_config.computeHash) {
        SectionData dataToSerialize = data;
        dataToSerialize.computeHash();

        const bool syncWrites = sync || m_config.consistencyMode != ConsistencyMode::Eventual;

        auto serializeResult = dataToSerialize.serialize();
        if (!serializeResult.success()) {
            return serializeResult.error();
        }

        // 写入数据库
        auto keyBytes = key.toKey();
        return m_db.put(m_cfName, keyBytes, serializeResult.value(), syncWrites);
    }

    const bool syncWrites = sync || m_config.consistencyMode != ConsistencyMode::Eventual;

    // 序列化
    auto serializeResult = data.serialize();
    if (!serializeResult.success()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto keyBytes = key.toKey();
    return m_db.put(m_cfName, keyBytes, serializeResult.value(), syncWrites);
}

} // namespace mc::world::storage
