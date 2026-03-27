#include "RegionFileCache.hpp"

namespace mc::world::save::region {

RegionFileCache::RegionFileCache(const std::filesystem::path& regionDir,
                                 bool sync,
                                 u32 maxCacheSize)
    : m_regionDir(regionDir)
    , m_sync(sync)
    , m_maxCacheSize(maxCacheSize)
{
}

RegionFileCache::~RegionFileCache() {
    close();
}

RegionFileCache::RegionFileCache(RegionFileCache&& other) noexcept
    : m_regionDir(std::move(other.m_regionDir))
    , m_sync(other.m_sync)
    , m_maxCacheSize(other.m_maxCacheSize)
    , m_cache(std::move(other.m_cache))
    , m_lruList(std::move(other.m_lruList))
    , m_lruMap(std::move(other.m_lruMap))
{
    other.m_cache.clear();
    other.m_lruList.clear();
    other.m_lruMap.clear();
}

RegionFileCache& RegionFileCache::operator=(RegionFileCache&& other) noexcept {
    if (this != &other) {
        close();
        m_regionDir = std::move(other.m_regionDir);
        m_sync = other.m_sync;
        m_maxCacheSize = other.m_maxCacheSize;
        m_cache = std::move(other.m_cache);
        m_lruList = std::move(other.m_lruList);
        m_lruMap = std::move(other.m_lruMap);
        other.m_cache.clear();
        other.m_lruList.clear();
        other.m_lruMap.clear();
    }
    return *this;
}

// ========== 区块操作 ==========

Result<std::optional<nbt::CompoundTag>>
RegionFileCache::readChunk(ChunkCoord chunkX, ChunkCoord chunkZ) {
    i32 regionX, regionZ;
    getRegionCoords(chunkX, chunkZ, regionX, regionZ);

    auto regionResult = getOrCreateRegionFile(regionX, regionZ);
    if (regionResult.failed()) {
        return regionResult.error();
    }

    auto& regionFile = regionResult.value();
    u32 localX = static_cast<u32>(chunkX & 31);
    u32 localZ = static_cast<u32>(chunkZ & 31);

    return regionFile->readChunk(localX, localZ);
}

Result<void>
RegionFileCache::writeChunk(ChunkCoord chunkX, ChunkCoord chunkZ, const nbt::CompoundTag& nbt) {
    i32 regionX, regionZ;
    getRegionCoords(chunkX, chunkZ, regionX, regionZ);

    auto regionResult = getOrCreateRegionFile(regionX, regionZ);
    if (regionResult.failed()) {
        return regionResult.error();
    }

    auto& regionFile = regionResult.value();
    u32 localX = static_cast<u32>(chunkX & 31);
    u32 localZ = static_cast<u32>(chunkZ & 31);

    return regionFile->writeChunk(localX, localZ, nbt);
}

bool RegionFileCache::hasChunk(ChunkCoord chunkX, ChunkCoord chunkZ) {
    i32 regionX, regionZ;
    getRegionCoords(chunkX, chunkZ, regionX, regionZ);

    std::lock_guard<std::mutex> lock(m_mutex);

    u64 key = getRegionKey(regionX, regionZ);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        u32 localX = static_cast<u32>(chunkX & 31);
        u32 localZ = static_cast<u32>(chunkZ & 31);
        return it->second->hasChunk(localX, localZ);
    }

    // Region 文件不在缓存中，检查文件是否存在
    std::filesystem::path regionPath = m_regionDir /
        ("r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca");

    if (!std::filesystem::exists(regionPath)) {
        return false;
    }

    // 文件存在，但我们需要打开它来检查区块
    auto regionResult = getOrCreateRegionFile(regionX, regionZ);
    if (regionResult.failed()) {
        return false;
    }

    u32 localX = static_cast<u32>(chunkX & 31);
    u32 localZ = static_cast<u32>(chunkZ & 31);
    return regionResult.value()->hasChunk(localX, localZ);
}

Result<void> RegionFileCache::deleteChunk(ChunkCoord chunkX, ChunkCoord chunkZ) {
    i32 regionX, regionZ;
    getRegionCoords(chunkX, chunkZ, regionX, regionZ);

    auto regionResult = getOrCreateRegionFile(regionX, regionZ);
    if (regionResult.failed()) {
        return regionResult.error();
    }

    auto& regionFile = regionResult.value();
    u32 localX = static_cast<u32>(chunkX & 31);
    u32 localZ = static_cast<u32>(chunkZ & 31);

    return regionFile->deleteChunk(localX, localZ);
}

// ========== 同步与关闭 ==========

Result<void> RegionFileCache::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [key, regionFile] : m_cache) {
        auto result = regionFile->flush();
        if (result.failed()) {
            return result.error();
        }
    }

    return {};
}

void RegionFileCache::close() {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (auto& [key, regionFile] : m_cache) {
        regionFile->close();
    }

    m_cache.clear();
    m_lruList.clear();
    m_lruMap.clear();
}

void RegionFileCache::closeRegion(i32 regionX, i32 regionZ) {
    std::lock_guard<std::mutex> lock(m_mutex);

    u64 key = getRegionKey(regionX, regionZ);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        it->second->close();
        m_cache.erase(it);

        // 从 LRU 列表中移除
        auto lruIt = m_lruMap.find(key);
        if (lruIt != m_lruMap.end()) {
            m_lruList.erase(lruIt->second);
            m_lruMap.erase(lruIt);
        }
    }
}

// ========== 信息查询 ==========

u32 RegionFileCache::cacheSize() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<u32>(m_cache.size());
}

void RegionFileCache::setMaxCacheSize(u32 maxCacheSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxCacheSize = maxCacheSize;

    // 如果当前缓存超过限制，驱逐最久未使用的
    while (m_cache.size() > m_maxCacheSize) {
        evictOldest();
    }
}

// ========== 私有方法 ==========

Result<std::shared_ptr<RegionFile>>
RegionFileCache::getOrCreateRegionFile(i32 regionX, i32 regionZ) {
    u64 key = getRegionKey(regionX, regionZ);

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 检查缓存
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            // 更新 LRU 顺序
            touch(key);
            return it->second;
        }

        // 如果缓存已满，驱逐最久未使用的
        while (m_cache.size() >= m_maxCacheSize) {
            evictOldest();
        }
    }

    // 创建 Region 文件路径
    std::filesystem::path regionPath = m_regionDir /
        ("r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca");

    // 打开或创建 Region 文件
    auto regionResult = RegionFile::open(regionPath, m_sync);
    if (regionResult.failed()) {
        return regionResult.error();
    }

    // 添加到缓存
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 再次检查（可能在创建过程中其他线程已经添加）
        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            return it->second;
        }

        auto regionFile = std::shared_ptr<RegionFile>(regionResult.value().release());
        m_cache[key] = regionFile;
        m_lruList.push_back(key);
        m_lruMap[key] = std::prev(m_lruList.end());

        return regionFile;
    }
}

void RegionFileCache::evictOldest() {
    // 注意：调用者必须持有锁
    if (m_lruList.empty()) {
        return;
    }

    u64 oldestKey = m_lruList.front();
    m_lruList.pop_front();
    m_lruMap.erase(oldestKey);

    auto it = m_cache.find(oldestKey);
    if (it != m_cache.end()) {
        it->second->close();
        m_cache.erase(it);
    }
}

void RegionFileCache::touch(u64 key) {
    // 注意：调用者必须持有锁
    auto it = m_lruMap.find(key);
    if (it != m_lruMap.end()) {
        // 移动到列表末尾
        m_lruList.erase(it->second);
        m_lruList.push_back(key);
        m_lruMap[key] = std::prev(m_lruList.end());
    }
}

} // namespace mc::world::save::region
