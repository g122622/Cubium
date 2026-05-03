#include "SectionManager.hpp"
#include "../../../perfetto/TraceEvents.hpp"
#include <spdlog/spdlog.h>
#include <mutex>

namespace mc::world::storage {

// ============================================================================
// 构造与析构
// ============================================================================

SectionManager::SectionManager(
    RocksDBDatabase& db,
    DimensionId dimension,
    const Config& config
)
    : m_db(db)
    , m_dimension(dimension)
    , m_cfName(cf::getSectionCF(dimension))
    , m_config(config)
    , m_cache(config.cacheCapacity)
{
    spdlog::info("SectionManager created for dimension {} (CF: {})",
            static_cast<i32>(m_dimension), m_cfName);
}

// ============================================================================
// Section加载
// ============================================================================

Result<SectionData*> SectionManager::loadSection(const SectionKey& key) {
    MC_TRACE_EVENT("storage.section", "SectionManager::loadSection",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

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
        MC_TRACE_EVENT("storage.section", "SectionManager::loadSection.cacheHit");
        return cached.get();
    }

    // 从数据库加载
    return loadFromDatabase(key);
}

std::future<Result<SectionData*>> SectionManager::loadSectionAsync(
    const SectionKey& key,
    util::TaskPriority priority
) {
    MC_TRACE_EVENT("storage.section", "SectionManager::loadSectionAsync",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

    // 使用std::async简化异步实现
    // 生产环境应该使用ServerWorkerPool
    return std::async(std::launch::async, [this, key]() {
        return loadSection(key);
    });
}

void SectionManager::loadSections(
    const std::vector<SectionKey>& keys,
    LoadCallback callback
) {
    MC_TRACE_EVENT("storage.section", "SectionManager::loadSections",
                   "count", keys.size());

    for (const auto& key : keys) {
        auto result = loadSection(key);
        callback(key, result.success() ? result.value() : nullptr);
    }
}

// ============================================================================
// Section保存
// ============================================================================

Result<void> SectionManager::saveSection(
    const SectionKey& key,
    const SectionData& data,
    bool immediate
) {
    MC_TRACE_EVENT("storage.section", "SectionManager::saveSection",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY),
                   "immediate", immediate);

    // 检查维度是否匹配
    if (key.dimension != m_dimension) {
        return Error(ErrorCode::InvalidArgument,
                     fmt::format("Dimension mismatch: expected {}, got {}",
                                 static_cast<i32>(m_dimension),
                                 static_cast<i32>(key.dimension)));
    }

    // 保存到数据库
    auto result = saveToDatabase(key, data, immediate);
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
    const SectionKey& key,
    const SectionData& data,
    util::TaskPriority priority
) {
    MC_TRACE_EVENT("storage.section", "SectionManager::saveSectionAsync",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

    return std::async(std::launch::async, [this, key, data]() {
        return saveSection(key, data);
    });
}

Result<size_t> SectionManager::flushDirtySections() {
    MC_TRACE_EVENT("storage.section", "SectionManager::flushDirtySections");

    // 获取所有脏Section
    auto dirtySections = m_cache.getDirtySections();

    if (dirtySections.empty()) {
        return 0;
    }

    spdlog::info("Flushing {} dirty sections", dirtySections.size());

    // 批量保存
    rocksdb::WriteBatch batch;
    size_t savedCount = 0;

    for (const auto& [key, data] : dirtySections) {
        if (!data) {
            continue;
        }

        // 序列化Section
        auto serializeResult = data->serialize();
        if (!serializeResult.success()) {
            spdlog::error("Failed to serialize section at ({}, {}, {}): {}",
                     key.chunkX, key.chunkZ, static_cast<i32>(key.sectionY),
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
            return Error(ErrorCode::InvalidState,
                         fmt::format("Column family not found: {}", m_cfName));
        }

        batch.Put(cf, keySlice, valueSlice);
        ++savedCount;
    }

    // 执行批量写入
    if (savedCount > 0) {
        auto writeResult = m_db.writeBatch(batch, true); // sync=true
        if (!writeResult.success()) {
            return writeResult.error();
        }
    }

    // 标记所有脏Section为干净
    for (const auto& [key, data] : dirtySections) {
        m_cache.markClean(key);
    }

    // 清空脏集合
    {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.clear();
    }

    spdlog::info("Flushed {} dirty sections", savedCount);

    return savedCount;
}

Result<size_t> SectionManager::saveAll() {
    MC_TRACE_EVENT("storage.section", "SectionManager::saveAll");

    // 获取所有缓存中的Section
    auto dirtySections = m_cache.getDirtySections();
    size_t savedCount = 0;

    for (const auto& [key, data] : dirtySections) {
        if (!data) {
            continue;
        }

        auto result = saveSection(key, *data, false);
        if (result.success()) {
            ++savedCount;
        }
    }

    return savedCount;
}

// ============================================================================
// Section卸载
// ============================================================================

Result<void> SectionManager::unloadSection(const SectionKey& key) {
    MC_TRACE_EVENT("storage.section", "SectionManager::unloadSection",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

    // 检查是否为脏
    bool isDirty = m_cache.isDirty(key);

    // 如果为脏，先保存
    if (isDirty) {
        auto data = m_cache.get(key);
        if (data) {
            auto saveResult = saveSection(key, *data, false);
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

Result<void> SectionManager::unloadAll() {
    MC_TRACE_EVENT("storage.section", "SectionManager::unloadAll");

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

Result<void> SectionManager::deleteSection(const SectionKey& key) {
    MC_TRACE_EVENT("storage.section", "SectionManager::deleteSection",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

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

Result<size_t> SectionManager::deleteChunkSections(i32 chunkX, i32 chunkZ) {
    MC_TRACE_EVENT("storage.section", "SectionManager::deleteChunkSections",
                   "chunkX", chunkX,
                   "chunkZ", chunkZ);

    // 构造范围删除的起止键
    // SectionKey格式: dimension:2 + chunkX:4 + chunkZ:4 + sectionY:1 + padding:2
    // 同一区块的所有Section，chunkX和chunkZ相同，sectionY从-4到19

    SectionKey startKey(chunkX, chunkZ, -128, m_dimension); // 最小sectionY
    SectionKey endKey(chunkX, chunkZ, 127, m_dimension);    // 最大sectionY

    auto startBytes = startKey.toKey();
    auto endBytes = endKey.toKey();

    // 范围删除
    auto result = m_db.deleteRange(m_cfName, startBytes, endBytes);
    if (!result.success()) {
        return result.error();
    }

    // 从缓存中移除该区块的所有Section
    for (i8 sectionY = -4; sectionY <= 19; ++sectionY) {
        SectionKey key(chunkX, chunkZ, sectionY, m_dimension);
        m_cache.evict(key);

        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.erase(key);
    }

    // 返回删除的数量（估算）
    return 24; // -4到19共24个Section
}

// ============================================================================
// 缓存管理
// ============================================================================

void SectionManager::setCacheCapacity(size_t capacity) {
    m_cache.setCapacity(capacity);
    m_config.cacheCapacity = capacity;
}

SectionCache::CacheStats SectionManager::getCacheStats() const {
    return m_cache.getStats();
}

void SectionManager::clearCache() {
    m_cache.clear();

    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    m_dirtySet.clear();
}

bool SectionManager::isCached(const SectionKey& key) const {
    return m_cache.contains(key);
}

// ============================================================================
// 脏标记追踪
// ============================================================================

bool SectionManager::markDirty(const SectionKey& key) {
    bool success = m_cache.markDirty(key);
    if (success) {
        std::lock_guard<std::mutex> lock(m_dirtyMutex);
        m_dirtySet.insert(key);
    }
    return success;
}

size_t SectionManager::getDirtyCount() const {
    std::lock_guard<std::mutex> lock(m_dirtyMutex);
    return m_dirtySet.size();
}

std::vector<SectionKey> SectionManager::getDirtyKeys() const {
    return m_cache.getDirtyKeys();
}

// ============================================================================
// 内部方法
// ============================================================================

Result<SectionData*> SectionManager::loadFromDatabase(const SectionKey& key) {
    MC_TRACE_EVENT("storage.section", "SectionManager::loadFromDatabase",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY));

    // 从数据库读取
    auto keyBytes = key.toKey();
    auto result = m_db.get(m_cfName, keyBytes);

    if (!result.success()) {
        if (result.error().code() == ErrorCode::NotFound) {
            // Section不存在，返回nullptr
            return nullptr;
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

    return data.get();
}

Result<void> SectionManager::saveToDatabase(
    const SectionKey& key,
    const SectionData& data,
    bool sync
) {
    MC_TRACE_EVENT("storage.section", "SectionManager::saveToDatabase",
                   "chunkX", key.chunkX,
                   "chunkZ", key.chunkZ,
                   "sectionY", static_cast<i32>(key.sectionY),
                   "sync", sync);

    // 计算哈希（如果启用）
    if (m_config.computeHash) {
        const_cast<SectionData&>(data).computeHash();
    }

    // 序列化
    auto serializeResult = data.serialize();
    if (!serializeResult.success()) {
        return serializeResult.error();
    }

    // 写入数据库
    auto keyBytes = key.toKey();
    return m_db.put(m_cfName, keyBytes, serializeResult.value(), sync);
}

} // namespace mc::world::storage
